#include "Simple_Lumped_Model_Realization.hpp"

#include <cmath>

Simple_Lumped_Model_Realization::Simple_Lumped_Model_Realization(
    forcing_params forcing_config,
    utils::StreamHandler output_stream,
    double storage_meters,
    double max_storage_meters,
    double a,
    double b,
    double Ks,
    double Kq,
    long n,
    const std::vector<double>& Sr,
    time_step_t t
  ):HY_CatchmentArea(forcing_config, output_stream)
{
    params.max_storage_meters = max_storage_meters;
    params.a = a;
    params.b = b;
    params.Ks = Ks;
    params.Kq = Kq;
    params.n = n;

    //Init the first time explicity using passed in data
    fluxes[0] = hymod_fluxes();
    cascade_backing_storage.emplace(0, Sr); //Move ownership of init vector to container
    state[0] = hymod_state(0.0, 0.0, cascade_backing_storage[0].data());
    state[0].storage_meters = storage_meters;
}

Simple_Lumped_Model_Realization::Simple_Lumped_Model_Realization(Simple_Lumped_Model_Realization && other)
:fluxes( std::move(other.fluxes) ), params( std::move(other.params) ),
cascade_backing_storage( std::move(other.cascade_backing_storage) ),
state( std::move(other.state) )
{
  this->forcing = std::move(other.forcing);
}

Simple_Lumped_Model_Realization::Simple_Lumped_Model_Realization(const Simple_Lumped_Model_Realization & other)
:fluxes( other.fluxes ), params( other.params),
cascade_backing_storage( other.cascade_backing_storage ),
state( other.state )
{
  this->forcing = other.forcing;
  //rehook state.Sr* -> cascade_backing_storage
  for(auto &s : state)
  {
    s.second.Sr = cascade_backing_storage[s.first].data();
  }
}

Simple_Lumped_Model_Realization::~Simple_Lumped_Model_Realization()
{
    //dtor
}

void Simple_Lumped_Model_Realization::add_time(time_t t, double n)
{
    if ( state.find(t) == state.end() )
    {
        // create storage for fluxes
        fluxes[t] = hymod_fluxes();
        // create a safe backing array for the Sr arrays that are part of state
        cascade_backing_storage[t].resize(params.n);

        // create storage for the state

        state[t] = hymod_state(0.0, 0.0, cascade_backing_storage[t].data());
    }
}

double Simple_Lumped_Model_Realization::calc_et(double soil_m, void* et_params)
{
    return 0.0;
}


double Simple_Lumped_Model_Realization::get_response(double input_flux, time_step_t t, time_step_t dt, void* et_params)
{
    //TODO input_et = this->forcing.get_et(t)
    double precip = this->forcing.get_next_hourly_precipitation_meters_per_second();
    add_time(t+1, params.n);
    //FIXME should this run "daily" or hourly (t) which should really be dt
    //Do we keep an "internal dt" i.e. this->dt and reconcile with t?
    //hymod_kernel::run(68400.0, params, state[t], state[t+1], fluxes[t], precip, et_params);
    hymod_kernel::run(dt, params, state[t], state[t+1], fluxes[t], precip, et_params);
    return fluxes[t].slow_flow_meters_per_second + fluxes[t].runoff_meters_per_second;
}
