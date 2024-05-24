#ifndef NGEN_REALIZATION_CONFIG_TIME_H
#define NGEN_REALIZATION_CONFIG_TIME_H

#include <boost/property_tree/ptree.hpp>
#include <string>

#include "Simulation_Time.hpp"

namespace realization{
  namespace config{
    /**
     * Simple structure for holding time bounds and output interval (seconds)
    */
    struct Time{
        std::string start_time;
        std::string end_time;
        unsigned int output_interval;
        
        /**
         * @brief Construct a new Time object from a boost property tree
         * 
         * The tree should have the following keys, and if not the given defaults
         * are applied
         * start_time (default "")
         * end_time (default "")
         * output_interval (default 0)
         * 
         * @param tree boost property tree to construct Time from
         */
        Time(const boost::property_tree::ptree& tree){
                start_time = tree.get("start_time", std::string());
                end_time = tree.get("end_time", std::string());
                output_interval = tree.get("output_interval", 0);
        }

        /**
         * @brief Converts string based parameters start_time and end_time
         * into time structs for use as simulation_time_params
         * 
         * @return simulation_time_params 
         */
        simulation_time_params make_params(){
            std::vector<std::string> missing_simulation_time_parameters;

            if (start_time.empty()){
                missing_simulation_time_parameters.push_back("start_time");
            }

            if (end_time.empty()) {
                missing_simulation_time_parameters.push_back("end_time");
            }

            if (output_interval == 0) {
                missing_simulation_time_parameters.push_back("output_interval");
            }

            if (missing_simulation_time_parameters.size() > 0) {
                std::string message = "ERROR: A simulation time parameter cannot be created; the following parameters are missing or invalid: ";

                for (int missing_parameter_index = 0; missing_parameter_index < missing_simulation_time_parameters.size(); missing_parameter_index++) {
                    message += missing_simulation_time_parameters[missing_parameter_index];

                    if (missing_parameter_index < missing_simulation_time_parameters.size() - 1) {
                        message += ", ";
                    }
                }
                
                throw std::runtime_error(message);
            }
            return simulation_time_params(
                        start_time,
                        end_time,
                        output_interval
                    );
        }
    };
  }//end namespace config
}//end namespace realization
#endif //NGEN_REALIZATION_CONFIG_TIME_H
