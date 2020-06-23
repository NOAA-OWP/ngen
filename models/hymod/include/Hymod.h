#ifndef HYMOD_H
#define HYMOD_H

#include <cmath>
#include <vector>
#include "HymodErrorCodes.h"
#include "LinearReservoir.hpp"
#include "Nonlinear_Reservoir.hpp"
#include "Pdm03.h"
#include "hymod_params.h"
#include "hymod_state.h"
#include "hymod_fluxes.h"

//! Hymod kernel class
/*!
    This class implements the hymod hydrological model
*/
class hymod_kernel
{
    public:

    //! stub function to simulate losses due to evapotransportation
    static double calc_et(double soil_m, void* et_params)
    {
        pdm03_struct* pdm = (pdm03_struct*) et_params;
        pdm->XHuz = soil_m;
        pdm03_wrapper(pdm);

        return pdm->XHuz - soil_m;
    }

    //! run one time step of hymod
    static int run(
        double dt,
        hymod_params params,        //!< static parameters for hymod
        hymod_state state,          //!< model state
        hymod_state& new_state,     //!< model state struct to hold new model state
        hymod_fluxes& fluxes,       //!< model flux object to hold calculated fluxes
        double input_flux_meters,          //!< the amount water entering the system this time step
        void* et_params)            //!< parameters for the et function
    {

        // initalize the Nash cascade of nonlinear reservoirs
        std::vector<Nonlinear_Reservoir> nash_cascade;

        nash_cascade.resize(params.n);
        for ( unsigned long i = 0; i < nash_cascade.size(); ++i )
        {
            //construct a single outlet nonlinear reservoir
            nash_cascade[i] = Nonlinear_Reservoir(0, params.max_storage_meters, state.Sr[i], params.Kq, 1, 0, 100);
        }

        // initalize groundwater reservoir
        Nonlinear_Reservoir groundwater(0, params.max_storage_meters, state.groundwater_storage_meters, params.Ks, 1, 0, 100);

        // add flux to the current state
        state.storage_meters += input_flux_meters;

        // calculate fs, runoff and slow
        double storage_function_value = (1.0 - pow((1.0 - state.storage_meters / params.max_storage_meters), params.b) );
        double runoff_meters_per_second = storage_function_value * params.a;
        //double slow_flow_meters_per_second = storage_function_value * (1.0 - params.a );
        double soil_m = state.storage_meters - storage_function_value;

        // calculate et
        double et_meters = calc_et(soil_m, et_params);

        double groundwater_excess_meters;  //excess water from groundwater reservoir that can be positive or negative
        double excess_water_meters;        // excess water from reservoir that can be positive or negative

        // get the slow flow output for this time - ks
        double slow_flow_meters_per_second = groundwater.response_meters_per_second(
                storage_function_value * (1.0 - params.a), dt, groundwater_excess_meters);
        
        //TODO: Review issues with dt and internal timestep
        runoff_meters_per_second += groundwater_excess_meters / dt;

        // cycle through Quickflow Nash cascade of nonlinear reservoirs
        for(unsigned long int i = 0; i < nash_cascade.size(); ++i)
        {
            // get response water velocity of nonlinear reservoir
            runoff_meters_per_second = nash_cascade[i].response_meters_per_second(runoff_meters_per_second, dt, excess_water_meters);
            
            //TODO: Review issues with dt and internal timestep
            runoff_meters_per_second += excess_water_meters / dt;
        }

        // record all fluxs
        fluxes.slow_flow_meters_per_second = slow_flow_meters_per_second;
        fluxes.runoff_meters_per_second = runoff_meters_per_second;
        fluxes.et_loss_meters = et_meters;

        // update new state
        new_state.storage_meters = soil_m - et_meters;
        new_state.groundwater_storage_meters = groundwater.get_storage_height_meters();
        for ( unsigned long i = 0; i < nash_cascade.size(); ++i )
        {
            new_state.Sr[i] = nash_cascade[i].get_storage_height_meters();
        }

        return mass_check(params, state, input_flux_meters, new_state, fluxes, dt);

    }

    static int mass_check(const hymod_params& params, const hymod_state& current_state, double input_flux_meters, const hymod_state& next_state, const hymod_fluxes& calculated_fluxes, double timestep_seconds)
    {
        // initalize both mass values from current and next states storage
        double inital_mass_meters = current_state.storage_meters + current_state.groundwater_storage_meters;
        double final_mass_meters = next_state.storage_meters + next_state.groundwater_storage_meters;

        // add the masses of the reservoirs before and after the time step
        for ( int i = 0; i < params.n; ++i)
        {
            inital_mass_meters += current_state.Sr[i];
            final_mass_meters += next_state.Sr[i];
        }

        // increase the inital mass by input value
        inital_mass_meters += input_flux_meters;

        // increase final mass by calculated fluxes
        final_mass_meters += (calculated_fluxes.et_loss_meters + calculated_fluxes.runoff_meters_per_second * timestep_seconds + calculated_fluxes.slow_flow_meters_per_second * timestep_seconds);

        if ( inital_mass_meters - final_mass_meters > 0.000001 )
        {
            return MASS_BALANCE_ERROR;
        }
        else
        {
            return NO_ERROR;
        }
    }
};

extern "C"
{
    /*!
        C entry point for calling hymod_kernel::run
    */

    inline int hymod(
        double dt,                          //!< size of time step
        hymod_params params,                //!< static parameters for hymod
        hymod_state state,                  //!< model state
        hymod_state* new_state,             //!< model state struct to hold new model state
        hymod_fluxes* fluxes,               //!< model flux object to hold calculated fluxes
        double input_flux,                  //!< the amount water entering the system this time step
        void* et_params)                    //!< parameters for the et function
    {
        return hymod_kernel::run(dt, params, state, *new_state, *fluxes, input_flux, et_params);
    }
}

#endif
