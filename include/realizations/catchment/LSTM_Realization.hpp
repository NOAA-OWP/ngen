#ifndef NGEN_LSTM_REALIZATION_HPP
#define NGEN_LSTM_REALIZATION_HPP

#include "Catchment_Formulation.hpp"
#include <unordered_map>
#include "lstm/include/LSTM.h"
#include "lstm/include/lstm_params.h"
#include "lstm/include/lstm_config.h"
#include <memory>

namespace realization {

    class LSTM_Realization : public Catchment_Formulation {

    public:

        typedef long time_step_t;

        LSTM_Realization(forcing_params forcing_config,
                           utils::StreamHandler output_stream,
                           std::string catchment_id,
                           lstm::lstm_params params,
                           lstm::lstm_config config);


        LSTM_Realization(
                forcing_params forcing_config,
                utils::StreamHandler output_stream,
                std::string catchment_id,
                std::string pytorch_model_path,
                std::string normalization_path,
                std::string initial_state_path,
                double latitude,
                double longitude,
                double area_square_km
                );

            LSTM_Realization(
                std::string id,
                forcing_params forcing_config,
                utils::StreamHandler output_stream
            ) : Catchment_Formulation(id, forcing_config, output_stream) {}

            virtual ~LSTM_Realization(){};

            /**
             * Execute the backing model formulation for the given time step, where it is of the specified size, and
             * return the total discharge.
             *
             * Function reads input precipitation from ``forcing`` member variable.  It also makes use of the params struct
             * for ET params accessible via ``get_et_params``.
             *
             * @param t_index The index of the time step for which to run model calculations.
             * @param d_delta_s The duration, in seconds, of the time step for which to run model calculations.
             * @return The total discharge for this time step.
             */
            double get_response(time_step_t t_index, time_step_t t_delta_s) override;

            double calc_et(double soil_m) override {return 0.0;}


            std::string get_formulation_type() override {
                return "lstm";
            }


            /**
             * Get a formatted line of output values for the given time step as a delimited string.
             *
             * For this type, the output consists of only the total discharge amount per time step; i.e., the same value
             * that was returned by ``get_response``.
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
             * The default delimiter is a comma.
             *
             * @param timestep The time step for which data is desired.
             * @return A delimited string with all the output variable values for the given time step.
             */
            std::string get_output_line_for_timestep(int timestep, std::string delimiter=",") override;

            void create_formulation(boost::property_tree::ptree &config, geojson::PropertyMap *global = nullptr) override;

            void create_formulation(geojson::PropertyMap properties) override;

            const std::vector<std::string>& get_required_parameters() override {
                return REQUIRED_PARAMETERS;
            }


        private:
            std::string catchment_id;
            shared_ptr<lstm::lstm_state> state;
            shared_ptr<lstm::lstm_fluxes> fluxes;
            lstm::lstm_params params;
            lstm::lstm_config config;
            std::unique_ptr<lstm::lstm_model> model;

            std::vector<std::string> REQUIRED_PARAMETERS = {
                 "pytorch_model_path",
                 "normalization_path",
                 "initial_state_path",
                 "latitude",
                 "longitude",
                 "area_square_km",
            };

    };

}

#endif //NGEN_LSTM_REALIZATION_HPP
