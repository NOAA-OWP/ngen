#ifndef NGEN_FORMULATION_MANAGER_H
#define NGEN_FORMULATION_MANAGER_H

#include <memory>
#include <sstream>
#include <tuple>
#include <functional>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "Formulation_Constructors.hpp"

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

            virtual void read(utils::StreamHandler output_stream) {
                auto possible_global_config = tree.get_child_optional("global");

                if (possible_global_config) {
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
                            this->global_formulation_tree = node.second;

                            for (std::pair<std::string, boost::property_tree::ptree> global_setting : node.second) {
                                this->global_formulation_parameters.emplace(
                                    global_setting.first,
                                    geojson::JSONProperty(global_setting.first, global_setting.second)
                                );
                            }
                        }
                    }
                }

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

            boost::property_tree::ptree tree;

            boost::property_tree::ptree global_formulation_tree;

            geojson::PropertyMap global_formulation_parameters;

            geojson::PropertyMap global_forcing;

            std::map<std::string, std::shared_ptr<Formulation>> formulations;
    };
}
#endif // NGEN_FORMULATION_MANAGER_H
