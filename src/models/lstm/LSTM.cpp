#include "LSTM.h"
#include "lstmErrorCodes.h"
#include "lstm_fluxes.h"
#include "lstm_state.h"
#include "Constants.h"
#include <cmath>

#include <iostream>


using namespace std;

namespace lstm {

    /**
     * Constructor for model object based on model parameters and initial state.
     *
     * @param model_params Model parameters lstm_params struct.
     * @param initial_state Shared smart pointer to lstm_state struct hold initial state values
     */
    lstm_model::lstm_model(lstm_params model_params, const shared_ptr<lstm_state> &initial_state)
            : model_params(model_params), previous_state(initial_state), current_state(initial_state)
    {
        fluxes = nullptr;



    }

    /**
     * Constructor for model object with parameters only.
     *
     * Constructor creates a "default" initial state, with soil_storage_meters and groundwater_storage_meters set to
     * 0.0, and then otherwise proceeds as the constructor accepting the second parameter for initial state.
     *
     * @param model_params Model parameters lstm_params struct.
     */
    lstm_model::lstm_model(lstm_params model_params) :
    //lstm_model(model_params, make_shared<lstm_state>(lstm_state(0.0, 0.0))) {}
    //lstm_model(model_params, make_shared<lstm_state>(lstm_state(0.0))) {}
    lstm_model(model_params, make_shared<lstm_state>(lstm_state())) {}


    /**
     * Return the shared pointer to the lstm::lstm_fluxes struct for holding this object's current fluxes.
     *
     * @return The shared pointer to the lstm_fluxes struct for holding this object's current fluxes.
     */
    shared_ptr<lstm_fluxes> lstm_model::get_fluxes() {
        return fluxes;
    }

    /**
     * Run the model to one time step, after performing initial housekeeping steps via a call to
     * `manage_state_before_next_time_step_run`.
     *
     * @param dt the time step size in seconds
     * @param input_storage_m the amount water entering the system this time step, in meters
     * @return
     */
    int lstm_model::run(double dt, double input_storage_m, shared_ptr<pdm03_struct> et_params) {
        // Do resetting/housekeeping for new calculations and new state values
        //manage_state_before_next_time_step_run();


        return 0;
    }

}
