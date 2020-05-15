#include "Tshirt.h"
#include "Constants.h"

namespace tshirt {

    /**
     * Calculate losses due to evapotranspiration.
     *
     * @param soil_m
     * @param et_params
     * @return
     */
    double tshirt_model::calc_evapotranspiration(double soil_m, pdm03_struct *et_params) {
        et_params->XHuz = soil_m;
        pdm03_wrapper(et_params);

        return et_params->XHuz - soil_m;
    }

    /**
     * Calculate soil field capacity storage, the level at which free drainage stops (i.e., "Sfc").
     *
     * @return
     */
    double tshirt_model::calc_soil_field_capacity_storage()
    {
        // Calculate the suction head above water table (Hwt)
        double head_above_water_table =
                model_params.alpha_fc * ((double) ATMOSPHERIC_PRESSURE_PASCALS / WATER_SPECIFIC_WEIGHT);
        // TODO: account for possibility of Hwt being less than 0.5 (though initially, it looks like this will never be the case)

        double z1 = head_above_water_table - 0.5;
        double z2 = z1 + 2;

        // Note that z^( 1 - (1/b) ) / (1 - (1/b)) == b * (z^( (b - 1) / b ) / (b - 1)
        return model_params.maxsmc * pow((1.0 / model_params.satpsi), (-1.0 / model_params.b)) *
               ((model_params.b * pow(z2, ((model_params.b - 1) / model_params.b)) / (model_params.b - 1)) -
                (model_params.b * pow(z1, ((model_params.b - 1) / model_params.b)) / (model_params.b - 1)));
    }

    /**
     * Run the model to one time step, moving the initial `current_state` value to `previous_state` and resetting
     * other members applicable only to in the context of the current time step so that they are recalculated.
     *
     * @param dt the time step
     * @param input_flux_meters the amount water entering the system this time step, in meters
     * @return
     */
    int tshirt_model::run(double dt, double input_flux_meters, pdm03_struct *et_params) {
        // TODO: think about keep the old previous_state somewhere?
        // Do resetting/housekeeping for new calculations
        previous_state = current_state;
        current_state = make_shared<tshirt_state>(tshirt_state(0.0, 0.0));
        fluxes = make_shared<tshirt_fluxes>(tshirt_fluxes(0.0, 0.0, 0.0, 0.0, 0.0));

        double soil_column_moisture_deficit =
                model_params.max_soil_storage_meters - previous_state->soil_storage_meters;

        double surface_runoff, subsurface_infiltration_flux;
        Schaake_partitioning_scheme(dt, model_params.Cschaake, soil_column_moisture_deficit, input_flux_meters,
                                    &surface_runoff, &subsurface_infiltration_flux);

        // TODO: the activation thresholds for the soil reservoir outlets are set to the Sfc value, which is dependent
        //  on state. Thus, to operate in the same way the static version did, the outlets need to be updated here.

        double subsurface_excess;
        soil_reservoir.response_meters_per_second(subsurface_infiltration_flux, dt, subsurface_excess);

        // lateral subsurface flow
        double Qlf = soil_reservoir.velocity_meters_per_second_for_outlet(lf_outlet_index);

        // percolation flow
        double Qperc = soil_reservoir.velocity_meters_per_second_for_outlet(perc_outlet_index);

        // TODO: make sure ET doesn't need to be taken out sooner
        double new_soil_storage = soil_reservoir.get_storage_height_meters();
        fluxes->et_loss_meters = calc_evapotranspiration(new_soil_storage, et_params);
        current_state->soil_storage_meters = new_soil_storage - fluxes->et_loss_meters;

        // TODO: update the activation with the new Sfc value (needs support within NonLinear_Reservoir class), as with
        //  the main soil reservoir object above.
        // TODO: potentially be able to update the max flow also (currently always just max_lateral_flow)
        //for (unsigned long i = 0; i < get_soil_lf_nash_res()->size(); ++i) {
        //    get_soil_lf_nash_res()[i]->update_activation(*get_soil_field_capacity_storage());
        //}

        // cycle through lateral flow Nash cascade of nonlinear reservoirs
        // loop essentially copied from Hymod logic, but with different variable names
        for (unsigned long int i = 0; i < soil_lf_nash_res.size(); ++i) {
            // get response water velocity of nonlinear reservoir
            Qlf = soil_lf_nash_res[i]->response_meters_per_second(Qlf, dt, subsurface_excess);
            // TODO: confirm this is correct
            Qlf += subsurface_excess / dt;
            current_state->nash_cascade_storeage_meters[i] = soil_lf_nash_res[i]->get_storage_height_meters();
        }

        // "raw" GW calculations
        //state.groundwater_storage_meters += soil_percolation_flow_meters_per_second * dt;
        //double groundwater_flow_meters_per_second = params.Cgw * ( exp(params.expon * state.groundwater_storage_meters / params.max_groundwater_storage_meters) - 1 );

        // TODO: verify activation threshold for groundwater reservoir should always be 0 (as it is when initialized)
        // Right now, the groundwater reservoir activation threshold doesn't change, but if that is incorrect, it should
        // be updated here in the same way as above for the soil reservoir.

        // TODO: what needs to be done with this value?
        double excess_gw_water;
        fluxes->groundwater_flow_meters_per_second = groundwater_reservoir.response_meters_per_second(Qperc, dt,
                                                                                               excess_gw_water);
        // update state
        current_state->groundwater_storage_meters = groundwater_reservoir.get_storage_height_meters();

        // record other fluxes
        fluxes->soil_lateral_flow_meters_per_second = Qlf;
        fluxes->soil_percolation_flow_meters_per_second = Qperc;

        // Save "raw" runoff here and have realization class calculate GIUH surface runoff
        //fluxes->surface_runoff_meters_per_second = giuh_obj->calc_giuh_output(dt, surface_runoff);

        // TODO: for now add this to runoff, but later adjust calculations to limit flow into reservoir to avoid excess
        fluxes->surface_runoff_meters_per_second = surface_runoff + excess_gw_water;
        //fluxes->surface_runoff_meters_per_second = surface_runoff;

        return 0;
    }

}