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
     * The core logic for the model is implemented within the `run` function.  The resulting state calculated by a call
     * to `run` is stored by a struct referenced via the `current_state` member.  A `previous_state` member also
     * exists for holding the state prior to the most recent call to `run`.  Calculated fluxes are held by the `fluxes`
     * member, for which there is not 'previous' analog.
     *
     * How state members are adjusted at the start of a call to `run` should be controlled via the implementation of
     * `manage_state_before_next_time_step_run`.
     *
     * Calls to run will return the result of a nested call to `mass_check`, which performs mass balance verification on
     * the calculated state at the end of a call to `run`.  The amount of acceptable difference is accessible with the
     * `get_mass_check_error_bound` function.  It is also possible to change the value of this (hard-coded in the
     * default implementation) with the protected `set_mass_check_error_bound` function.
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
         * @return The calculated soil field capacity storage.
         */
        double calc_soil_field_capacity_storage();

        shared_ptr<tshirt_state> get_current_state();

        shared_ptr<tshirt_fluxes> get_fluxes();

        double get_mass_check_error_bound();

        int mass_check(double input_flux_meters, double timestep_seconds);

        /**
         * Run the model to one time step, after performing initial housekeeping steps via a call to
         * `manage_state_before_next_time_step_run`.
         *
         * @param dt the time step
         * @param input_flux_meters the amount water entering the system this time step, in meters
         * @param et_params ET parameters struct
         * @return
         */
        int run(double dt, double input_flux_meters, shared_ptr<pdm03_struct> et_params);

    protected:

        /**
         * Perform necessary steps prior to the execution of model calculations for a new time step, for managing member
         * variables that contain model state.
         *
         * This function is intended to be run only at the start of a new execution of the tshirt_model::run method.  It
         * performs three housekeeping tasks needed before running the next group of time step modeling operations:
         *
         *      * the initial maintained `current_state` is moved to `previous_state`
         *      * a new `current_state` is created
         *      * a new `fluxes` is created
         */
        void manage_state_before_next_time_step_run();

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
        /** Soil field capacity storage, or the level at which free drainage stops (i.e., "Sfc"). */
        double soil_field_capacity_storage;

        /**
         * Check that the current state of this model object (which could be its provided initial state) is valid, printing
         * a message and raising an error if not.
         *
         * The model parameter for Nash Cascade size, `nash_n`, must correspond appropriately to the size of referenced
         * `current_state->nash_cascade_storeage_meters` vector.  The latter holds the storage values of the individual
         * reservoirs within the Nash Cascade.  Note that the function will interpret any `nash_n` greater than `0` as valid
         * if the vector itself is empty, and initialize such a vector to the correct size with all `0.0` values.
         */
        void check_valid();

        /**
         * Initialize the subsurface groundwater reservoir for the model, in the `groundwater_reservoir` member field.
         *
         * Initialize the subsurface groundwater reservoir for the model as a Nonlinear_Reservoir object, creating the
         * reservoir with a single outlet.  In particular, this is a Reservoir_Exponential_Outlet object, since the outlet
         * requires the following be used to calculate discharge flow:
         *
         *      Cgw * ( exp(expon * S / S_max) - 1 );
         *
         * Note that this function should only be used during object construction.
         *
         * @see Nonlinear_Reservoir
         * @see Reservoir_Exponential_Outlet
         */
        void initialize_groundwater_reservoir();

        /**
         * Initialize the subsurface soil reservoir for the model, in the `soil_reservoir` member field.
         *
         * Initialize the subsurface soil reservoir for the model as a Nonlinear_Reservoir object, creating the reservoir
         * with outlets for both the subsurface lateral flow and the percolation flow. This should only be used during
         * object construction.
         *
         * Per the class type of the reservoir, outlets have an associated index value within a reservoir, and certain
         * outlet-specific functionality requires having appropriate outlet index.  This function also sets corresponding
         * index values of the lateral flow and percolation flow outlets within the lf_outlet_index and
         * perc_outlet_index member variables respectively.
         *
         * @see Nonlinear_Reservoir
         */
        void initialize_soil_reservoir();

        /**
         * Initialize the Nash Cascade reservoirs applied to the subsurface soil reservoir's lateral flow outlet.
         *
         * Initialize the soil_lf_nash_res member, containing the collection of Nonlinear_Reservoir objects used to create
         * the Nash Cascade for soil_reservoir lateral flow outlet.  The analogous values for Nash Cascade storage from
         * previous_state are used for current storage of reservoirs at each given index.
         */
        void initialize_subsurface_lateral_flow_nash_cascade();

    };
}

#endif //TSHIRT_H
