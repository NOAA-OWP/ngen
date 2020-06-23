#include "Tshirt.h"
#include "TshirtErrorCodes.h"
#include "tshirt_fluxes.h"
#include "tshirt_state.h"
#include "Constants.h"
#include <cmath>

namespace tshirt {

    /**
     * Calculate losses due to evapotranspiration.
     *
     * @param soil_m The soil moisture measured in meters.
     * @param et_params A shared pointer to the struct holding the ET parameters.
     * @return The calculated loss value due to evapotranspiration.
     */
    double tshirt_model::calc_evapotranspiration(double soil_m, shared_ptr<pdm03_struct> et_params) {
        et_params->XHuz = soil_m;
        pdm03_wrapper(et_params.get());

        return et_params->XHuz - soil_m;
    }

    /**
     * Calculate soil field capacity storage, the level at which free drainage stops (i.e., "Sfc").
     *
     * @return The calculated soil field capacity storage.
     */
    double tshirt_model::calc_soil_field_capacity_storage()
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
     * Return this object's member that is the smart shared pointer to the ``tshirt_model`` struct for holding the
     * object's current state.
     *
     * @return The smart shared pointer to the ``tshirt_model`` struct for holding this object's current state.
     */
    shared_ptr<tshirt_state> tshirt_model::get_current_state() {
        return current_state;
    }

    /**
     * Return this object's member that is the smart shared pointer to the ``tshirt_fluxes`` struct for holding the
     * object's current fluxes.
     *
     * @return The smart shared pointer to the ``tshirt_fluxes`` struct for holding this object's current fluxes.
     */
    shared_ptr<tshirt_fluxes> tshirt_model::get_fluxes() {
        return fluxes;
    }

    /**
     * Get the size of the error bound that is acceptable when performing mass check calculations.
     *
     * @return The size of the error bound that is acceptable when performing mass check calculations.
     * @see tshirt_model::mass_check
     */
    double tshirt_model::get_mass_check_error_bound() {
        return mass_check_error_bound;
    }

    /**
     * Check that mass was conserved by the model's calculations of the current time step.
     *
     * @param input_flux_meters The amount of water input to the system at the time step, in meters.
     * @param timestep_seconds The size of the time step, in seconds.
     * @return The appropriate code value indicating whether mass was conserved in the current time step's calculations.
     */
    int tshirt_model::mass_check(double input_flux_meters, double timestep_seconds) {
        // TODO: change this to have those be part of state somehow, either of object or struct, or just make private
        // Initialize both mass values from current and next states storage
        double previous_mass_meters = previous_state->soil_storage_meters + previous_state->groundwater_storage_meters;
        double current_mass_meters = current_state->soil_storage_meters + current_state->groundwater_storage_meters;

        // Add the masses of the Nash reservoirs before and after the time step
        for (unsigned int i = 0; i < previous_state->nash_cascade_storeage_meters.size(); ++i) {
            previous_mass_meters += previous_state->nash_cascade_storeage_meters[i];
            current_mass_meters += current_state->nash_cascade_storeage_meters[i];
        }

        // Increase the initial mass by input value
        previous_mass_meters += input_flux_meters;

        // Increase final mass by calculated fluxes that leave the system (i.e., not the percolation flow)
        current_mass_meters += fluxes->et_loss_meters;
        current_mass_meters += fluxes->surface_runoff_meters_per_second * timestep_seconds;
        current_mass_meters += fluxes->soil_lateral_flow_meters_per_second * timestep_seconds;
        current_mass_meters += fluxes->groundwater_flow_meters_per_second * timestep_seconds;

        double abs_mass_diff_meters = abs(previous_mass_meters - current_mass_meters);
        return abs_mass_diff_meters > get_mass_check_error_bound() ? tshirt::TSHIRT_MASS_BALANCE_ERROR
                                                                   : tshirt::TSHIRT_NO_ERROR;
    }

    /**
     * Run the model to one time step, moving the initial `current_state` value to `previous_state` and resetting
     * other members applicable only in the context of the current time step so that they are recalculated.
     *
     * @param dt the time step size in seconds
     * @param input_flux_meters the amount water entering the system this time step, in meters
     * @return
     */
    int tshirt_model::run(double dt, double input_flux_meters, shared_ptr<pdm03_struct> et_params) {
        // Do resetting/housekeeping for new calculations
        // TODO: move this to separate small function; e.g., reset_state_members_for_next_time_step_run
        previous_state = current_state;
        current_state = make_shared<tshirt_state>(tshirt_state(0.0, 0.0, vector<double>(model_params.nash_n)));
        fluxes = make_shared<tshirt_fluxes>(tshirt_fluxes(0.0, 0.0, 0.0, 0.0, 0.0));

        double soil_column_moisture_deficit =
                model_params.max_soil_storage_meters - previous_state->soil_storage_meters;

        // Perform Schaake partitioning, passing some declared references to hold the calculated values.
        double surface_runoff, subsurface_infiltration_flux;
        Schaake_partitioning_scheme(dt, model_params.Cschaake, soil_column_moisture_deficit, input_flux_meters,
                                    &surface_runoff, &subsurface_infiltration_flux);

        double subsurface_excess, nash_subsurface_excess;
        soil_reservoir.response_meters_per_second(subsurface_infiltration_flux, dt, subsurface_excess);

        // lateral subsurface flow
        double Qlf = soil_reservoir.velocity_meters_per_second_for_outlet(lf_outlet_index);

        // percolation flow
        double Qperc = soil_reservoir.velocity_meters_per_second_for_outlet(perc_outlet_index);

        // TODO: make sure ET doesn't need to be taken out sooner
        // Get new soil storage amount calculated by reservoir
        double new_soil_storage = soil_reservoir.get_storage_height_meters();
        // Calculate and store ET
        fluxes->et_loss_meters = calc_evapotranspiration(new_soil_storage, et_params);
        // Update the current soil storage, accounting for ET
        current_state->soil_storage_meters = new_soil_storage - fluxes->et_loss_meters;

        // Cycle through lateral flow Nash cascade of nonlinear reservoirs
        // loop essentially copied from Hymod logic, but with different variable names
        for (unsigned long int i = 0; i < soil_lf_nash_res.size(); ++i) {
            // get response water velocity of nonlinear reservoir
            Qlf = soil_lf_nash_res[i]->response_meters_per_second(Qlf, dt, nash_subsurface_excess);
            // TODO: confirm this is correct
            Qlf += nash_subsurface_excess / dt;
            current_state->nash_cascade_storeage_meters[i] = soil_lf_nash_res[i]->get_storage_height_meters();
        }

        double excess_gw_water;
        fluxes->groundwater_flow_meters_per_second = groundwater_reservoir.response_meters_per_second(Qperc, dt,
                                                                                               excess_gw_water);
        // update state
        current_state->groundwater_storage_meters = groundwater_reservoir.get_storage_height_meters();

        // record other fluxes
        fluxes->soil_lateral_flow_meters_per_second = Qlf;
        fluxes->soil_percolation_flow_meters_per_second = Qperc;

        // Save "raw" runoff here and have realization class calculate GIUH surface runoff using that kernel
        // TODO: for now add this to runoff, but later adjust calculations to limit flow into reservoir to avoid excess
        fluxes->surface_runoff_meters_per_second = surface_runoff + (subsurface_excess / dt) + (excess_gw_water / dt);
        //fluxes->surface_runoff_meters_per_second = surface_runoff;

        return mass_check(input_flux_meters, dt);
    }

    /**
     * Set the mass_check_error_bound member to the absolute value of the given parameter.
     *
     * @param error_bound The value used to set the mass_check_error_bound member.
     */
    void tshirt_model::set_mass_check_error_bound(double error_bound) {
        mass_check_error_bound = error_bound >= 0 ? error_bound : abs(error_bound);
    }

}
