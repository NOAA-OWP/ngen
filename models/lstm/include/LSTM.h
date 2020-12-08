#ifndef LSTM_H
#define LSTM_H


#include "all.h"
#include "lstm_fluxes.h"
#include "lstm_params.h"
#include "lstm_state.h"

#include <torch/torch.h>


using namespace std;

namespace lstm {

    class lstm_model {

    public:

        /**
         * Constructor for model object based on model parameters and initial state.
         *
         * @param model_params Model parameters lstm_params struct.
         * @param initial_state Shared smart pointer to lstm_state struct hold initial state values
         */
        lstm_model(lstm_params model_params, const shared_ptr<lstm_state>& initial_state);

        /**
         * Constructor for model object with parameters only.
         *
         * Constructor creates a "default" initial state, with soil_storage_meters and groundwater_storage_meters set to
         * 0.0, and then otherwise proceeds as the constructor accepting the second parameter for initial state.
         *
         * @param model_params Model parameters lstm_params struct.
         */
        lstm_model(lstm_params model_params);

        /**
         * Return the smart pointer to the lstm::lstm_model struct for holding this object's current state.
         *
         * @return The smart pointer to the lstm_model struct for holding this object's current state.
         */
        shared_ptr<lstm_state> get_current_state();

        /**
         * Return the shared pointer to the lstm::lstm_fluxes struct for holding this object's current fluxes.
         *
         * @return The shared pointer to the lstm_fluxes struct for holding this object's current fluxes.
         */
        shared_ptr<lstm_fluxes> get_fluxes();

        /**
         * Run the model to one time step, after performing initial housekeeping steps via a call to
         * `manage_state_before_next_time_step_run`.
         *
         * @param dt the time step
         * @param input_storage_m the amount water entering the system this time step, in meters
         * @param et_params ET parameters struct
         * @return
         */
        //int run(double dt, double input_storage_m, shared_ptr<pdm03_struct> et_params);
        int run(double dt, double AORC_DLWRF_surface_W_per_meters_squared, double PRES_surface_Pa, double SPFH_2maboveground_kg_per_kg, double precip, double DSWRF_surface_W_per_meters_squared, double TMP_2maboveground_K, double UGRD_10maboveground_meters_per_second, double VGRD_10maboveground_meters_per_second);

    protected:

        /**
         * Perform necessary steps prior to the execution of model calculations for a new time step, for managing member
         * variables that contain model state.
         *
         * This function is intended to be run only at the start of a new execution of the lstm_model::run method.  It
         * performs three housekeeping tasks needed before running the next group of time step modeling operations:
         *
         *      * the initial maintained `current_state` is moved to `previous_state`
         *      * a new `current_state` is created
         *      * a new `fluxes` is created
         */
        virtual void manage_state_before_next_time_step_run();

        /**
         * Set the mass_check_error_bound member to the absolute value of the given parameter.
         *
         * @param error_bound The value used to set the mass_check_error_bound member.
         */
        //void set_mass_check_error_bound(double error_bound);




    private:
        /** torch confiruation variables */
        bool useGPU;
        torch::Device device;
        /** Model state for the "current" time step, which may not be calculated yet. */
        shared_ptr<lstm_state> current_state;
        /** Model execution parameters. */
        lstm_params model_params;
        /** Model state from that previous time step before the current. */
        shared_ptr<lstm_state> previous_state;

        shared_ptr<lstm_fluxes> fluxes;

        torch::jit::script::Module model;

    };
}


#endif //LSTM_H
