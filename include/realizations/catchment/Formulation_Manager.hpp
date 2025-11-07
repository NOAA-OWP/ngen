#ifndef NGEN_FORMULATION_MANAGER_H
#define NGEN_FORMULATION_MANAGER_H

#include <NGenConfig.h>
#include "Logger.hpp"

#include <memory>
#include <sstream>
#include <tuple>
#include <functional>
#include <dirent.h>
#include <sys/stat.h>
#include <cerrno>
#include <regex>
#include <sys/types.h>
#include <unistd.h>
#include <string>

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
#include "forcing/ForcingsEngineDataProvider.hpp"

namespace realization {

    class Formulation_Manager {
        public:
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

            void read(simulation_time_params &simulation_time_config,
                      geojson::GeoJSON fabric, utils::StreamHandler output_stream) {
                std::stringstream ss;
                ss.str(""); ss << "Entering Formulation_Manager::read()" << std::endl;
                LOG(ss.str(), LogLevel::DEBUG);

                //TODO seperate the parsing of configuration options like time
                //and routing and other non feature specific tasks from this main function
                //which has to iterate the entire hydrofabric.
                auto possible_global_config = tree.get_child_optional("global");

                if (possible_global_config) {
                    global_config = realization::config::Config(*possible_global_config);
                }

                // Log layer descriptions
                // try to get the json node
                auto layers_json_array = tree.get_child_optional("layers");
                //Create the default surface layer
                config::Layer layer;
                // layer description struct
                ngen::LayerDescription layer_desc;
                layer_desc = layer.get_descriptor();
                // add the default surface layer to storage
                layer_storage.put_layer(layer_desc, layer_desc.id);

                if (layers_json_array) {
                    
                    for (std::pair<std::string, boost::property_tree::ptree> layer_config : *layers_json_array) 
                    {
                        layer = config::Layer(layer_config.second);
                        layer_desc = layer.get_descriptor();

                        // add the layer to storage
                        layer_storage.put_layer(layer_desc, layer_desc.id);
                        ss.str(""); ss << "Layer added: ID = " << layer_desc.id << ", Name = " << layer_desc.name << std::endl;
                        LOG(ss.str(), LogLevel::DEBUG);

                        if (layer.has_formulation() && layer.get_domain() == "catchments") {
                            domain_formulations.emplace(
                                layer_desc.id,
                                construct_formulation_from_config(
                                    simulation_time_config,
                                    "layer-" + std::to_string(layer_desc.id),
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

                 // Read routing configurations from configuration file
                auto possible_routing_configs = tree.get_child_optional("routing");
                
                if (possible_routing_configs) {
                    // Since it is possible to build NGEN without routing support, if we see it in the config
                    // but it isn't enabled in the build, we should at least put up a warning
                #if NGEN_WITH_ROUTING
                    this->routing_config = (config::Routing(*possible_routing_configs)).params;
                    using_routing = true;
                #else
                    using_routing = false;
                    ss.str("");
                    ss <<"Formulation Manager found routing configuration"
                       << ", but routing support isn't enabled. No routing will occur." << std::endl;
                    LOG(ss.str(), LogLevel::WARNING);
                #endif // NGEN_WITH_ROUTING
                 }

                std::unordered_map<std::string, realization::config::Formulation> formulation_groups;
                auto possible_formulation_groups = tree.get_child_optional("formulation_groups");
                if (possible_formulation_groups) {
                    for (std::pair<std::string, boost::property_tree::ptree> formulation_config : *possible_formulation_groups) {
                        std::cout << formulation_config.first.c_str() << std::endl;
                        realization::config::Formulation formulation(formulation_config.second.get_child(".")); // "." for first element in list
                        formulation_groups[formulation_config.first] = formulation;
                    }
                }

                std::unordered_map<std::string, realization::config::Forcing> forcing_groups;
                auto possible_forcing_groups = tree.get_child_optional("forcing_groups");
                if (possible_forcing_groups) {
                    for (std::pair<std::string, boost::property_tree::ptree> forcing_config : *possible_forcing_groups) {
                        realization::config::Forcing forcing(forcing_config.second);
                        forcing_groups[forcing_config.first] = forcing;
                    }
                }

                /**
                 * Read catchment configurations from configuration file
                 */      
                auto possible_catchment_configs = tree.get_child_optional("catchments");

                if (possible_catchment_configs) {
                    for (std::pair<std::string, boost::property_tree::ptree> catchment_config : *possible_catchment_configs) {
                        ss.str(""); ss << "Processing catchment: " << catchment_config.first << std::endl;
                        LOG(ss.str(), LogLevel::DEBUG);

                        int catchment_index = fabric->find(catchment_config.first);
                        if (catchment_index == -1) {
                            #ifndef NGEN_QUIET
                                ss.str("");
                                ss << "Formulation_Manager::read: Cannot create formulation for catchment "
                                   << catchment_config.first
                                   << " that isn't identified in the hydrofabric or requested subset" << std::endl;
                                LOG(ss.str(), LogLevel::WARNING);
                            #endif
                            continue;
                        }
                        realization::config::Config catchment_formulation(catchment_config.second, formulation_groups, forcing_groups);

                        if (!catchment_formulation.has_formulation()) {
                            std::string throw_msg;
                            throw_msg.assign("ERROR: No formulations defined for " + catchment_config.first + ".");
                            LOG(throw_msg, LogLevel::WARNING);
                            throw std::runtime_error(throw_msg);
                        }

                        // Parse catchment-specific model_params
                        auto catchment_feature = fabric->get_feature(catchment_index);
                        ss.str(""); ss << "Linking external properties for catchment: " << catchment_config.first << std::endl;
                        LOG(ss.str(), LogLevel::DEBUG);
                        catchment_formulation.formulation.link_external(catchment_feature);

                        this->add_formulation(
                            this->construct_formulation_from_config(
                                simulation_time_config,
                                catchment_config.first,
                                catchment_formulation,
                                output_stream
                            )
                        );
                        ss.str(""); ss << "Formulation constructed for catchment: " << catchment_config.first << std::endl;
                        LOG(ss.str(), LogLevel::DEBUG);
                    }
                }

                // Process any catchments not explicitly defined in the realization file
                for (geojson::Feature location : *fabric) {
                    if (not this->contains(location->get_id())) {
                        ss.str(""); ss << "Creating missing formulation for location: " << location->get_id() << std::endl;
                        LOG(ss.str(), LogLevel::DEBUG);
                        std::shared_ptr<Catchment_Formulation> missing_formulation = this->construct_missing_formulation(
                            location, output_stream, simulation_time_config);
                        this->add_formulation(missing_formulation);
//                        ss.str(""); ss << "Missing formulation created for location: " << location->get_id() << std::endl;
//                        LOG(ss.str(), LogLevel::DEBUG);
                    }
                }
            }

            void add_formulation(std::shared_ptr<Catchment_Formulation> formulation) {
                this->formulations.emplace(formulation->get_id(), formulation);
            }

            std::shared_ptr<Catchment_Formulation> get_formulation(std::string id) const {
                return this->formulations.at(id);
            }

            std::shared_ptr<Catchment_Formulation> get_domain_formulation(long id) const {
                return this->domain_formulations.at(id);
            }

            bool has_domain_formulation(int id) const {
                return this->domain_formulations.count(id) > 0;
            }

            bool contains(std::string identifier) const {
                return this->formulations.count(identifier) > 0;
            }

            /**
             * @return The number of elements within the collection
             */
            int get_size() {
                return this->formulations.size();
            }

            /**
             * @return Whether or not the collection is empty
             */
            bool is_empty() {
                return this->formulations.empty();
            }

            typename std::map<std::string, std::shared_ptr<Catchment_Formulation>>::const_iterator begin() const {
                return this->formulations.cbegin();
            }

            typename std::map<std::string, std::shared_ptr<Catchment_Formulation>>::const_iterator end() const {
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
                std::stringstream ss;
                ss.str(""); ss << "Retrieving t_route config file path" << std::endl;
                LOG(ss.str(), LogLevel::DEBUG);
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
                std::stringstream ss;
                ss.str(""); ss << "Finalizing Formulation_Manager" << std::endl;
                LOG(ss.str(), LogLevel::DEBUG);
                for (auto const& fmap: formulations) {
                    fmap.second->finalize();
                }
                for (auto const& fmap: domain_formulations) {
                    fmap.second->finalize();
                }

#if NGEN_WITH_NETCDF
                data_access::NetCDFPerFeatureDataProvider::cleanup_shared_providers();
#endif
#if NGEN_WITH_PYTHON
                data_access::detail::ForcingsEngineStorage::instances.clear();
#endif
                ss.str(""); ss << "Formulation_Manager finalized" << std::endl;
                LOG(ss.str(), LogLevel::DEBUG);
            }

            /**
             * @brief Get the formatted output root: check the existence of the output_root directory defined
             * in realization. If true, return the directory name. Otherwise, try to create the directory
             * or throw an error on failure.
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
            std::string get_output_root() const {
                const auto output_root = this->tree.get_optional<std::string>("output_root");
                if (output_root != boost::none && *output_root != "") {
                    // Check if the path ends with a trailing slash,
                    // otherwise add it.
                    std::string str = output_root->back() == '/'
                           ? *output_root
                           : *output_root + "/";

                    const char* dir = str.c_str();

                    //use C++ system function to check if there is a dir match that defined in realization
                    struct stat sb;
                    if (stat(dir, &sb) == 0 && S_ISDIR(sb.st_mode)) {
                        return str;
                    } else {
                        errno = 0;
                        int result = mkdir(dir, 0755);      
                        if (result == 0)
                            return str;
                        else
                            throw std::runtime_error("failed to create directory '" + str + "': " + std::strerror(errno));
                    }
                }
 
                //for case where there is no output_root in the realization file
                return "./";
            }

            /**
             * @brief return the layer storage used for formulations
             * @return a reference to the LayerStorageObject
             */
            ngen::LayerDataStorage& get_layer_metadata() {
                std::stringstream ss;
                ss.str(""); ss << "Retrieving layer metadata" << std::endl;
                LOG(ss.str(), LogLevel::DEBUG);
                return layer_storage;
            }


        protected:
            std::shared_ptr<Catchment_Formulation> construct_formulation_from_config(
                simulation_time_params &simulation_time_config,
                std::string identifier,
                const realization::config::Config& catchment_formulation,
                utils::StreamHandler output_stream
            ) {
                std::stringstream ss;
                ss.str(""); ss << "Entering construct_formulation_from_config for identifier: " << identifier << std::endl;
                LOG(ss.str(), LogLevel::DEBUG);

                // Check if the formulation exists
                if (!formulation_exists(catchment_formulation.formulation.type)) {
                    std::string throw_msg;
                    throw_msg.assign("Catchment " + identifier + " failed initialization: '" +
                                     catchment_formulation.formulation.type + "' is not a valid formulation. Options are: " +
                                     valid_formulation_keys());
                    LOG(throw_msg, LogLevel::WARNING);
                    throw std::runtime_error(throw_msg);
                }

                // Check for missing forcing parameters
                ss.str(""); ss << "Checking forcing parameters for identifier: " << identifier << std::endl;
                LOG(ss.str(), LogLevel::DEBUG);
                if (catchment_formulation.forcing.parameters.empty()) {
                    std::string throw_msg;
                    throw_msg.assign("No forcing definition was found for " + identifier);
                    LOG(throw_msg, LogLevel::FATAL);
                    throw std::runtime_error(throw_msg);
                }

                // Check for missing path
                std::vector<std::string> missing_parameters;
                if (!catchment_formulation.forcing.has_key("path")) {
                    ss.str(""); ss << "Missing path parameter for identifier: " << identifier << std::endl;
                    LOG(ss.str(), LogLevel::DEBUG);
                    missing_parameters.push_back("path");
                }

                // Log missing parameters, if any
                if (!missing_parameters.empty()) {
                    std::string message = "A forcing configuration cannot be created for '" + identifier + "'; the following parameters are missing: ";
                    for (size_t i = 0; i < missing_parameters.size(); ++i) {
                        message += missing_parameters[i];
                        if (i < missing_parameters.size() - 1) {
                            message += ", ";
                        }
                    }
                    
                    std::string throw_msg;
                    throw_msg.assign(message);
                    LOG(throw_msg, LogLevel::WARNING);
                    throw std::runtime_error(throw_msg);
                }

                // Extract forcing parameters
                forcing_params forcing_config = this->get_forcing_params(catchment_formulation.forcing.parameters, identifier, simulation_time_config);
                ss.str("");
                ss << "Forcing parameters extracted for identifier: " << identifier << std::endl;
                ss << "  Forcing path:        " << forcing_config.path << std::endl;
                ss << "  Forcing provider:    " << forcing_config.provider << std::endl;
                ss << "  Forcing init_config: " << forcing_config.init_config << std::endl;
                LOG(ss.str(), LogLevel::DEBUG);

                std::time_t start_t = static_cast<std::time_t>(forcing_config.simulation_start_t);
                std::time_t end_t   = static_cast<std::time_t>(forcing_config.simulation_end_t);

                ss.str("");
                ss << "  Simulation start time: " << std::put_time(std::gmtime(&start_t), "%Y-%m-%d %H:%M:%S UTC")
                          << " (" << forcing_config.simulation_start_t << ")" << std::endl;
                ss << "  Simulation end time:   " << std::put_time(std::gmtime(&end_t), "%Y-%m-%d %H:%M:%S UTC")
                          << " (" << forcing_config.simulation_end_t << ")" << std::endl;
                LOG(ss.str(), LogLevel::DEBUG);

                // Construct formulation
                ss.str(""); ss << "Constructing formulation for type: " << catchment_formulation.formulation.type << std::endl;
                LOG(ss.str(), LogLevel::DEBUG);
                std::shared_ptr<Catchment_Formulation> constructed_formulation = construct_formulation(
                    catchment_formulation.formulation.type, identifier, forcing_config, output_stream
                );
                ss.str(""); ss << "Formulation constructed successfully for identifier: " << identifier << std::endl;
                LOG(ss.str(), LogLevel::DEBUG);

                // Create formulation instance
                ss.str(""); ss << "Calling create_formulation for identifier: " << identifier << std::endl;
                LOG(ss.str(), LogLevel::DEBUG);
                constructed_formulation->create_formulation(catchment_formulation.formulation.parameters);
                ss.str(""); ss << "Formulation creation completed for identifier: " << identifier << std::endl;
                LOG(ss.str(), LogLevel::DEBUG);

                return constructed_formulation;
            }

            std::shared_ptr<Catchment_Formulation> construct_missing_formulation(
                geojson::Feature& feature,
                utils::StreamHandler output_stream,
                simulation_time_params &simulation_time_config
            ) {
                std::stringstream ss;
                const std::string identifier = feature->get_id();
                ss.str(""); ss << "Entering construct_missing_formulation for identifier: " << identifier << std::endl;
                LOG(ss.str(), LogLevel::DEBUG);

                // Extract forcing parameters from the global config
                forcing_params forcing_config = this->get_forcing_params(global_config.forcing.parameters, identifier, simulation_time_config);
                ss.str(""); 
                ss << "Forcing parameters extracted for identifier: " << identifier << std::endl;
                ss << "  Forcing path:        " << forcing_config.path << std::endl;
                ss << "  Forcing provider:    " << forcing_config.provider << std::endl;
                ss << "  Forcing init_config: " << forcing_config.init_config << std::endl;
                LOG(ss.str(), LogLevel::DEBUG);

                std::time_t start_t = static_cast<std::time_t>(forcing_config.simulation_start_t);
                std::time_t end_t   = static_cast<std::time_t>(forcing_config.simulation_end_t);

                ss.str(""); 
                ss << "  Simulation start time: " << std::put_time(std::gmtime(&start_t), "%Y-%m-%d %H:%M:%S UTC")
                          << " (" << forcing_config.simulation_start_t << ")" << std::endl;
                ss << "  Simulation end time:   " << std::put_time(std::gmtime(&end_t), "%Y-%m-%d %H:%M:%S UTC")
                          << " (" << forcing_config.simulation_end_t << ")" << std::endl;
                LOG(ss.str(), LogLevel::DEBUG);

                // Construct the formulation object
                ss.str(""); ss << "Entering construct_formulation for identifier: " << identifier << ", type: " << global_config.formulation.type << std::endl;
                LOG(ss.str(), LogLevel::DEBUG);
                std::shared_ptr<Catchment_Formulation> missing_formulation = construct_formulation(
                    global_config.formulation.type,
                    identifier,
                    forcing_config,
                    output_stream
                );
                ss.str(""); ss << "Formulation object constructed for identifier: " << identifier << std::endl;
                LOG(ss.str(), LogLevel::DEBUG);

                // Make a copy of the global configuration so parameters don't clash when linking to external data
                realization::config::Config global_copy = global_config;

//                // Log parameters before substitution
//                ss.str(""); ss << "Global config parameters (before substitution) for identifier: " << identifier << std::endl;
//                for (auto it = global_copy.formulation.parameters.begin(); it != global_copy.formulation.parameters.end(); ++it) {
//                    const std::string& key = it->first;
//                    const geojson::JSONProperty& value = it->second;
//
//                    if (value.get_type() == geojson::PropertyType::String) {
//                        std::cout << "    " << key << ": " << value.as_string() << std::endl;
//                    } else {
//                        std::cout << "    " << key << ": (non-string value)" << std::endl;
//                    }
//                }

                // Substitute {{id}} in the global formulation
                ss.str(""); ss << "Checking for init_config before substitution for identifier: " << identifier << std::endl;
                LOG(ss.str(), LogLevel::DEBUG);
                auto init_config_it = global_copy.formulation.parameters.find(BMI_REALIZATION_CFG_PARAM_REQ__INIT_CONFIG);
                if (init_config_it != global_copy.formulation.parameters.end()) {
                    const geojson::JSONProperty& init_config_property = init_config_it->second;

                    if (init_config_property.get_type() == geojson::PropertyType::String) {
                        std::string original_value = init_config_property.as_string();
                        if (!original_value.empty()) {
                            ss.str(""); ss << "construct_missing_formulation Performing pattern substitution for key: " << BMI_REALIZATION_CFG_PARAM_REQ__INIT_CONFIG
                                           << ", pattern: {{id}}, replacement: " << identifier << std::endl;
                            LOG(ss.str(), LogLevel::DEBUG);
                            ss.str("");
//                            ss.str(""); ss << "Original value: " << original_value << std::endl;
//                            LOG(ss.str(), LogLevel::DEBUG);

                            Catchment_Formulation::config_pattern_substitution(
                                global_copy.formulation.parameters,
                                BMI_REALIZATION_CFG_PARAM_REQ__INIT_CONFIG,
                                "{{id}}",
                                identifier
                            );
                        } else {
                            ss.str(""); ss << "init_config is present but empty for identifier: " << identifier << std::endl;
                            LOG(ss.str(), LogLevel::WARNING);
                        }
                    } else {
                        ss.str(""); ss << "init_config is present but not a string for identifier: " << identifier << std::endl;
                        LOG(ss.str(), LogLevel::WARNING);
                    }
                } else {
                    ss.str(""); ss << "[WARNING] init_config not present in global configuration for identifier: " << identifier << std::endl;
                    LOG(ss.str(), LogLevel::WARNING);
                }

//                // Log parameters after substitution
//                ss.str(""); ss << "Global config parameters (after substitution) for identifier: " << identifier << std::endl;
//                LOG(ss.str(), LogLevel::DEBUG);
//                for (auto it = global_copy.formulation.parameters.begin(); it != global_copy.formulation.parameters.end(); ++it) {
//                    const std::string& key = it->first;
//                    const geojson::JSONProperty& value = it->second;
//
//                    if (value.get_type() == geojson::PropertyType::String) {
//                        std::cout << "    " << key << ": " << value.as_string() << std::endl;
//                    } else {
//                        std::cout << "    " << key << ": (non-string value)" << std::endl;
//                    }
//                }

                // Link external properties
                ss.str(""); ss << "Linking external properties for identifier: " << identifier << std::endl;
                LOG(ss.str(), LogLevel::DEBUG);
                auto formulation = realization::config::Formulation(global_copy.formulation);
                formulation.link_external(feature);

                // Create the formulation
                ss.str(""); ss << "Creating formulation for identifier: " << identifier << std::endl;
                LOG(ss.str(), LogLevel::DEBUG);
                missing_formulation->create_formulation(formulation.parameters);
//                ss.str(""); ss << "Formulation creation completed for identifier: " << identifier << std::endl;
//                LOG(ss.str(), LogLevel::DEBUG);

                return missing_formulation;
            }

            forcing_params get_forcing_params(const geojson::PropertyMap &forcing_prop_map, std::string identifier, simulation_time_params &simulation_time_config) {
                std::stringstream ss;
                ss.str(""); ss << "Entering get_forcing_params for identifier: " << identifier << std::endl;
                LOG(ss.str(), LogLevel::DEBUG);

                // Extract the required 'path' parameter
                std::string path = "";
                if (forcing_prop_map.count("path") != 0) {
                    path = forcing_prop_map.at("path").as_string();
                    ss.str(""); ss << "  Forcing path: " << path << std::endl;
                    LOG(ss.str(), LogLevel::DEBUG);
                }

                // Extract the required 'provider' parameter
                std::string provider;
                if (forcing_prop_map.count("provider") != 0) {
                    provider = forcing_prop_map.at("provider").as_string();
                    ss.str(""); ss << "  Forcing provider: " << provider << std::endl;
                    LOG(ss.str(), LogLevel::DEBUG);
                }

                // Extract the optional 'init_config' parameter from 'params'
                std::string init_config = "";
                if (forcing_prop_map.count("params") != 0) {
                    const geojson::JSONProperty& params_property = forcing_prop_map.at("params");
                    if (params_property.get_type() == geojson::PropertyType::Object) {
                        const geojson::PropertyMap& params_map = params_property.get_values();
                        if (params_map.count("init_config") != 0) {
                            init_config = params_map.at("init_config").as_string();
                            ss.str(""); ss << "  Forcing init_config: " << init_config << std::endl;
                            LOG(ss.str(), LogLevel::DEBUG);
                        }
                    } else {
                        std::cout << "[WARNING] 'params' is not an object for identifier: " << identifier << std::endl;
                    }
                }

                // If no file pattern is present, return the parameters directly
                if (forcing_prop_map.count("file_pattern") == 0) {
                    return forcing_params(
                        path,
                        provider,
                        simulation_time_config.start_time,
                        simulation_time_config.end_time,
                        init_config
                    );
                }

                // Ensure the 'path' is set for pattern matching
                if (path.empty()) {
                    std::string throw_msg = "Error with NGEN config - 'path' in forcing params must be set to a "
                                            "non-empty parent directory path when 'file_pattern' is used.";
                    LOG(throw_msg, LogLevel::WARNING);
                    throw std::runtime_error(throw_msg);
                }

                // Append a trailing slash to the path if not already present
                if (path.compare(path.size() - 1, 1, "/") != 0) {
                    path += "/";
                }

                // Extract and process the file pattern
                std::string filepattern = forcing_prop_map.at("file_pattern").as_string();
                int id_index = filepattern.find("{{id}}");

                // Replace {{id}} if present
                if (id_index != std::string::npos) {
                    filepattern = filepattern.replace(id_index, sizeof("{{id}}") - 1, identifier);
                }

                // Compile the file pattern as a regex
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

                // Retry on certain error codes
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

                    // Sleep before retrying to avoid a tight loop
                    sleep(2);
                    directory = opendir(path.c_str());
                    errMsg = "Received system error number " + std::to_string(errno);
                }

                // Iterate over directory entries
                if (directory != nullptr) {
                    while ((entry = readdir(directory))) {
                        if (std::regex_match(entry->d_name, pattern)) {
                            // Check for regular files and symlinks
            #ifdef _DIRENT_HAVE_D_TYPE
                            if (entry->d_type == DT_REG || entry->d_type == DT_LNK) {
                                closedir(directory);
                                return forcing_params(
                                    path + entry->d_name,
                                    provider,
                                    simulation_time_config.start_time,
                                    simulation_time_config.end_time,
                                    init_config
                                );
                            }
                            else if (entry->d_type == DT_UNKNOWN) {
            #endif
                                // Use stat for systems that don't set d_type
                                struct stat st;
                                if (stat((path + entry->d_name).c_str(), &st) != 0) {
                                    std::string throw_msg = "Could not stat file " + path + entry->d_name;
                                    LOG(throw_msg, LogLevel::WARNING);
                                    throw std::runtime_error(throw_msg);
                                }

                                if (S_ISREG(st.st_mode)) {
                                    closedir(directory);
                                    return forcing_params(
                                        path + entry->d_name,
                                        provider,
                                        simulation_time_config.start_time,
                                        simulation_time_config.end_time,
                                        init_config
                                    );
                                }

                                // Log a warning if the entry is not a regular file
                                std::string throw_msg = "Forcing data in path " + path + entry->d_name + " is not a file";
                                LOG(throw_msg, LogLevel::WARNING);
                                throw std::runtime_error(throw_msg);
            #ifdef _DIRENT_HAVE_D_TYPE
                            }
            #endif
                        }
                    }
                    closedir(directory);
                } else {
                    // The directory wasn't found or otherwise couldn't be opened; forcing data cannot be retrieved
                    std::string throw_msg = "Error opening forcing data dir '" + path + "' after " + std::to_string(attemptCount) + " attempts: " + errMsg;
                    LOG(throw_msg, LogLevel::WARNING);
                    throw std::runtime_error(throw_msg);
                }

                // If no match was found, throw an error
                std::string throw_msg = "Forcing data could not be found for '" + identifier + "'";
                LOG(throw_msg, LogLevel::WARNING);
                throw std::runtime_error(throw_msg);
            }

            /**
             * @brief Parse a `model_params` property tree and replace external parameters
             *        with values from a catchment's properties
             * 
             * @param model_params Property tree with root key "model_params"
             * @param catchment_feature Associated catchment feature
             */
            void parse_external_model_params(boost::property_tree::ptree& model_params, const geojson::Feature catchment_feature) {
                std::stringstream ss;
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
                                ss.str("");
                                ss  << "property type " << static_cast<int>(catchment_attribute.get_type()) << " not allowed as model parameter. "
                                          << "Must be one of: Natural (int), Real (double), Boolean, or String" << '\n';
                                LOG(ss.str(), LogLevel::WARNING); ss.str("");
                                break;
                        }
                    } else {
                        ss.str("");
                        ss << " Failed to parse external parameter: catchment `"
                           << catchment_feature->get_id()
                           << "` does not contain the property `"
                           << param_name << "`\n";
                        LOG(ss.str(), LogLevel::WARNING); 
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
                std::stringstream ss;
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
                                ss.str(""); ss << "property type " << static_cast<int>(catchment_attribute.get_type()) << " not allowed as model parameter. "
                                               << "Must be one of: Natural (int), Real (double), Boolean, or String" << '\n';
                                LOG(ss.str(), LogLevel::WARNING);
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
