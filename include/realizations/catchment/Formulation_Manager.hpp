#ifndef NGEN_FORMULATION_MANAGER_H
#define NGEN_FORMULATION_MANAGER_H

#include <NGenConfig.h>

#include <memory>
#include <sstream>
#include <tuple>
#include <functional>
#include <dirent.h>
#include <sys/stat.h>
#include <regex>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <FeatureBuilder.hpp>
#include "features/Features.hpp"
#include "Formulation_Constructors.hpp"
#include "LayerData.hpp"
#include "realizations/config/time.hpp"
#include "realizations/config/routing.hpp"
#include "realizations/config/config.hpp"
#include "realizations/config/layer.hpp"

namespace realization {

    class Formulation_Manager {
        public:

            std::shared_ptr<Simulation_Time> Simulation_Time_Object;

            Formulation_Manager(std::stringstream &data) {
                boost::property_tree::ptree loaded_tree;
                boost::property_tree::json_parser::read_json(data, loaded_tree);
                this->tree = loaded_tree;
            }

            Formulation_Manager(const std::string &file_path) {
                boost::property_tree::ptree loaded_tree;
                boost::property_tree::json_parser::read_json(file_path, loaded_tree);
                this->tree = loaded_tree;
            }

            Formulation_Manager(boost::property_tree::ptree &loaded_tree) {
                this->tree = loaded_tree;
            }

            ~Formulation_Manager() = default;

            virtual void read(geojson::GeoJSON fabric, utils::StreamHandler output_stream) {
                //TODO seperate the parsing of configuration options like time
                //and routing and other non feature specific tasks from this main function
                //which has to iterate the entire hydrofabric.
                auto possible_global_config = tree.get_child_optional("global");

                if (possible_global_config) {
                    global_config = realization::config::Config(*possible_global_config);
                }

                auto possible_simulation_time = tree.get_child_optional("time");

                if (!possible_simulation_time) {
                    throw std::runtime_error("ERROR: No simulation time period defined.");
                }
                config::Time time = config::Time(*possible_simulation_time);
                auto simulation_time_config = time.make_params();
                /**
                 * Call constructor to construct a Simulation_Time object
                 */ 
                this->Simulation_Time_Object = std::make_shared<Simulation_Time>(simulation_time_config);

                /**
                 * Read the layer descriptions
                */

                // try to get the json node
                auto layers_json_array = tree.get_child_optional("layers");
                //Create the default surface layer
                config::Layer layer;
                // layer description struct
                ngen::LayerDescription layer_desc;
                layer_desc = layer.get_descriptor();
                // add the default surface layer to storage
                layer_storage.put_layer(layer_desc, layer_desc.id);

                if(layers_json_array){
                    
                    for (std::pair<std::string, boost::property_tree::ptree> layer_config : *layers_json_array) 
                    {
                        layer = config::Layer(layer_config.second);
                        layer_desc = layer.get_descriptor();

                        // add the layer to storage
                        layer_storage.put_layer(layer_desc, layer_desc.id);
                        if(layer.has_formulation() && layer.get_domain()=="catchments"){
                            double c_value = UnitsHelper::get_converted_value(layer_desc.time_step_units,layer_desc.time_step,"s");
                            // make a new simulation time object with a different output interval
                            Simulation_Time sim_time(*Simulation_Time_Object, c_value);
                            domain_formulations.emplace(
                                layer_desc.id,
                                construct_formulation_from_config(simulation_time_config,
                                "layer-"+std::to_string(layer_desc.id),
                                layer.formulation,
                                output_stream
                                )
                            );
                            domain_formulations.at(layer_desc.id)->set_output_stream(get_output_root() + layer_desc.name + "_layer_"+std::to_string(layer_desc.id) + ".csv");
                        }
                        //TODO for each layer, create deferred providers for use by other layers
                        //VERY SIMILAR TO NESTED MODULE INIT
                    }
                }

                //TODO use the set of layer providers as input for catchments to lookup from

                /**
                 * Read routing configurations from configuration file
                 */      
                auto possible_routing_configs = tree.get_child_optional("routing");
                
                if (possible_routing_configs) {
                    //Since it is possible to build NGEN without routing support, if we see it in the config
                    //but it isn't enabled in the build, we should at least put up a warning
                #if NGEN_WITH_ROUTING
                    this->routing_config = (config::Routing(*possible_routing_configs)).params;
                    using_routing = true;
                #else
                    using_routing = false;
                    std::cerr<<"WARNING: Formulation Manager found routing configuration"
                             <<", but routing support isn't enabled. No routing will occur."<<std::endl;
                #endif //NGEN_WITH_ROUTING
                 }

                /**
                 * Read catchment configurations from configuration file
                 */      
                auto possible_catchment_configs = tree.get_child_optional("catchments");

                if (possible_catchment_configs) {
                    for (std::pair<std::string, boost::property_tree::ptree> catchment_config : *possible_catchment_configs) {
                      int catchment_index = fabric->find(catchment_config.first);
                      if( catchment_index == -1 )
                      {
                          #ifndef NGEN_QUIET
                          std::cerr<<"WARNING Formulation_Manager::read: Cannot create formulation for catchment "
                                  <<catchment_config.first
                                  <<" that isn't identified in the hydrofabric or requested subset"<<std::endl;
                          #endif
                          continue;
                      }
                      realization::config::Config catchment_formulation(catchment_config.second);

                      if(!catchment_formulation.has_formulation()){
                        throw std::runtime_error("ERROR: No formulations defined for "+catchment_config.first+".");
                      }
                      // Parse catchment-specific model_params
                      auto catchment_feature = fabric->get_feature(catchment_index);
                      catchment_formulation.formulation.link_external(catchment_feature);
                      this->add_formulation(
                        this->construct_formulation_from_config(
                            simulation_time_config,
                            catchment_config.first,
                            catchment_formulation,
                            output_stream
                        )
                      );
                        //  break; //only construct one for now FIXME
                       // } //end for formulaitons
                      }//end for catchments


                }//end if possible_catchment_configs

                for (geojson::Feature location : *fabric) {
                    if (not this->contains(location->get_id())) {
                        std::shared_ptr<Catchment_Formulation> missing_formulation = this->construct_missing_formulation(
                          location, output_stream, simulation_time_config);
                        this->add_formulation(missing_formulation);
                    }
                }
            }

            virtual void add_formulation(std::shared_ptr<Catchment_Formulation> formulation) {
                this->formulations.emplace(formulation->get_id(), formulation);
            }

            virtual std::shared_ptr<Catchment_Formulation> get_formulation(std::string id) const {
                // TODO: Implement on-the-fly formulation creation using global parameters
                return this->formulations.at(id);
            }

            virtual std::shared_ptr<Catchment_Formulation> get_domain_formulation(long id) const {
                return this->domain_formulations.at(id);
            }

            virtual bool has_domain_formulation(int id) const {
                return this->domain_formulations.count( id ) > 0;
            }

            virtual bool contains(std::string identifier) const {
                return this->formulations.count(identifier) > 0;
            }

            /**
             * @return The number of elements within the collection
             */
            virtual int get_size() {
                return this->formulations.size();
            }

            /**
             * @return Whether or not the collection is empty
             */
            virtual bool is_empty() {
                return this->formulations.empty();
            }

            virtual typename std::map<std::string, std::shared_ptr<Catchment_Formulation>>::const_iterator begin() const {
                return this->formulations.cbegin();
            }

            virtual typename std::map<std::string, std::shared_ptr<Catchment_Formulation>>::const_iterator end() const {
                return this->formulations.cend();
            }

            /**
             * @return Whether or not using routing
             */
            bool get_using_routing() {
                return this->using_routing;
            }

            /**
             * @return routing t_route_config_file_with_path
             */
            std::string get_t_route_config_file_with_path() {
                if(this->routing_config != nullptr)
                    return this->routing_config->t_route_config_file_with_path;
                else
                    return "";
            }

            /**
             * Release any resources that should not be held as the run is shutting down
             *
             * In particular, this should be called before MPI_Finalize()
             */
            void finalize() {
                // The calls in these loops are staticly dispatched to
                // Catchment_Formulation::finalize(). That does not
                // inherit from DataProvider, with its virtual member
                // function of the same name.
                //
                // If any formulation class needs to customize this
                // behavior through this becoming a virtual dispatch,
                // take care. Bmi_Multi_Formulation was a concern, but
                // does not currently need to because none of its
                // constituent formulations points to any forcing
                // object other than the enclosing
                // Bmi_Multi_Formulation instance itself.
                for (auto const& fmap: formulations) {
                    fmap.second->finalize();
                }
                for (auto const& fmap: domain_formulations) {
                    fmap.second->finalize();
                }

#if NGEN_WITH_NETCDF
                data_access::NetCDFPerFeatureDataProvider::cleanup_shared_providers();
#endif
            }

            /**
             * @brief Get the formatted output root
             *
             * @code{.cpp}
             * // Example config:
             * // ...
             * // "output_root": "/path/to/dir/"
             * // ...
             * const auto manager = Formulation_Manger(CONFIG);
             * manager.get_output_root();
             * //> "/path/to/dir/"
             * @endcode
             * 
             * @return std::string of the output root directory
             */
            std::string get_output_root() const noexcept {
                const auto output_root = this->tree.get_optional<std::string>("output_root");
                if (output_root != boost::none && *output_root != "") {
                    // Check if the path ends with a trailing slash,
                    // otherwise add it.
                    return output_root->back() == '/'
                           ? *output_root
                           : *output_root + "/";
                }

                return "./";
            }

            /**
             * @brief return the layer storage used for formulations
             * @return a reference to the LayerStorageObject
             */
            ngen::LayerDataStorage& get_layer_metadata() { return layer_storage; }


        protected:
            std::shared_ptr<Catchment_Formulation> construct_formulation_from_config(
                simulation_time_params &simulation_time_config,
                std::string identifier,
                const realization::config::Config& catchment_formulation,
                utils::StreamHandler output_stream
            ) {
                if(!formulation_exists(catchment_formulation.formulation.type)){
                    throw std::runtime_error("Catchment " + identifier + " failed initialization: '" +
                            catchment_formulation.formulation.type + "' is not a valid formulation. Options are: "+valid_formulation_keys());
                }

                if(catchment_formulation.forcing.parameters.empty()){
                    throw std::runtime_error("No forcing definition was found for " + identifier);
                }

                std::vector<std::string> missing_parameters;
                
                if (!catchment_formulation.forcing.has_key("path")) {
                    missing_parameters.push_back("path");
                }

                if (missing_parameters.size() > 0) {
                    std::string message = "A forcing configuration cannot be created for '" + identifier + "'; the following parameters are missing: ";

                    for (int missing_parameter_index = 0; missing_parameter_index < missing_parameters.size(); missing_parameter_index++) {
                        message += missing_parameters[missing_parameter_index];

                        if (missing_parameter_index < missing_parameters.size() - 1) {
                            message += ", ";
                        }
                    }
                    
                    throw std::runtime_error(message);
                }

                forcing_params forcing_config = this->get_forcing_params(catchment_formulation.forcing.parameters, identifier, simulation_time_config);
                std::shared_ptr<Catchment_Formulation> constructed_formulation = construct_formulation(catchment_formulation.formulation.type, identifier, forcing_config, output_stream);
                //, geometry);

                constructed_formulation->create_formulation(catchment_formulation.formulation.parameters);
                return constructed_formulation;
            }

            std::shared_ptr<Catchment_Formulation> construct_missing_formulation(geojson::Feature& feature, utils::StreamHandler output_stream, simulation_time_params &simulation_time_config){
                const std::string identifier = feature->get_id();
  
                forcing_params forcing_config = this->get_forcing_params(global_config.forcing.parameters, identifier, simulation_time_config);
                std::shared_ptr<Catchment_Formulation> missing_formulation = construct_formulation(global_config.formulation.type, identifier, forcing_config, output_stream);
                // Need to work with a copy, since it is altered in-place
                realization::config::Config global_copy = global_config;
                Catchment_Formulation::config_pattern_substitution(global_copy.formulation.parameters,
                                                                   BMI_REALIZATION_CFG_PARAM_REQ__INIT_CONFIG, "{{id}}",
                                                                   identifier);
                //Some helpful debugging prints, commented out, but left for later
                //because they will eventually be used by someone, someday, looking at configurations
                //being turned into concrecte formulations...
                // geojson::JSONProperty::print_property(global_config.formulation.parameters.at("modules"));
                global_config.formulation.link_external(feature);
                // geojson::JSONProperty::print_property(global_config.formulation.parameters.at("modules"));
                missing_formulation->create_formulation(global_config.formulation.parameters);

                return missing_formulation;
            }

            forcing_params get_forcing_params(const geojson::PropertyMap &forcing_prop_map, std::string identifier, simulation_time_params &simulation_time_config) {
                std::string path = "";
                if(forcing_prop_map.count("path") != 0){
                    path = forcing_prop_map.at("path").as_string();
                }
                std::string provider;
                if(forcing_prop_map.count("provider") != 0){
                    provider = forcing_prop_map.at("provider").as_string();
                }
                if (forcing_prop_map.count("file_pattern") == 0) {
                    return forcing_params(
                        path,
                        provider,
                        simulation_time_config.start_time,
                        simulation_time_config.end_time
                    );
                }

                if (path.empty()) {
                    throw std::runtime_error("Error with NGEN config - 'path' in forcing params must be set to a "
                                             "non-empty parent directory path when 'file_pattern' is used.");
                }

                // Since we are given a pattern, we need to identify the directory and pull out anything that matches the pattern
                if (path.compare(path.size() - 1, 1, "/") != 0) {
                    path += "/";
                }

                std::string filepattern = forcing_prop_map.at("file_pattern").as_string();

                int id_index = filepattern.find("{{id}}");

                // If an index for '{{id}}' was found, we can count on that being where the id for this realization can be found.
                //     For instance, if we have a pattern of '.*{{id}}_14_15.csv' and this is named 'cat-87',
                //     this will match on 'stuff_example_cat-87_14_15.csv'
                if (id_index != std::string::npos) {
                    filepattern = filepattern.replace(id_index, sizeof("{{id}}") - 1, identifier);
                }

                // Create a regular expression used to identify proper file names
                std::regex pattern(filepattern);

                // A stream providing the functions necessary for evaluating a directory:
                //    https://www.gnu.org/software/libc/manual/html_node/Opening-a-Directory.html#Opening-a-Directory
                DIR *directory = nullptr;

                // structure representing the member of a directory: https://www.gnu.org/software/libc/manual/html_node/Directory-Entries.html
                struct dirent *entry = nullptr;

                // Attempt to open the directory for evaluation
                directory = opendir(path.c_str());
                // Allow for a few retries in certain failure situations
                size_t attemptCount = 0;
                std::string errMsg;
                while (directory == nullptr && attemptCount++ < 5) {
                    // For several error codes, we should break immediately and not retry
                    if (errno == ENOENT) {
                        errMsg = "No such file or directory.";
                        break;
                    }
                    if (errno == ENXIO) {
                        errMsg = "No such device or address.";
                        break;
                    }
                    if (errno == EACCES) {
                        errMsg = "Permission denied.";
                        break;
                    }
                    if (errno == EPERM) {
                        errMsg = "Operation not permitted.";
                        break;
                    }
                    if (errno == ENOTDIR) {
                        errMsg = "File at provided path is not a directory.";
                        break;
                    }
                    if (errno == EMFILE) {
                        errMsg = "The current process has too many open files.";
                        break;
                    }
                    if (errno == ENFILE) {
                        errMsg = "The system has too many open files.";
                        break;
                    }
                    sleep(2);
                    directory = opendir(path.c_str());
                    errMsg = "Received system error number " + std::to_string(errno);
                }

                // If the directory could be found and opened, we can go ahead and iterate
                if (directory != nullptr) {
                    bool match;
                    while ((entry = readdir(directory))) {
                        match = std::regex_match(entry->d_name, pattern);
                        if( match ) {
                            // If the entry is a regular file or symlink AND the name matches the pattern, 
                            //    we can consider this ready to be interpretted as valid forcing data (even if it isn't)
                            #ifdef _DIRENT_HAVE_D_TYPE
                            if ( entry->d_type == DT_REG or entry->d_type == DT_LNK ) {
                                return forcing_params(
                                    path + entry->d_name,
                                    provider,
                                    simulation_time_config.start_time,
                                    simulation_time_config.end_time
                                );
                            }
                            else if ( entry->d_type == DT_UNKNOWN )
                            #endif
                            {
                                //dirent is not guaranteed to provide propoer file type identification in d_type
                                //so if a system returns unknown or it isn't supported, need to use stat to determine if it is a file
                                struct stat st;
                                if( stat((path+entry->d_name).c_str(), &st) != 0) {
                                    throw std::runtime_error("Could not stat file "+path+entry->d_name);
                                }
                                if( S_ISREG(st.st_mode) ) {
                                    //Sinde we used stat and not lstat, we get the result of the target of links as well
                                    //so this covers both cases we are interested in.
                                    return forcing_params(
                                        path + entry->d_name,
                                        provider,
                                        simulation_time_config.start_time,
                                        simulation_time_config.end_time
                                    );
                                }
                                throw std::runtime_error("Forcing data is path "+path+entry->d_name+" is not a file");
                            }
                        } //no match found, try next entry
                    } // end while iter dir
                } //end if directory
                else {
                    // The directory wasn't found or otherwise couldn't be opened; forcing data cannot be retrieved
                    throw std::runtime_error("Error opening forcing data dir '" + path + "' after " + std::to_string(attemptCount) + " attempts: " + errMsg);
                }

                closedir(directory);

                throw std::runtime_error("Forcing data could not be found for '" + identifier + "'");
            }

            /**
             * @brief Parse a `model_params` property tree and replace external parameters
             *        with values from a catchment's properties
             * 
             * @param model_params Property tree with root key "model_params"
             * @param catchment_feature Associated catchment feature
             */
            void parse_external_model_params(boost::property_tree::ptree& model_params, const geojson::Feature catchment_feature) {
                 boost::property_tree::ptree attr {};
                 for (decltype(auto) param : model_params) {
                    if (param.second.count("source") == 0) {
                        attr.put_child(param.first, param.second);
                        continue;
                    }
                
                    decltype(auto) param_source = param.second.get_child("source");
                    decltype(auto) param_source_name = param_source.get_value<std::string>();
                    if (param_source_name != "hydrofabric") {
                        // temporary until the logic for alternative sources is designed
                        throw std::logic_error("ERROR: 'model_params' source `" + param_source_name + "` not currently supported. Only `hydrofabric` is supported.");
                    }

                    decltype(auto) param_name = param.second.find("from") == param.second.not_found()
                        ? param.first
                        : param.second.get_child("from").get_value<std::string>();

                    if (catchment_feature->has_property(param_name)) {
                        auto catchment_attribute = catchment_feature->get_property(param_name);
                        switch (catchment_attribute.get_type()) {
                            case geojson::PropertyType::Natural:
                                attr.put(param.first, catchment_attribute.as_natural_number());
                                break;
                            case geojson::PropertyType::Boolean:
                                attr.put(param.first, catchment_attribute.as_boolean());
                                break;
                            case geojson::PropertyType::Real:
                                attr.put(param.first, catchment_attribute.as_real_number());
                                break;
                            case geojson::PropertyType::String:
                                attr.put(param.first, catchment_attribute.as_string());
                                break;

                            case geojson::PropertyType::List:
                            case geojson::PropertyType::Object:
                            default:
                                std::cerr << "WARNING: property type " << static_cast<int>(catchment_attribute.get_type()) << " not allowed as model parameter. "
                                          << "Must be one of: Natural (int), Real (double), Boolean, or String" << '\n';
                                break;
                        }
                    } else {
                        std::cerr << "WARNING Failed to parse external parameter: catchment `"
                                  << catchment_feature->get_id()
                                  << "` does not contain the property `"
                                  << param_name << "`\n";
                    }
                }

                model_params.swap(attr);
            }

            /**
             * @brief Parse a `model_params` property map and replace external parameters
             *        with values from a catchment's properties
             * 
             * @param model_params Property map with root key "model_params"
             * @param catchment_feature Associated catchment feature
             */
            void parse_external_model_params(geojson::PropertyMap& model_params, const geojson::Feature catchment_feature) {
                geojson::PropertyMap attr {};
                for (decltype(auto) param : model_params) {
                    // Check for type to short-circuit. If param.second is not an object, `.has_key()` will throw
                    if (param.second.get_type() != geojson::PropertyType::Object || !param.second.has_key("source")) {
                        attr.emplace(param.first, param.second);
                        continue;
                    }
                
                    decltype(auto) param_source = param.second.at("source");
                    decltype(auto) param_source_name = param_source.as_string();
                    if (param_source_name != "hydrofabric") {
                        // TODO: temporary until the logic for alternative sources is designed
                        throw std::logic_error("ERROR: 'model_params' source `" + param_source_name + "` not currently supported. Only `hydrofabric` is supported.");
                    }

                    // Property name in the feature properties is either
                    // the value of key "from", or has the same name as
                    // the expected model parameter key
                    decltype(auto) param_name = param.second.has_key("from")
                        ? param.second.at("from").as_string()
                        : param.first;

                    if (catchment_feature->has_property(param_name)) {
                        auto catchment_attribute = catchment_feature->get_property(param_name);

                        // Use param.first in the `.emplace` calls instead of param_name since
                        // the expected name is given by the key of the model_params values.
                        switch (catchment_attribute.get_type()) {
                            case geojson::PropertyType::Natural:
                                attr.emplace(param.first, geojson::JSONProperty(param.first, catchment_attribute.as_natural_number()));
                                break;
                            case geojson::PropertyType::Boolean:
                                attr.emplace(param.first, geojson::JSONProperty(param.first, catchment_attribute.as_boolean()));
                                break;
                            case geojson::PropertyType::Real:
                                attr.emplace(param.first, geojson::JSONProperty(param.first, catchment_attribute.as_real_number()));
                                break;
                            case geojson::PropertyType::String:
                                attr.emplace(param.first, geojson::JSONProperty(param.first, catchment_attribute.as_string()));
                                break;

                            case geojson::PropertyType::List:
                            case geojson::PropertyType::Object:
                            default:
                                // TODO: Should list/object values be passed to model parameters?
                                //       Typically, feature properties *should* be scalars.
                                std::cerr << "WARNING: property type " << static_cast<int>(catchment_attribute.get_type()) << " not allowed as model parameter. "
                                          << "Must be one of: Natural (int), Real (double), Boolean, or String" << '\n';
                                break;
                        }
                    }
                }

                model_params.swap(attr);
            }

            boost::property_tree::ptree tree;

            realization::config::Config global_config;

            std::map<std::string, std::shared_ptr<Catchment_Formulation>> formulations;

            //Store global layer formulation pointers
            std::map<int, std::shared_ptr<Catchment_Formulation> > domain_formulations;

            std::shared_ptr<routing_params> routing_config;

            bool using_routing = false;

            ngen::LayerDataStorage layer_storage;
    };
}
#endif // NGEN_FORMULATION_MANAGER_H
