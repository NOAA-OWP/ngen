#ifndef NGEN_LSTM_REALIZATION_HPP
#define NGEN_LSTM_REALIZATION_HPP


#include "Catchment_Formulation.hpp"
#include <unordered_map>
#include "GIUH.hpp"
#include "GiuhJsonReader.h"
#include "lstm/include/LSTM.h"
#include "lstm/include/lstm_params.h"
#include <memory>

namespace realization {

    class LSTM_Realization : public Catchment_Formulation {

    public:

        typedef long time_step_t;

        LSTM_Realization(forcing_params forcing_config,
                           utils::StreamHandler output_stream,
                           double soil_storage_meters,
                           double groundwater_storage_meters,
                           std::string catchment_id,
                           giuh::GiuhJsonReader &giuh_json_reader,
                           lstm::lstm_params params,
                           const std::vector<double>& nash_storage,
                           time_step_t t);


        LSTM_Realization(
                forcing_params forcing_config,
                utils::StreamHandler output_stream,
                double soil_storage_meters,
                double groundwater_storage_meters,
                std::string catchment_id,
                giuh::GiuhJsonReader &giuh_json_reader,
                double maxsmc,
                double wltsmc,
                double satdk,
                double satpsi,
                double slope,
                double b,
                double multiplier,
                double alpha_fc,
                double Klf,
                double Kn,
                int nash_n,
                double Cgw,
                double expon,
                double max_gw_storage,
                const std::vector<double>& nash_storage,
                time_step_t t
                );

            LSTM_Realization(
                std::string id,
                forcing_params forcing_config,
                utils::StreamHandler output_stream
            ) : Catchment_Formulation(id, forcing_config, output_stream) {}




            void set_giuh_kernel(std::shared_ptr<giuh::GiuhJsonReader> reader);

            virtual ~LSTM_Realization(){};

            double calc_et(double soil_m) override;



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



            std::string get_formulation_type() {
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

            void create_formulation(boost::property_tree::ptree &config, geojson::PropertyMap *global = nullptr);

            void create_formulation(geojson::PropertyMap properties);

            const std::vector<std::string>& get_required_parameters() override {
                return REQUIRED_PARAMETERS;
            }


        private:
            std::string catchment_id;
            std::unordered_map<time_step_t, shared_ptr<lstm::lstm_state>> state;
            std::unordered_map<time_step_t, shared_ptr<lstm::lstm_fluxes>> fluxes;
            std::unordered_map<time_step_t, std::vector<double> > cascade_backing_storage;
            lstm::lstm_params *params;
            std::unique_ptr<lstm::lstm_model> model;
            std::shared_ptr<giuh::giuh_kernel> giuh_kernel;

            //The delta time (dt) this instance is configured to use
            time_step_t dt;

            std::vector<std::string> REQUIRED_PARAMETERS = {
                "maxsmc",
                "wltsmc",
                "satdk",
                "satpsi",
                "slope",
                "scaled_distribution_fn_shape_parameter",
                "multiplier",
                "alpha_fc",
                "Klf",
                "Kn",
                "nash_n",
                "Cgw",
                "expon",
                "max_groundwater_storage_meters",
                "nash_storage",
                "soil_storage_percentage",
                "groundwater_storage_percentage",
                "timestep",
                "giuh"
            };

    };

}

#endif //NGEN_LSTM_REALIZATION_HPP

