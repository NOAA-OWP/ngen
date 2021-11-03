#ifndef SIMPLE_LUMPED_MODEL_REALIZATION_H
#define SIMPLE_LUMPED_MODEL_REALIZATION_H

#include "Catchment_Formulation.hpp"
#include "reservoir/Reservoir.hpp"
#include "hymod/include/Hymod.h"
#include <unordered_map>
#include <ForcingProvider.hpp>
#include <Forcing.h>

class Simple_Lumped_Model_Realization
        : public realization::Catchment_Formulation {
    public:

        typedef long time_step_t;

        Simple_Lumped_Model_Realization(
            std::string id,
            forcing_params forcing_config,
            utils::StreamHandler output_stream,
            double storage,
            double gw_storage,
            double gw_max_storage,
            double nash_max_storage,
            double smax,
            double a,
            double b,
            double Ks,
            double Kq,
            long n,
            const std::vector<double>& Sr,
            time_step_t t
        );

        Simple_Lumped_Model_Realization(std::string id, unique_ptr<forcing::ForcingProvider> forcing_provider, utils::StreamHandler output_stream) : Catchment_Formulation(id, std::move(forcing_provider), output_stream) {
            // We now only use the ForcingProvider interface on Forcing objects, so this is not needed (and explodes).
            //_link_legacy_forcing();
        };

        Simple_Lumped_Model_Realization(std::string id) : Catchment_Formulation(id){};

        /**
         * @brief Explicit move constructor
         * This constuctor explicitly moves a Simple_Lumped_Model_Realization
         * and is required to properly move the HY_CatchmentRealization forcing object
         */
        Simple_Lumped_Model_Realization(Simple_Lumped_Model_Realization &&);
        /**
         * @brief Explicit copy constructor
         * This constuctor explicitly copies Simple_Lumped_Model_Realization
         * and is required to properly copy the HY_CatchmentRealization forcing object
         * as well connectet the hymod_state.Sr* to the copied cascade_backing_storage vector
         */
        Simple_Lumped_Model_Realization(const Simple_Lumped_Model_Realization &);
        virtual ~Simple_Lumped_Model_Realization();

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
        double get_response(time_step_t t, time_step_t dt) override;

        double calc_et() override;

        void create_formulation(boost::property_tree::ptree &config, geojson::PropertyMap *global = nullptr) override;
        void create_formulation(geojson::PropertyMap properties) override;

        std::string get_formulation_type() override {
            return "simple_lumped";
        }

        void add_time(time_t t, double n) override;

    protected:
        std::vector<std::string> REQUIRED_PARAMETERS = {
            "sr",
            "storage",
            "gw_storage",
            "gw_max_storage",
            "nash_max_storage",
            "smax",
            "a",
            "b",
            "Ks",
            "Kq",
            "n",
            "t"
        };

        const std::vector<std::string>& get_required_parameters() override {
            return REQUIRED_PARAMETERS;
        }

    private:

        std::unordered_map<time_step_t, hymod_state> state;
        std::unordered_map<time_step_t, hymod_fluxes> fluxes;
        std::unordered_map<time_step_t, std::vector<double> > cascade_backing_storage;
        hymod_params params;



};

#endif // SIMPLE_LUMPED_MODEL_REALIZATION_H
