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
        // ********** Start by calculating Sfc, as this will get by several other things
        soil_field_capacity_storage = calc_soil_field_capacity_storage();

        // ********** Sanity check init (in particular, size of Nash Cascade storage vector in the state parameter).
        check_valid();

        // ********** Create the vector of Nash Cascade reservoirs used at the end of the soil lateral flow outlet
        initialize_subsurface_lateral_flow_nash_cascade();

        // ********** Create the soil reservoir
        initialize_soil_reservoir();

        // ********** Create the groundwater reservoir
        initialize_groundwater_reservoir();

        // ********** Set fluxes to null for now: it is bogus until first call of run function, which initializes it
        fluxes = nullptr;

        // ********** Acceptable error range for mass balance calculations; hard-coded for now to this value
        mass_check_error_bound = 0.000001;


        cout << model_params.input_biases_path;


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
    lstm_model(model_params, make_shared<lstm_state>(lstm_state(0.0, 0.0))) {}

    /**
     * Check that the current state of this model object (which could be its provided initial state) is valid, printing
     * a message and raising an error if not.
     *
     * The model parameter for Nash Cascade size, `nash_n`, must correspond appropriately to the size of referenced
     * `current_state->nash_cascade_storeage_meters` vector.  The latter holds the storage values of the individual
     * reservoirs within the Nash Cascade.  Note that the function will interpret any `nash_n` greater than `0` as valid
     * if the vector itself is empty, and initialize such a vector to the correct size with all `0.0` values.
     */
    void lstm_model::check_valid()
    {
        // We expect the Nash size model parameter 'nash_n' to be equal to the size of the
        // 'nash_cascade_storeage_meters' member of the passed 'initial_state param (to which 'current_state' is set)
        if (current_state->nash_cascade_storeage_meters.size() != model_params.nash_n) {
            // Infer that empty vector should be initialized to a vector of size 'nash_n' with all 0.0 values
            if (model_params.nash_n > 0 && current_state->nash_cascade_storeage_meters.empty()) {
                current_state->nash_cascade_storeage_meters.resize(model_params.nash_n);
                for (unsigned i = 0; i < model_params.nash_n; ++i) {
                    current_state->nash_cascade_storeage_meters[i] = 0.0;
                }
            }
            else {
                cerr << "ERROR: Nash Cascade size parameter in lstm model init doesn't match storage vector size "
                     << "in state parameter"
                     << endl;
                // TODO: return error of some kind here
            }
        }
    }

    /**
     * Initialize the subsurface groundwater reservoir for the model, in the `groundwater_reservoir` member field.
     *
     * Initialize the subsurface groundwater reservoir for the model as a Reservoir object, creating the
     * reservoir with a single outlet.  In particular, this is a Reservoir_Exponential_Outlet object, since the outlet
     * requires the following be used to calculate discharge flow:
     *
     *      Cgw * ( exp(expon * S / S_max) - 1 );
     *
     * Note that this function should only be used during object construction.
     *
     * @see Reservoir
     * @see Reservoir_Exponential_Outlet
     */
    void lstm_model::initialize_groundwater_reservoir()
    {
        // TODO: confirm, based on the equation, that max gw velocity doesn't need to be Cgw * (exp(expon) - 1)
        // TODO: (i.e., S == S_max, thus maximizing the term passed to the exp() function, and thereby the equation)
        double max_gw_velocity = std::numeric_limits<double>::max();

        // Build vector of pointers to outlets to pass the custom exponential outlet through
        vector<std::shared_ptr<Reservoir::Explicit_Time::Reservoir_Outlet>> gw_outlets_vector(1);
        // TODO: verify activation threshold
        gw_outlets_vector[0] = make_shared<Reservoir::Explicit_Time::Reservoir_Exponential_Outlet>(
                Reservoir::Explicit_Time::Reservoir_Exponential_Outlet(model_params.Cgw, model_params.expon, 0.0, max_gw_velocity));
        // Create the reservoir, passing the outlet via the vector argument
        groundwater_reservoir = Reservoir::Explicit_Time::Reservoir(0.0, model_params.max_groundwater_storage_meters,
                                                    previous_state->groundwater_storage_meters, gw_outlets_vector);
    }

    /**
     * Initialize the subsurface soil reservoir for the model, in the `soil_reservoir` member field.
     *
     * Initialize the subsurface soil reservoir for the model as a Reservoir object, creating the reservoir
     * with outlets for both the subsurface lateral flow and the percolation flow.  This should only be used during
     * object construction.
     *
     * Per the class type of the reservoir, outlets have an associated index value within a reservoir, and certain
     * outlet-specific functionality requires having appropriate outlet index.  These outlet indexes are maintained in
     * for the lateral flow and percolation flow outlets are maintained this class within the lf_outlet_index and
     * perc_outlet_index member variables respectively.
     *
     * @see Reservoir
     */
    void lstm_model::initialize_soil_reservoir()
    {
        // Build the vector of pointers to reservoir outlets
        vector<std::shared_ptr<Reservoir::Explicit_Time::Reservoir_Outlet>> soil_res_outlets(2);

        // init subsurface lateral flow linear outlet
        soil_res_outlets[lf_outlet_index] = std::make_shared<Reservoir::Explicit_Time::Reservoir_Linear_Outlet>(
                Reservoir::Explicit_Time::Reservoir_Linear_Outlet(model_params.Klf, soil_field_capacity_storage, model_params.max_lateral_flow));

        // init subsurface percolation flow linear outlet
        // The max perc flow should be equal to the params.satdk value
        soil_res_outlets[perc_outlet_index] = std::make_shared<Reservoir::Explicit_Time::Reservoir_Linear_Outlet>(
                Reservoir::Explicit_Time::Reservoir_Linear_Outlet(model_params.satdk * model_params.slope, soil_field_capacity_storage,
                                 std::numeric_limits<double>::max()));
        // Create the reservoir, included the created vector of outlet pointers
        soil_reservoir = Reservoir::Explicit_Time::Reservoir(0.0, model_params.max_soil_storage_meters,
                                             previous_state->soil_storage_meters, soil_res_outlets);
    }

    /**
     * Initialize the Nash Cascade reservoirs applied to the subsurface soil reservoir's lateral flow outlet.
     *
     * Initialize the soil_lf_nash_res member, containing the collection of Reservoir objects used to create
     * the Nash Cascade for soil_reservoir lateral flow outlet.  The analogous values for Nash Cascade storage from
     * previous_state are used for current storage of reservoirs at each given index.
     */
    void lstm_model::initialize_subsurface_lateral_flow_nash_cascade()
    {
        soil_lf_nash_res.resize(model_params.nash_n);
        // TODO: verify correctness of activation_threshold (Sfc) and max_velocity (max_lateral_flow) arg values
        for (unsigned long i = 0; i < soil_lf_nash_res.size(); ++i) {
            //construct a single linear outlet reservoir
            soil_lf_nash_res[i] = make_unique<Reservoir::Explicit_Time::Reservoir>(
                    Reservoir::Explicit_Time::Reservoir(0.0, model_params.max_soil_storage_meters,
                                        previous_state->nash_cascade_storeage_meters[i], model_params.Kn,
                                        0.0, model_params.max_lateral_flow));
        }
    }

    /**
     * Calculate losses due to evapotranspiration.
     *
     * @param soil_m The soil moisture measured in meters.
     * @param et_params A shared pointer to the struct holding the ET parameters.
     * @return The calculated loss value due to evapotranspiration.
     */
    double lstm_model::calc_evapotranspiration(double soil_m, shared_ptr<pdm03_struct> et_params) {
        et_params->final_height_reservoir = soil_m;
        pdm03_wrapper(et_params.get());

        return et_params->final_height_reservoir - soil_m;
    }

    /**
     * Calculate soil field capacity storage, the level at which free drainage stops (i.e., "Sfc").
     *
     * @return The calculated soil field capacity storage.
     */
    double lstm_model::calc_soil_field_capacity_storage()
    {
        // Calculate the suction head above water table (Hwt)
        double head_above_water_table =
                model_params.alpha_fc * ((double) STANDARD_ATMOSPHERIC_PRESSURE_PASCALS / WATER_SPECIFIC_WEIGHT);
        // TODO: account for possibility of Hwt being less than 0.5 (though initially, it looks like this will never be the case)

        double z1 = head_above_water_table - 0.5;
        double z2 = z1 + 2;

        // Note that z^( 1 - (1/b) ) / (1 - (1/b)) == b * (z^( (b - 1) / b ) / (b - 1)
        return model_params.maxsmc * pow((1.0 / model_params.satpsi), (-1.0 / model_params.b)) *
               ((model_params.b * pow(z2, ((model_params.b - 1) / model_params.b)) / (model_params.b - 1)) -
                (model_params.b * pow(z1, ((model_params.b - 1) / model_params.b)) / (model_params.b - 1)));
    }

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
     * Get the size of the error bound that is acceptable when performing mass check calculations.
     *
     * @return The size of the error bound that is acceptable when performing mass check calculations.
     * @see lstm_model::mass_check
     */
    double lstm_model::get_mass_check_error_bound() {
        return mass_check_error_bound;
    }

    /**
     * Check that mass was conserved by the model's calculations of the current time step.
     *
     * @param input_storage_m The amount of water input to the system at the time step, in meters.
     * @param timestep_s The size of the time step, in seconds.
     * @return The appropriate code value indicating whether mass was conserved in the current time step's calculations.
     */
    int lstm_model::mass_check(double input_storage_m, double timestep_s) {
        // TODO: change this to have those be part of state somehow, either of object or struct, or just make private
        // Initialize both mass values from current and next states storage
        double previous_storage_m = previous_state->soil_storage_meters + previous_state->groundwater_storage_meters;
        double current_storage_m = current_state->soil_storage_meters + current_state->groundwater_storage_meters;

        // Add the masses of the Nash reservoirs before and after the time step
        for (unsigned int i = 0; i < previous_state->nash_cascade_storeage_meters.size(); ++i) {
            previous_storage_m += previous_state->nash_cascade_storeage_meters[i];
            current_storage_m += current_state->nash_cascade_storeage_meters[i];
        }

        // Increase the initial mass by input value
        previous_storage_m += input_storage_m;

        // Increase final mass by calculated fluxes that leave the system (i.e., not the percolation flow)
        current_storage_m += fluxes->et_loss_meters;
        current_storage_m += fluxes->surface_runoff_meters_per_second * timestep_s;
        current_storage_m += fluxes->soil_lateral_flow_meters_per_second * timestep_s;
        current_storage_m += fluxes->groundwater_flow_meters_per_second * timestep_s;

        double abs_mass_diff_meters = abs(previous_storage_m - current_storage_m);
        return abs_mass_diff_meters > get_mass_check_error_bound() ? lstm::LSTM_MASS_BALANCE_ERROR
                                                                   : lstm::LSTM_NO_ERROR;
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
        manage_state_before_next_time_step_run();

        // In meters
        double soil_column_moisture_deficit_m =
                model_params.max_soil_storage_meters - previous_state->soil_storage_meters;

        // Perform Schaake partitioning, passing some declared references to hold the calculated values.
        double surface_runoff, subsurface_infiltration_flux;
        Schaake_partitioning_scheme(dt, model_params.Cschaake, soil_column_moisture_deficit_m, input_storage_m,
                                    &surface_runoff, &subsurface_infiltration_flux);

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
        // update local copy of state
        current_state->groundwater_storage_meters = groundwater_reservoir.get_storage_height_meters();

        // record other fluxes in internal copy
        fluxes->soil_lateral_flow_meters_per_second = Qlf;
        fluxes->soil_percolation_flow_meters_per_second = Qperc;

        // Save "raw" runoff here and have realization class calculate GIUH surface runoff using that kernel
        // TODO: for now add this to runoff, but later adjust calculations to limit flow into reservoir to avoid excess
        fluxes->surface_runoff_meters_per_second = surface_runoff + (subsurface_excess / dt) + (excess_gw_water / dt);
        //fluxes->surface_runoff_meters_per_second = surface_runoff;

        return mass_check(input_storage_m, dt);
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
        current_state = make_shared<lstm_state>(lstm_state(0.0, 0.0, vector<double>(model_params.nash_n)));
        fluxes = make_shared<lstm_fluxes>(lstm_fluxes(0.0, 0.0, 0.0, 0.0, 0.0));
    }

    /**
     * Set the mass_check_error_bound member to the absolute value of the given parameter.
     *
     * @param error_bound The value used to set the mass_check_error_bound member.
     */
    void lstm_model::set_mass_check_error_bound(double error_bound) {
        mass_check_error_bound = error_bound >= 0 ? error_bound : abs(error_bound);
    }

}
