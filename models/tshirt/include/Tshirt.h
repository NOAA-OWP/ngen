#ifndef TSHIRT_H
#define TSHIRT_H

#include "all.h"
#include "schaake_partitioning.hpp"
#include "Constants.h"
#include "reservoir/Nonlinear_Reservoir.hpp"
#include "Pdm03.h"
#include "GIUH.hpp"
#include "reservoir/Reservoir_Exponential_Outlet.hpp"
#include "reservoir/Reservoir_Outlet.hpp"
#include "tshirt_fluxes.h"
#include "tshirt_params.h"
#include "tshirt_state.h"
#include <cmath>
#include <utility>
#include <vector>
#include <memory>

namespace tshirt {

    /**
     * Tshirt model class.
     *
     * Object oriented implementation of the Tshirt hydrological model, as documented at:
     *      https://github.com/NOAA-OWP/ngen/blob/master/doc/T-shirt_model_description.pdf
     *
     *
     */
    class tshirt_model {

    public:

        /**
         * Constructor for model object based on model parameters and initial state.
         *
         * @param model_params Model parameters tshirt_params struct.
         * @param initial_state Shared smart pointer to tshirt_state struct hold initial state values
         */
        tshirt_model(tshirt_params model_params, const shared_ptr<tshirt_state>& initial_state);

        /**
         * Constructor for model object with parameters only.
         *
         * Constructor creates a "default" initial state, with soil_storage_meters and groundwater_storage_meters set to
         * 0.0, and then otherwise proceeds as the constructor accepting the second parameter for initial state.
         *
         * @param model_params Model parameters tshirt_params struct.
         */
        tshirt_model(tshirt_params model_params);

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
}

#endif //TSHIRT_H
