#ifndef NGEN_TSHIRT_REALIZATION_HPP
#define NGEN_TSHIRT_REALIZATION_HPP

#include "Catchment_Formulation.hpp"
#include <unordered_map>
#include "GIUH.hpp"
#include "GiuhJsonReader.h"
#include "tshirt/include/Tshirt.h"
#include "tshirt/include/tshirt_params.h"
#include <memory>

namespace realization {

    class Tshirt_Realization : public Catchment_Formulation {

    public:

        typedef long time_step_t;

        Tshirt_Realization(forcing_params forcing_config,
                           utils::StreamHandler output_stream,
                           double soil_storage_meters,
                           double groundwater_storage_meters,
                           std::string catchment_id,
                           giuh::GiuhJsonReader &giuh_json_reader,
                           tshirt::tshirt_params params,
                           const std::vector<double>& nash_storage,
                           time_step_t t);

        Tshirt_Realization(
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

            Tshirt_Realization(
                std::string id,
                forcing_params forcing_config,
                utils::StreamHandler output_stream
            ) : Catchment_Formulation(id, forcing_config, output_stream) {}

            void set_giuh_kernel(std::shared_ptr<giuh::GiuhJsonReader> reader);

            virtual ~Tshirt_Realization(){};

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
                return "tshirt";
            }

            void create_formulation(boost::property_tree::ptree &config, geojson::PropertyMap *global = nullptr);

            void create_formulation(geojson::PropertyMap properties);

            const std::vector<std::string>& get_required_parameters() override {
                return REQUIRED_PARAMETERS;
            }

        private:
            std::string catchment_id;
            std::unordered_map<time_step_t, shared_ptr<tshirt::tshirt_state>> state;
            std::unordered_map<time_step_t, shared_ptr<tshirt::tshirt_fluxes>> fluxes;
            std::unordered_map<time_step_t, std::vector<double> > cascade_backing_storage;
            tshirt::tshirt_params *params;
            std::unique_ptr<tshirt::tshirt_model> model;
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


#endif //NGEN_TSHIRT_REALIZATION_HPP
