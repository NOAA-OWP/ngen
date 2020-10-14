#ifndef FORMULATION_H
#define FORMULATION_H

#include <memory>
#include <string>
#include <map>
#include <exception>
#include <vector>

#include "JSONProperty.hpp"
#include "Pdm03.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/algorithm/string.hpp>

namespace realization {

    class Formulation {
        public:
            typedef long time_step_t;

            Formulation(std::string id) : id(id) {}
            
            virtual ~Formulation(){};

            virtual std::string get_formulation_type() = 0;

            virtual double get_response(double input_flux, time_step_t t, time_step_t dt, void* et_params) = 0;

            // The neccessity of this function is in question
            virtual void add_time(time_t t, double n){};

            std::string get_id() const {
                return this->id;
            }
            
            virtual void create_formulation(boost::property_tree::ptree &config, geojson::PropertyMap *global = nullptr) = 0;
            virtual void create_formulation(geojson::PropertyMap properties) = 0;

        protected:

            virtual const std::vector<std::string>& get_required_parameters() = 0;

            virtual geojson::PropertyMap interpret_parameters(boost::property_tree::ptree &config, geojson::PropertyMap *global = nullptr) {
                geojson::PropertyMap options;

                for (auto &formulation_parameter : config) {
                    options.emplace(formulation_parameter.first, geojson::JSONProperty(formulation_parameter.first, formulation_parameter.second));
                }

                if (global != nullptr) {
                    for(auto &global_option : *global) {
                        if (options.count(global_option.first) == 0) {
                            options.emplace(global_option.first, global_option.second);
                        }
                    }
                }

                validate_parameters(options);

                return options;
            }

            virtual void validate_parameters(geojson::PropertyMap options) {
                std::vector<std::string> missing_parameters;
                std::vector<std::string> required_parameters = get_required_parameters();

                for (auto parameter : required_parameters) {
                  if (options.count(parameter) == 0) {
                        missing_parameters.push_back(parameter);
                    }
                }

                if (missing_parameters.size() > 0) {
                    std::string message = "A " + get_formulation_type() + " formulation cannot be created; the following parameters are missing: ";

                    for (int missing_parameter_index = 0; missing_parameter_index < missing_parameters.size(); missing_parameter_index++) {
                        message += missing_parameters[missing_parameter_index];

                        if (missing_parameter_index < missing_parameters.size() - 1) {
                            message += ", ";
                        }
                    }
                    
                    throw std::runtime_error(message);
                }
            }

            std::string id;
    };

}
#endif // FORMULATION_H
