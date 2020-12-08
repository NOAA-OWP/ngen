#include "LSTM.h"
#include "lstmErrorCodes.h"
#include "lstm_fluxes.h"
#include "lstm_state.h"

using namespace std;

namespace lstm {

    /**
     * Constructor for model object based on model parameters and initial state.
     *
     * @param model_params Model parameters lstm_params struct.
     * @param initial_state Shared smart pointer to lstm_state struct hold initial state values
     */
    lstm_model::lstm_model(lstm_params model_params, const shared_ptr<lstm_state> &initial_state)
            : model_params(model_params), previous_state(initial_state), current_state(initial_state),
            device( torch::Device(torch::kCPU) )
    {
        // ********** Set fluxes to null for now: it is bogus until first call of run function, which initializes it
        fluxes = nullptr;
        //manually set torch seed for reproducibility
        torch::manual_seed(0);
        useGPU = torch::cuda::is_available();
        device = torch::Device( useGPU ? torch::kCUDA : torch::kCPU );
        lstm::to_device(*current_state, device);
        lstm::to_device(*previous_state, device);

        model = torch::jit::load(model_params.pytorch_model_path);
        model.to( device );
        // Set to `eval` model (just like Python)
        model.eval();
        //FIXME what is the SUPPOSED to do?
        torch::NoGradGuard no_grad_;

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
    lstm_model(model_params, make_shared<lstm_state>(lstm_state())) {}

    /**
     * Return the smart pointer to the lstm::lstm_model struct for holding this object's current state.
     *
     * @return The smart pointer to the lstm_model struct for holding this object's current state.
     */
    shared_ptr<lstm_state> lstm_model::get_current_state() {
        return current_state;
    }

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
    //int lstm_model::run(double dt, double input_storage_m, shared_ptr<pdm03_struct> et_params) {
    int lstm_model::run(double dt, double AORC_DLWRF_surface_W_per_meters_squared, double PRES_surface_Pa, double SPFH_2maboveground_kg_per_kg, double precip, double DSWRF_surface_W_per_meters_squared, double TMP_2maboveground_K, double UGRD_10maboveground_meters_per_second, double VGRD_10maboveground_meters_per_second) {
        // Do resetting/housekeeping for new calculations and new state values
        manage_state_before_next_time_step_run();











        // In meters
        //double soil_column_moisture_deficit_m =
        //        model_params.max_soil_storage_meters - previous_state->soil_storage_meters;

        // Perform Schaake partitioning, passing some declared references to hold the calculated values.
        //double surface_runoff, subsurface_infiltration_flux;
        //Schaake_partitioning_scheme(dt, model_params.Cschaake, soil_column_moisture_deficit_m, input_storage_m,
        //                            &surface_runoff, &subsurface_infiltration_flux);

/*
        double subsurface_excess, nash_subsurface_excess;
        soil_reservoir.response_meters_per_second(subsurface_infiltration_flux, dt, subsurface_excess);

        // lateral subsurface flow
        double Qlf = soil_reservoir.velocity_meters_per_second_for_outlet(lf_outlet_index);

        // percolation flow
        double Qperc = soil_reservoir.velocity_meters_per_second_for_outlet(perc_outlet_index);

        // TODO: make sure ET doesn't need to be taken out sooner
        // Get new soil storage amount calculated by reservoir
        double new_soil_storage_m = soil_reservoir.get_storage_height_meters();
        // Calculate and store ET
        fluxes->et_loss_meters = calc_evapotranspiration(new_soil_storage_m, et_params);
        // Update the current soil storage, accounting for ET
        current_state->soil_storage_meters = new_soil_storage_m - fluxes->et_loss_meters;

        // Cycle through lateral flow Nash cascade of reservoirs
        // loop essentially copied from Hymod logic, but with different variable names
        for (unsigned long int i = 0; i < soil_lf_nash_res.size(); ++i) {
            // get response water velocity of reservoir
            Qlf = soil_lf_nash_res[i]->response_meters_per_second(Qlf, dt, nash_subsurface_excess);
            // TODO: confirm this is correct
            Qlf += nash_subsurface_excess / dt;
            current_state->nash_cascade_storeage_meters[i] = soil_lf_nash_res[i]->get_storage_height_meters();
        }

        // Get response and update gw res state
        double excess_gw_water;
        fluxes->groundwater_flow_meters_per_second = groundwater_reservoir.response_meters_per_second(Qperc, dt,
                                                                                               excess_gw_water);
*/


//////////////////////////////////////////////////////
        // update local copy of state
        //current_state->groundwater_storage_meters = groundwater_reservoir.get_storage_height_meters();
////////////////////////////////////////////

        // record other fluxes in internal copy
        //fluxes->soil_lateral_flow_meters_per_second = Qlf;
        //fluxes->soil_percolation_flow_meters_per_second = Qperc;

        // Save "raw" runoff here and have realization class calculate GIUH surface runoff using that kernel
        // TODO: for now add this to runoff, but later adjust calculations to limit flow into reservoir to avoid excess
        //fluxes->surface_runoff_meters_per_second = surface_runoff + (subsurface_excess / dt) + (excess_gw_water / dt);
        //fluxes->surface_runoff_meters_per_second = surface_runoff;

        //return mass_check(input_storage_m, dt);
        return 0;
    }

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
    void lstm_model::manage_state_before_next_time_step_run()
    {
        previous_state = current_state;
        //current_state = make_shared<lstm_state>(lstm_state(0.0, 0.0, vector<double>(model_params.nash_n)));
        //fluxes = make_shared<lstm_fluxes>(lstm_fluxes(0.0, 0.0, 0.0, 0.0, 0.0));
    }

}
