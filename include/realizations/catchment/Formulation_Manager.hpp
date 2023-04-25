#ifndef NGEN_FORMULATION_MANAGER_H
#define NGEN_FORMULATION_MANAGER_H

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
#include <FeatureCollection.hpp>
#include "Formulation_Constructors.hpp"
#include "Simulation_Time.h"
#include "GIUH.hpp"
#include "GiuhJsonReader.h"
#include "routing/Routing_Params.h"

namespace realization {

    class Formulation_Manager {
        typedef std::tuple<std::string, std::string> dual_keys;

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

            virtual ~Formulation_Manager(){
#ifdef NETCDF_ACTIVE
                data_access::NetCDFPerFeatureDataProvider::cleanup_shared_providers();
#endif
            };

            virtual void read(geojson::GeoJSON fabric, utils::StreamHandler output_stream) {
                //TODO seperate the parsing of configuration options like time
                //and routing and other non feature specific tasks from this main function
                //which has to iterate the entire hydrofabric.
                auto possible_global_config = tree.get_child_optional("global");

                if (possible_global_config) {
                    this->global_formulation_tree = *possible_global_config;

                    //get forcing info
                    for (auto &forcing_parameter : (*possible_global_config).get_child("forcing")) {
                        this->global_forcing.emplace(
                            forcing_parameter.first,
                            geojson::JSONProperty(forcing_parameter.first, forcing_parameter.second)
                        );
                      }

                    //get first empty key under formulations (corresponds to first json array element)
                    auto formulation = (*possible_global_config).get_child("formulations..");
                    for (std::pair<std::string, boost::property_tree::ptree> global_setting : formulation.get_child("params")) {
                        this->global_formulation_parameters.emplace(
                            global_setting.first,
                            geojson::JSONProperty(global_setting.first, global_setting.second)
                        );
                    }
                }

                /**
                 * Read simulation time from configuration file
                 * /// \todo TODO: Separate input_interval from output_interval
                 */            
                auto possible_simulation_time = tree.get_child_optional("time");

                if (!possible_simulation_time) {
                    throw std::runtime_error("ERROR: No simulation time period defined.");
                }

                geojson::JSONProperty simulation_time_parameters("time", *possible_simulation_time);

                std::vector<std::string> missing_simulation_time_parameters;

                if (!simulation_time_parameters.has_key("start_time")) {
                    missing_simulation_time_parameters.push_back("start_time");
                }

                if (!simulation_time_parameters.has_key("end_time")) {
                    missing_simulation_time_parameters.push_back("end_time");
                }

                if (!simulation_time_parameters.has_key("output_interval")) {
                    missing_simulation_time_parameters.push_back("output_interval");
                }

                if (missing_simulation_time_parameters.size() > 0) {
                    std::string message = "ERROR: A simulation time parameter cannot be created; the following parameters are missing: ";

                    for (int missing_parameter_index = 0; missing_parameter_index < missing_simulation_time_parameters.size(); missing_parameter_index++) {
                        message += missing_simulation_time_parameters[missing_parameter_index];

                        if (missing_parameter_index < missing_simulation_time_parameters.size() - 1) {
                            message += ", ";
                        }
                    }
                    
                    throw std::runtime_error(message);
                }

                simulation_time_params simulation_time_config(
                    simulation_time_parameters.at("start_time").as_string(),
                    simulation_time_parameters.at("end_time").as_string(),
                    simulation_time_parameters.at("output_interval").as_natural_number()
                );

                /**
                 * Call constructor to construct a Simulation_Time object
                 */ 
                this->Simulation_Time_Object = std::make_shared<Simulation_Time>(simulation_time_config);

                /**
                 * Read routing configurations from configuration file
                 */      
                auto possible_routing_configs = tree.get_child_optional("routing");
                
                if (possible_routing_configs) {
                    //Since it is possible to build NGEN without routing support, if we see it in the config
                    //but it isn't enabled in the build, we should at least put up a warning
                #ifdef NGEN_ROUTING_ACTIVE
                    geojson::JSONProperty routing_parameters("routing", *possible_routing_configs);
                    
                    this->routing_config = std::make_shared<routing_params>(
                        routing_parameters.at("t_route_config_file_with_path").as_string()
                    );
                    using_routing = true;
                #else
                    using_routing = false;
                    std::cerr<<"WARNING: Formulation Manager found routing configuration"
                             <<", but routing support isn't enabled. No routing will occur."<<std::endl;
                #endif //NGEN_ROUTING_ACTIVE
                 }

                /**
                 * Read catchment configurations from configuration file
                 */      
                auto possible_catchment_configs = tree.get_child_optional("catchments");

                if (possible_catchment_configs) {
                    for (std::pair<std::string, boost::property_tree::ptree> catchment_config : *possible_catchment_configs) {
                      if( fabric->find(catchment_config.first) == -1 )
                      {
                        #ifndef NGEN_QUIET
                        std::cerr<<"WARNING Formulation_Manager::read: Cannot create formulation for catchment "
                                 <<catchment_config.first
                                 <<" that isn't identified in the hydrofabric or requested subset"<<std::endl;
                        #endif
                        continue;
                      }
                      auto formulations = catchment_config.second.get_child_optional("formulations");
                      if( !formulations ) {
                        throw std::runtime_error("ERROR: No formulations defined for "+catchment_config.first+".");
                      }

                      for (const auto &formulation: *formulations) {
                          this->add_formulation(
                              this->construct_formulation_from_tree(
                                  simulation_time_config,
                                  catchment_config.first,
                                  catchment_config.second,
                                  formulation.second,
                                  output_stream
                              )
                          );
                          break; //only construct one for now FIXME
                        } //end for formulaitons
                      }//end for catchments


                }//end if possible_catchment_configs

                for (geojson::Feature location : *fabric) {
                    if (not this->contains(location->get_id())) {
                        std::shared_ptr<Catchment_Formulation> missing_formulation = this->construct_missing_formulation(
                          location->get_id(), output_stream, simulation_time_config);
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


        protected:
            std::shared_ptr<Catchment_Formulation> construct_formulation_from_tree(
                simulation_time_params &simulation_time_config,
                std::string identifier,
                boost::property_tree::ptree &tree,
                const boost::property_tree::ptree &formulation,
                utils::StreamHandler output_stream
            ) {
                auto params = formulation.get_child("params");
                std::string formulation_type_key;
                try {
                    formulation_type_key = get_formulation_key(formulation);
                }
                catch(std::exception& e) {
                    throw std::runtime_error("Catchment " + identifier + " failed initialization: " + e.what());
                }

                boost::property_tree::ptree formulation_config = formulation.get_child("params");

                auto possible_forcing = tree.get_child_optional("forcing");

                if (!possible_forcing) {
                    throw std::runtime_error("No forcing definition was found for " + identifier);
                }

                geojson::JSONProperty forcing_parameters("forcing", *possible_forcing);

                std::vector<std::string> missing_parameters;
                
                if (!forcing_parameters.has_key("path")) {
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

                geojson::PropertyMap local_forcing;
                for (auto &forcing_parameter : *possible_forcing) {
                    local_forcing.emplace(
                        forcing_parameter.first,
                        geojson::JSONProperty(forcing_parameter.first, forcing_parameter.second)
                    );
                }

                forcing_params forcing_config = this->get_forcing_params(local_forcing, identifier, simulation_time_config);

                std::shared_ptr<Catchment_Formulation> constructed_formulation = construct_formulation(formulation_type_key, identifier, forcing_config, output_stream);
                //, geometry);
                constructed_formulation->create_formulation(formulation_config, &global_formulation_parameters);
                return constructed_formulation;
            }

            std::shared_ptr<Catchment_Formulation> construct_missing_formulation(std::string identifier, utils::StreamHandler output_stream, simulation_time_params &simulation_time_config){
                std::string formulation_type_key = get_formulation_key(global_formulation_tree.get_child("formulations.."));

                forcing_params forcing_config = this->get_forcing_params(this->global_forcing, identifier, simulation_time_config);

                std::shared_ptr<Catchment_Formulation> missing_formulation = construct_formulation(formulation_type_key, identifier, forcing_config, output_stream);
                // Need to work with a copy, since it is altered in-place
                geojson::PropertyMap global_properties_copy = global_formulation_parameters;
                Catchment_Formulation::config_pattern_substitution(global_properties_copy,
                                                                   BMI_REALIZATION_CFG_PARAM_REQ__INIT_CONFIG, "{{id}}",
                                                                   identifier);
                missing_formulation->create_formulation(global_properties_copy);
                return missing_formulation;
            }

            forcing_params get_forcing_params(geojson::PropertyMap &forcing_prop_map, std::string identifier, simulation_time_params &simulation_time_config) {
                std::string path = forcing_prop_map.at("path").as_string();
                std::string provider = "";
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

            boost::property_tree::ptree tree;

            boost::property_tree::ptree global_formulation_tree;

            geojson::PropertyMap global_formulation_parameters;

            geojson::PropertyMap global_forcing;

            std::map<std::string, std::shared_ptr<Catchment_Formulation>> formulations;

            std::shared_ptr<routing_params> routing_config;

            bool using_routing = false;
    };
}
#endif // NGEN_FORMULATION_MANAGER_H
