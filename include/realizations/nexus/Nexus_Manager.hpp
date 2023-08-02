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

namespace nexus {

    class Nexus_Manager {
        //typedef std::tuple<std::string, std::string> dual_keys;

        public:

            Nexus_Manager(geojson::GeoJSON nexus) {
              for (const auto &nex: *nexus) {
                  this->add_nexus(
                      this->construct_nexus(
                          nex,
                          output_stream
                      )
                  );
            }
          }

            virtual ~Nexus_Manager(){};

            virtual void add_nexus(std::shared_ptr<HY_HydroNexus> formulation) {
                this->nexuses.emplace(formulation->get_id(), formulation);
            }

            virtual std::shared_ptr<Formulation> get_nexus(std::string id) const {
                return this->nexuses.at(id);
            }

            virtual bool contains(std::string identifier) const {
                return this->nexuses.count(identifier) > 0;
            }

            /**
             * @return The number of elements within the collection
             */
            virtual int get_size() {
                return this->nexuses.size();
            }

            /**
             * @return Whether or not the collection is empty
             */
            virtual bool is_empty() {
                return this->nexuses.empty();
            }

            virtual typename std::map<std::string, std::shared_ptr<HY_HydroNexus>>::const_iterator begin() const {
                return this->nexuses.cbegin();
            }

            virtual typename std::map<std::string, std::shared_ptr<HY_HydroNexus>>::const_iterator end() const {
                return this->nexuses.cend();
            }

        protected:
            std::shared_ptr<HY_HydroNexus> construct_nexus(
                geojson::Feature nexus_feature,
                utils::StreamHandler output_stream
            ) {
                auto params = formulation.get_child("params");
                std::string formulation_type_key =  get_formulation_key(formulation);

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

                forcing_params forcing_config(
                    forcing_parameters.at("path").as_string(),
                    simulation_time_config.start_time,
                    simulation_time_config.end_time
                );

                std::shared_ptr<Formulation> constructed_formulation = construct_formulation(formulation_type_key, identifier, forcing_config, output_stream);
                constructed_formulation->create_formulation(formulation_config, &global_formulation_parameters);
                return constructed_formulation;
            }

            std::shared_ptr<Formulation> construct_missing_formulation(std::string identifier, utils::StreamHandler output_stream, simulation_time_params &simulation_time_config) {
                std::string formulation_type_key = get_formulation_key(global_formulation_tree.get_child("formulations.."));

                forcing_params forcing_config = this->get_global_forcing_params(identifier, simulation_time_config);

                std::shared_ptr<Formulation> missing_formulation = construct_formulation(formulation_type_key, identifier, forcing_config, output_stream);
                missing_formulation->create_formulation(this->global_formulation_parameters);
                return missing_formulation;
            }


            std::map<std::string, std::shared_ptr<HY_HydroNexus>> nexuses;
    };
}
#endif // NGEN_FORMULATION_MANAGER_H
