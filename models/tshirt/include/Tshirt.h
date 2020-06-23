
#ifndef TSHIRT_H
#define TSHIRT_H

#include "schaake_partitioning.hpp"
#include "Constants.h"
#include "Nonlinear_Reservoir.hpp"
#include "Pdm03.h"
#include "GIUH.hpp"
#include "tshirt_fluxes.h"
#include "tshirt_params.h"
#include "tshirt_state.h"
#include <cmath>
#include <utility>
#include <vector>
#include <memory>

namespace tshirt {

    // TODO: consider combining with or differentiating from similar hymod enum
    enum TshirtErrorCodes {
        TSHIRT_NO_ERROR = 0,
        TSHIRT_MASS_BALANCE_ERROR = 100
    };

    /**
     * Tshirt model class.
     *
     * A less static, more OO implementation of the Tshirt hydrological model.
     */
    class tshirt_model {

    public:

        tshirt_model(tshirt_params model_params,
                     const shared_ptr<tshirt_state>& initial_state) : model_params(model_params),
                                                                      previous_state(initial_state),
                                                                      current_state(initial_state) {
            // ********** Calculate Sfc, as this will get used a few times ...
            double Sfc = calc_soil_field_capacity_storage();

            // ********** Sanity check the size of the Nash Cascade storage vector in the state parameter.
            // We expect the Nash size model parameter 'nash_n' to be equal to the size of the
            // 'nash_cascade_storeage_meters' member of the passed 'initial_state param
            if (initial_state->nash_cascade_storeage_meters.size() != model_params.nash_n) {
                // Infer that empty vector should be initialized to a vector of size 'nash_n' with all 0.0 values
                if (initial_state->nash_cascade_storeage_meters.empty()) {
                    initial_state->nash_cascade_storeage_meters.resize(model_params.nash_n);
                    for (unsigned i = 0; i < model_params.nash_n; ++i) {
                        initial_state->nash_cascade_storeage_meters[i] = 0.0;
                    }
                }
                else {

                    cerr << "ERROR: Nash Cascade size parameter in tshirt model init doesn't match storage vector size";
                    cerr << " in state parameter" << endl;
                    // TODO: return error of some kind here
                }
            }

            // ********** Create the vector of Nash Cascade reservoirs used at the end of the soil lateral flow outlet
            soil_lf_nash_res.resize(model_params.nash_n);
            // TODO: verify correctness of activation_threshold (Sfc) and max_velocity (max_lateral_flow) arg values
            for (unsigned long i = 0; i < soil_lf_nash_res.size(); ++i) {
                //construct a single outlet nonlinear reservoir
                soil_lf_nash_res[i] = make_unique<Nonlinear_Reservoir>(
                        Nonlinear_Reservoir(0.0, model_params.max_soil_storage_meters,
                                            previous_state->nash_cascade_storeage_meters[i], model_params.Kn, 1.0,
                                            0.0, model_params.max_lateral_flow));
            }

            // ********** Create the soil reservoir
            // Build the vector of pointers to reservoir outlets
            vector<std::shared_ptr<Reservoir_Outlet>> soil_res_outlets(2);

            // init subsurface later flow outlet
            soil_res_outlets[lf_outlet_index] = std::make_shared<Reservoir_Outlet>(
                    Reservoir_Outlet(model_params.Klf, 1.0, Sfc, model_params.max_lateral_flow));

            // init subsurface percolation flow outlet
            // The max perc flow should be equal to the params.satdk value
            soil_res_outlets[perc_outlet_index] = std::make_shared<Reservoir_Outlet>(
                    Reservoir_Outlet(model_params.satdk * model_params.slope, 1.0, Sfc,
                                     std::numeric_limits<double>::max()));
            // Create the reservoir, included the created vector of outlet pointers
            soil_reservoir = Nonlinear_Reservoir(0.0, model_params.max_soil_storage_meters, previous_state->soil_storage_meters,
                                                 soil_res_outlets);

            // ********** Create the groundwater reservoir
            // Given the equation:
            //      double groundwater_flow_meters_per_second = params.Cgw * ( exp(params.expon * state.groundwater_storage_meters / params.max_groundwater_storage_meters) - 1 );
            // The max value should be when groundwater_storage_meters == max_groundwater_storage_meters, or ...
            double max_gw_velocity = std::numeric_limits<double>::max();//model_params.Cgw * (exp(model_params.expon) - 1);

            // Build vector of pointers to outlets to pass the custom exponential outlet through
            vector<std::shared_ptr<Reservoir_Outlet>> gw_outlets_vector(1);
            // TODO: verify activation threshold
            gw_outlets_vector[0] = make_shared<Reservoir_Exponential_Outlet>(
                    Reservoir_Exponential_Outlet(model_params.Cgw, model_params.expon, 0.0, max_gw_velocity));
            // Create the reservoir, passing the outlet via the vector argument
            groundwater_reservoir = Nonlinear_Reservoir(0.0, model_params.max_groundwater_storage_meters,
                                                        previous_state->groundwater_storage_meters, gw_outlets_vector);

            // ********** Set fluxes to null for now: it is bogus until first call of run function, which initializes it
            fluxes = nullptr;

            // ********** Set this statically to this default value
            mass_check_error_bound = 0.000001;

        }

        tshirt_model(tshirt_params model_params) :
            tshirt_model(model_params, make_shared<tshirt_state>(tshirt_state(0.0, 0.0))) {}

        /**
         * Calculate losses due to evapotranspiration.
         *
         * @param soil_m
         * @param et_params
         * @return
         */
        double calc_evapotranspiration(double soil_m, shared_ptr<pdm03_struct> et_params);

        /**
         * Calculate soil field capacity storage, the level at which free drainage stops (i.e., "Sfc").
         *
         * @return
         */
        double calc_soil_field_capacity_storage();

        shared_ptr<tshirt_state> get_current_state();

        shared_ptr<tshirt_fluxes> get_fluxes();

        double get_mass_check_error_bound();

        int mass_check(double input_flux_meters, double timestep_seconds);

        /**
         * Run the model to one time step, moving the initial `current_state` value to `previous_state` and resetting
         * other members applicable only to in the context of the current time step so that they are recalculated.
         *
         * @param dt the time step
         * @param input_flux_meters the amount water entering the system this time step, in meters
         * @param et_params ET parameters struct
         * @return
         */
        int run(double dt, double input_flux_meters, shared_ptr<pdm03_struct> et_params);

    protected:

        void set_mass_check_error_bound(double error_bound);

    private:
        /** Model state for the "current" time step, which may not be calculated yet. */
        shared_ptr<tshirt_state> current_state;
        /** Model execution parameters. */
        tshirt_params model_params;
        /** Model state from that previous time step before the current. */
        shared_ptr<tshirt_state> previous_state;
        /**
         * A collection of reservoirs for a Nash Cascade at the end of the lateral flow output from the subsurface soil
         * reservoir.
         */
        vector<unique_ptr<Nonlinear_Reservoir>> soil_lf_nash_res;
        //FIXME reservoir construction sorts outlets by activation_threshold
        //so the fixed index assumption is invalid.  However, in the current use case
        //they both have the save activation_threshold (Sfc), but we do want percolation fluxes to happen first
        //so make it index 0
        /** The index of the subsurface lateral flow outlet in the soil reservoir. */
        int lf_outlet_index = 1;
        /** The index of the percolation flow outlet in the soil reservoir. */
        int perc_outlet_index = 0;
        Nonlinear_Reservoir soil_reservoir;
        Nonlinear_Reservoir groundwater_reservoir;
        shared_ptr<tshirt_fluxes> fluxes;
        /** The size of the error bound that is acceptable when performing mass check calculations. */
        double mass_check_error_bound;

    };

    /*!
     * Tshirt kernel class
     *
     * This class implements the Tshirt hydrological model.
     */
    class tshirt_kernel {
    public:

        /**
         * Calculate losses due to evapotranspiration.
         *
         * @param soil_m
         * @param et_params
         * @return
         */
        static double calc_evapotranspiration(double soil_m, void *et_params) {
            pdm03_struct *pdm = (pdm03_struct *) et_params;
            pdm->XHuz = soil_m;
            pdm03_wrapper(pdm);

            return pdm->XHuz - soil_m;
        }

        /**
         * Calculate soil field capacity storage, the level at which free drainage stops (i.e., "Sfc").
         *
         * @param params
         * @param state
         * @return
         */
        static double calc_soil_field_capacity_storage(const tshirt_params &params) {
            // Calculate the suction head above water table (Hwt)
            double head_above_water_table = params.alpha_fc * ( (double) STANDARD_ATMOSPHERIC_PRESSURE_PASCALS / WATER_SPECIFIC_WEIGHT);
            // TODO: account for possibility of Hwt being less than 0.5 (though initially, it looks like this will never be the case)

            double z1 = head_above_water_table - 0.5;
            double z2 = z1 + 2;

            // Note that z^( 1 - (1/b) ) / (1 - (1/b)) == b * (z^( (b - 1) / b ) / (b - 1)
            return params.maxsmc * pow((1.0 / params.satpsi), (-1.0 / params.b)) *
                   ((params.b * pow(z2, ((params.b - 1) / params.b)) / (params.b - 1)) -
                    (params.b * pow(z1, ((params.b - 1) / params.b)) / (params.b - 1)));
        }

        static void init_nash_cascade_vector(vector<Nonlinear_Reservoir> &reservoirs, const tshirt_params &params,
                                             const tshirt_state &state, const double activation,
                                             const double max_flow_velocity) {
            reservoirs.resize(params.nash_n);
            for (unsigned long i = 0; i < reservoirs.size(); ++i) {
                //construct a single outlet nonlinear reservoir
                reservoirs[i] = Nonlinear_Reservoir(0, params.max_soil_storage_meters,
                                                    state.nash_cascade_storeage_meters[i], params.Kn, 1, activation,
                                                    max_flow_velocity);
            }
        }

        //! run one time step of tshirt
        static int run(
                double dt,
                tshirt_params params,        //!< static parameters for tshirt
                tshirt_state state,          //!< model state
                tshirt_state &new_state,     //!< model state struct to hold new model state
                tshirt_fluxes &fluxes,       //!< model flux object to hold calculated fluxes
                double input_flux_meters,          //!< the amount water entering the system this time step
                // TODO: should/can this be a smart pointer?
                giuh::giuh_kernel *giuh_obj,       //!< kernel object for calculating GIUH runoff from subsurface lateral flow
                void *et_params)            //!< parameters for the et function
        {
            double column_total_soil_moisture_deficit = params.max_soil_storage_meters - state.soil_storage_meters;

            // Note this surface runoff value has not yet performed GIUH calculations
            double surface_runoff, subsurface_infiltration_flux;

            Schaake_partitioning_scheme(dt, params.Cschaake, column_total_soil_moisture_deficit, input_flux_meters,
                                        &surface_runoff, &subsurface_infiltration_flux);

            double Sfc = calc_soil_field_capacity_storage(params);

            vector<std::shared_ptr<Reservoir_Outlet>> subsurface_outlets(2);

            // Keep track of the indexes of the specific outlets for later access
            int lf_outlet_index = 0;
            int perc_outlet_index = 1;

            // init subsurface later flow outlet
            // TODO: look into this in debugging, as it looks like a EXC_BAD_ACCESS exception is actually happening
            //  during unit testing and not being especially visible
            subsurface_outlets[lf_outlet_index] = std::make_shared<Reservoir_Outlet>(
                    Reservoir_Outlet(params.Klf, 1.0, Sfc, params.max_lateral_flow));
            // init subsurface percolation flow outlet
            // The max perc flow should be equal to the params.satdk value
            subsurface_outlets[perc_outlet_index] = std::make_shared<Reservoir_Outlet>(
                    Reservoir_Outlet(params.satdk * params.slope, 1.0, Sfc, params.satdk));

            Nonlinear_Reservoir subsurface_reservoir(0.0, params.depth, state.soil_storage_meters, subsurface_outlets);

            double subsurface_excess;
            subsurface_reservoir.response_meters_per_second(subsurface_infiltration_flux, dt, subsurface_excess);

            // lateral subsurface flow
            double Qlf = subsurface_reservoir.velocity_meters_per_second_for_outlet(lf_outlet_index);

            // percolation flow
            double Qperc = subsurface_reservoir.velocity_meters_per_second_for_outlet(perc_outlet_index);

            // TODO: make sure ET doesn't need to be taken out sooner
            double new_soil_storage = subsurface_reservoir.get_storage_height_meters();
            fluxes.et_loss_meters = calc_evapotranspiration(new_soil_storage, et_params);
            new_state.soil_storage_meters = new_soil_storage - fluxes.et_loss_meters;

            // initialize the Nash cascade of nonlinear reservoirs
            std::vector<Nonlinear_Reservoir> nash_cascade;
            // TODO: verify correctness of activation_threshold (Sfc) and max_velocity (max_lateral_flow) arg values
            init_nash_cascade_vector(nash_cascade, params, state, Sfc, params.max_lateral_flow);

            // cycle through lateral flow Nash cascade of nonlinear reservoirs
            // loop essentially copied from Hymod logic, but with different variable names
            for (unsigned long int i = 0; i < nash_cascade.size(); ++i) {
                // get response water velocity of nonlinear reservoir
                Qlf = nash_cascade[i].response_meters_per_second(Qlf, dt, subsurface_excess);
                // TODO: confirm this is correct
                Qlf += subsurface_excess / dt;
            }

            // "raw" GW calculations
            //state.groundwater_storage_meters += soil_percolation_flow_meters_per_second * dt;
            //double groundwater_flow_meters_per_second = params.Cgw * ( exp(params.expon * state.groundwater_storage_meters / params.max_groundwater_storage_meters) - 1 );

            // Given the equation:
            //      double groundwater_flow_meters_per_second = params.Cgw * ( exp(params.expon * state.groundwater_storage_meters / params.max_groundwater_storage_meters) - 1 );
            // The max value should be when groundwater_storage_meters == max_groundwater_storage_meters, or ...
            double max_gw_velocity = params.Cgw * (exp(params.expon) - 1);
            // TODO: verify activation threshold
            Nonlinear_Reservoir groundwater_res(0, params.max_groundwater_storage_meters,
                                                state.groundwater_storage_meters, params.Cgw, 1, 0, max_gw_velocity);
            // TODO: what needs to be done with this value?
            double excess_gw_water;
            fluxes.groundwater_flow_meters_per_second = groundwater_res.response_meters_per_second(Qperc, dt,
                                                                                                   excess_gw_water);
            // update state
            new_state.groundwater_storage_meters = groundwater_res.get_storage_height_meters();

            // record other fluxes
            fluxes.soil_lateral_flow_meters_per_second = Qlf;
            fluxes.soil_percolation_flow_meters_per_second = Qperc;
            // Calculate GIUH surface runoff
            fluxes.surface_runoff_meters_per_second = giuh_obj->calc_giuh_output(dt, surface_runoff);

            return 0;
        }

        static int mass_check(
                const tshirt_params &params,
                const tshirt_state &current_state,
                double input_flux_meters,
                const tshirt_state &next_state,
                const tshirt_fluxes &calculated_fluxes,
                double timestep_seconds) {
            // TODO: implement
            return 0;
        }
    };
}

//!

//!



#endif //TSHIRT_H
