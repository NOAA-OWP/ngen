#ifndef FORMULATION_H
#define FORMULATION_H

#include <memory>
#include <string>
#include <map>
#include <exception>
#include <vector>

#include "JSONProperty.hpp"

#include <boost/property_tree/ptree.hpp>
#include <boost/algorithm/string.hpp>

#define DEFAULT_FORMULATION_OUTPUT_DELIMITER ","

namespace realization {

    class Formulation {
        public:
            typedef long time_step_t;

            Formulation(std::string id) : id(id) {}
            
            virtual ~Formulation(){};

            virtual std::string get_formulation_type() = 0;

            // TODO: to truly make this properly generalized (beyond catchments, and to some degree even in that
            //  context) a more complex type for the entirety of the response/output is needed, perhaps with independent
            //  access functions

            // TODO: a reference to the actual time of the initial time step is needed

            // TODO: a reference to the last calculated time step is needed

            // TODO: a mapping of previously calculated time steps to the size/delta of each is needed (unless we
            //  introduce a way to enforce immutable time step delta values for an object)

            // TODO: a convenience method for getting the actual time of calculated time steps (at least the last)

            /**
             * Execute the backing model formulation for the given time step, where it is of the specified size, and
             * return the response output.
             *
             * Any inputs and additional parameters must be made available as instance members.
             *
             * Types should clearly document the details of their particular response output.
             *
             * @param t_index The index of the time step for which to run model calculations.
             * @param d_delta_s The duration, in seconds, of the time step for which to run model calculations.
             * @return The response output of the model for this time step.
             */
            virtual double get_response(time_step_t t_index, time_step_t d_delta_s) = 0;

            // TODO: look at adding another overloaded function that uses instance members to get the index and delta

            // The neccessity of this function is in question
            virtual void add_time(time_t t, double n){};

            std::string get_id() const {
                return this->id;
            }

            /**
             * Get a header line appropriate for a file made up of entries from this type's implementation of
             * ``get_output_line_for_timestep``.
             *
             * Note that like the output generating function, this line does not include anything for time step.
             *
             * @return An appropriate header line for this type.
             */
            virtual std::string get_output_header_line(std::string delimiter=DEFAULT_FORMULATION_OUTPUT_DELIMITER) =0;

            virtual std::vector<int> get_output_bbox_list() = 0;
            //virtual std::string get_output_bbox_list(std::string delimiter=DEFAULT_FORMULATION_OUTPUT_DELIMITER) =0;

            /**
             * Get a formatted line of output values for the given time step as a delimited string.
             *
             * This method is useful for preparing calculated data in a representation useful for output files, such as
             * CSV files.
             *
             * The resulting string will contain calculated values for applicable output variables for the particular
             * formulation, as determined for the given time step.  However, the string will not contain any
             * representation of the time step itself.
             *
             * An empty string is returned if the time step value is not in the range of valid time steps for which there
             * are calculated values for all variables.
             *
             * @param timestep The time step for which data is desired.
             * @param delimiter The value delimiter for the string.
             * @return A delimited string with all the output variable values for the given time step.
             */
            virtual std::string get_output_line_for_timestep(int timestep,
                                                             std::string delimiter = DEFAULT_FORMULATION_OUTPUT_DELIMITER) = 0;
            
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
