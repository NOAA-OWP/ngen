#ifndef NGEN_FORMULATION_MANAGER_H
#define NGEN_FORMULATION_MANAGER_H

#include <memory>
#include <sstream>
#include <tuple>
#include <functional>
#include <dirent.h>
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

namespace realization {

    class Formulation_Manager {
        typedef std::tuple<std::string, std::string> dual_keys;

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

            virtual ~Formulation_Manager(){};

            virtual void read(geojson::GeoJSON fabric, utils::StreamHandler output_stream) {
                auto possible_global_config = tree.get_child_optional("global");

                if (possible_global_config) {
                    this->global_formulation_tree = *possible_global_config;
                    std::string formulation_key = get_formulation_key(*possible_global_config);
                    for(std::pair<std::string, boost::property_tree::ptree> node : *possible_global_config) {
                        if (node.first == "forcing") {
                            for (auto &forcing_parameter : node.second) {
                                this->global_forcing.emplace(
                                    forcing_parameter.first,
                                    geojson::JSONProperty(forcing_parameter.first, forcing_parameter.second)
                                );
                            }
                        }
                        else if (node.first == formulation_key) {
                            for (std::pair<std::string, boost::property_tree::ptree> global_setting : node.second) {
                                this->global_formulation_parameters.emplace(
                                    global_setting.first,
                                    geojson::JSONProperty(global_setting.first, global_setting.second)
                                );
                            }
                        }
                    }
                }

                auto possible_simulation_time = tree.get_child_optional("time");

                if (!possible_simulation_time) {
                    throw std::runtime_error("ERROR: No simulation time period defined.";
                }

                geojson::JSONProperty simulation_time_parameters("time", *possible_simulation_time);

                std::vector<std::string> missing_simulation_time_parameters;

                if (!simulation_time_parameters.has_key("start_time")) {
                    missing_simulation_time_parameters.push_back("start_time");
                }

                if (!simulation_time_parameters.has_key("end_time")) {
                    missing_simulation_time_parameters.push_back("end_time");
                }

                if (missing_simulation_time_parameters.size() > 0) {
                    std::string message = "ERROR: A simulation time parameter cannot be created; the following parameters are missing: ";

                    for (int missing_parameter_index = 0; missing_parameter_index < missing_parameters.size(); missing_parameter_index++) {
                        message += missing_parameters[missing_parameter_index];

                        if (missing_parameter_index < missing_parameters.size() - 1) {
                            message += ", ";
                        }
                    }
                    
                    throw std::runtime_error(message);
                }

                simulation_time_params forcing_config(
                    simulation_time_parameters.at("start_time").as_string(),
                    simulation_time_parameters.at("end_time").as_string()
                );

                auto possible_catchment_configs = tree.get_child_optional("catchments");

                if (possible_catchment_configs) {
                    for (std::pair<std::string, boost::property_tree::ptree> catchment_config : *possible_catchment_configs) {
                        this->add_formulation(
                            this->construct_formulation_from_tree(
                                catchment_config.first,
                                catchment_config.second,
                                output_stream
                            )
                        );
                    }
                }

                for (geojson::Feature location : *fabric) {
                    if (not this->contains(location->get_id())) {
                        std::shared_ptr<Formulation> missing_formulation = this->construct_missing_formulation(location->get_id(), output_stream);
                        this->add_formulation(missing_formulation);
                    }
                }
            }

            virtual void add_formulation(std::shared_ptr<Formulation> formulation) {
                this->formulations.emplace(formulation->get_id(), formulation);
            }

            virtual std::shared_ptr<Formulation> get_formulation(std::string id) const {
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

            virtual typename std::map<std::string, std::shared_ptr<Formulation>>::const_iterator begin() const {
                return this->formulations.cbegin();
            }

            virtual typename std::map<std::string, std::shared_ptr<Formulation>>::const_iterator end() const {
                return this->formulations.cend();
            }

        protected:
            std::shared_ptr<Formulation> construct_formulation_from_tree(
                std::string identifier,
                boost::property_tree::ptree &tree,
                utils::StreamHandler output_stream
            ) {
                std::string formulation_type_key = get_formulation_key(tree);

                boost::property_tree::ptree formulation_config = tree.get_child(formulation_type_key);

                auto possible_forcing = tree.get_child_optional("forcing");

                if (!possible_forcing) {
                    throw std::runtime_error("No forcing definition was found for " + identifier);
                }

                geojson::JSONProperty forcing_parameters("forcing", *possible_forcing);

                std::vector<std::string> missing_parameters;
                
                if (!forcing_parameters.has_key("path")) {
                    missing_parameters.push_back("path");
                }

                if (!forcing_parameters.has_key("start_time")) {
                    missing_parameters.push_back("start_time");
                }

                if (!forcing_parameters.has_key("end_time")) {
                    missing_parameters.push_back("end_time");
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

                forcing_params forcing_config(
                    forcing_parameters.at("path").as_string(),
                    forcing_parameters.at("start_time").as_string(),
                    forcing_parameters.at("end_time").as_string()
                );

                std::shared_ptr<Formulation> constructed_formulation = construct_formulation(formulation_type_key, identifier, forcing_config, output_stream);
                constructed_formulation->create_formulation(formulation_config, &global_formulation_parameters);
                return constructed_formulation;
            }

            std::shared_ptr<Formulation> construct_missing_formulation(std::string identifier, utils::StreamHandler output_stream) {
                std::string formulation_type_key = get_formulation_key(global_formulation_tree);

                forcing_params forcing_config = this->get_global_forcing_params(identifier);

                std::shared_ptr<Formulation> missing_formulation = construct_formulation(formulation_type_key, identifier, forcing_config, output_stream);
                missing_formulation->create_formulation(this->global_formulation_parameters);
                return missing_formulation;
            }

            forcing_params get_global_forcing_params(std::string identifier) {
                std::string path = this->global_forcing.at("path").as_string();
                
                if (this->global_forcing.count("file_pattern") == 0) {
                    return forcing_params(
                        path,
                        this->global_forcing.at("start_time").as_string(),
                        this->global_forcing.at("end_time").as_string()
                    );
                }

                // Since we are given a pattern, we need to identify the directory and pull out anything that matches the pattern
                if (path.compare(path.size() - 1, 1, "/") != 0) {
                    path += "/";
                }

                std::string filepattern = this->global_forcing.at("file_pattern").as_string();

                int id_index = filepattern.find("{{ID}}");

                // If an index for '{{ID}}' was found, we can count on that being where the id for this realization can be found.
                //     For instance, if we have a pattern of '.*{{ID}}_14_15.csv' and this is named 'cat-87',
                //     this will match on 'stuff_example_cat-87_14_15.csv'
                if (id_index != std::string::npos) {
                    filepattern = filepattern.replace(id_index, sizeof("{{ID}}") - 1, identifier);
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

                // If the directory could be found, we can go ahead and iterate
                if (directory != nullptr) {
                    while ((entry = readdir(directory))) {
                        // If the entry is a regular file or symlink AND the name matches the pattern, 
                        //    we can consider this ready to be interpretted as valid forcing data (even if it isn't)
                        if ((entry->d_type == DT_REG or entry->d_type == DT_LNK) and std::regex_match(entry->d_name, pattern)) {
                            return forcing_params(
                                path + entry->d_name,
                                this->global_forcing.at("start_time").as_string(),
                                this->global_forcing.at("end_time").as_string()
                            );
                        }
                    }
                }
                else {
                    // The directory wasn't found; forcing data cannot be retrieved
                    throw std::runtime_error("No directory for forcing data was found at: " + path);
                }

                closedir(directory);

                throw std::runtime_error("Forcing data could not be found for '" + identifier + "'");
            }

            boost::property_tree::ptree tree;

            boost::property_tree::ptree global_formulation_tree;

            geojson::PropertyMap global_formulation_parameters;

            geojson::PropertyMap global_forcing;

            std::map<std::string, std::shared_ptr<Formulation>> formulations;
    };
}
#endif // NGEN_FORMULATION_MANAGER_H
