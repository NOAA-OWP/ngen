#include "Simple_Lumped_Model_Realization.hpp"

#include <cmath>



Simple_Lumped_Model_Realization::Simple_Lumped_Model_Realization(
    double storage,
    double max_storage,
    double a,
    double b,
    double Ks,
    double Kq,
    long n,
    const std::vector<double>& Sr,
    time_step_t t
    )
{
    params.max_storage = max_storage;
    params.a = a;
    params.b = b;
    params.Ks = Ks;
    params.Kq = Kq;
    params.n = n;

    add_time(t, n);

    state[t].storage = storage;
    for ( int i = 0; i < n; ++i)
    {
        state[t].Sr[i] = Sr[i];
    }

}

Simple_Lumped_Model_Realization::~Simple_Lumped_Model_Realization()
{
    //dtor
}

void Simple_Lumped_Model_Realization::add_time(time_t t, double n)
{
    if ( state.find(t) != state.end() )
    {
        // create storage for fluxes
        fluxes[t] = hymod_fluxes();
        // create a safe backing array for the Sr arrays that are part of state
        cascade_backing_storage[t].resize(params.n);

        // create storage for the state

        state[t] = hymod_state(0.0,cascade_backing_storage[t].data());
    }
}

double Simple_Lumped_Model_Realization::calc_et(double soil_m, void* et_params)
{
    return 0.0;
}

double Simple_Lumped_Model_Realization::get_response(double input_flux, time_step_t t, void* et_params)
{
    add_time(t+1, params.n);
    hymod_kernel::run(params, state[t], fluxes[t-params.Ks], state[t+1], fluxes[t], input_flux, et_params);
    return fluxes[t].slow_flow_out + fluxes[t].runnoff;
}
