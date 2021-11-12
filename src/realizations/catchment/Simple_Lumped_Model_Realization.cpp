#include "Simple_Lumped_Model_Realization.hpp"

#include <cmath>

/*
Simple_Lumped_Model_Realization::Simple_Lumped_Model_Realization(
    std::string id,
    forcing_params forcing_config,
    utils::StreamHandler output_stream,
    double storage_meters,
    double gw_storage_meters,
    double gw_max_storage_meters,
    double nash_max_storage_meters,
    double smax,
    double a,
    double b,
    double Ks,
    double Kq,
    long n,
    const std::vector<double>& Sr,
    time_step_t t
  ): Catchment_Formulation(id, forcing_config, output_stream)
{
    params.gw_max_storage_meters = gw_max_storage_meters;
    params.nash_max_storage_meters = nash_max_storage_meters;
    params.min_storage_meters = 0;
    params.activation_threshold_meters_groundwater_reservoir = 0;
    params.activation_threshold_meters_nash_cascade_reservoir = 0;
    params.reservoir_max_velocity_meters_per_second = 0;
    params.smax = smax;
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
    state[0].groundwater_storage_meters = gw_storage_meters;
}

Simple_Lumped_Model_Realization::Simple_Lumped_Model_Realization(Simple_Lumped_Model_Realization && other)
:fluxes( std::move(other.fluxes) ), params( std::move(other.params) ),
cascade_backing_storage( std::move(other.cascade_backing_storage) ),
state( std::move(other.state) ), realization::Catchment_Formulation(other.get_id())
{
  this->legacy_forcing = std::move(other.legacy_forcing);
}
*/

Simple_Lumped_Model_Realization::Simple_Lumped_Model_Realization(const Simple_Lumped_Model_Realization & other)
:fluxes( other.fluxes ), params( other.params),
cascade_backing_storage( other.cascade_backing_storage ),
state( other.state ), realization::Catchment_Formulation(other.get_id())
{
  this->legacy_forcing = other.legacy_forcing;
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

double Simple_Lumped_Model_Realization::calc_et()
{
    return 0.0;
}

/**
 * Execute the backing model formulation for the given time step, where it is of the specified size, and
 * return the total discharge.
 *
 * Function reads input precipitation from ``forcing`` member variable.  It also makes use of the params struct
 * for ET params accessible via ``get_et_params``.
 *
 * @param t_index The index of the time step for which to run model calculations.
 * @param d_delta_s The duration, in seconds, of the time step for which to run model calculations.
 * @return The total discharge for this time step.
 */
double Simple_Lumped_Model_Realization::get_response(time_step_t t, time_step_t dt)
{
    //TODO input_et = this->forcing.get_et(t)
    double precip;
    time_t t_unix = this->forcing->get_forcing_output_time_begin("") + (t * 3600);
    try {
        precip = this->forcing->get_value("precip_rate", t_unix, dt, ""); // classic forcing object/format
    }
    catch (const std::exception& e){
        precip = this->forcing->get_value(CSDMS_STD_NAME_LIQUID_EQ_PRECIP_RATE, t_unix, dt, ""); // CsvPerFeatureForcingProvider
    }
    add_time(t+1, params.n);
    //FIXME should this run "daily" or hourly (t) which should really be dt
    //Do we keep an "internal dt" i.e. this->dt and reconcile with t?
    //hymod_kernel::run(68400.0, params, state[t], state[t+1], fluxes[t], precip, et_params);

    pdm03_struct params_copy = get_et_params();
    hymod_kernel::run(dt, params, state[t], state[t+1], fluxes[t], precip*dt, &params_copy);
    return fluxes[t].slow_flow_meters_per_second + fluxes[t].runoff_meters_per_second;
}

/**
 * Get a formatted line of output values for the given time step as a delimited string.
 *
 * For this type, the output consists of only the total discharge amount per time step; i.e., the same value that was
 * returned by ``get_response``.
 *
 * This method is useful for preparing calculated data in a representation useful for output files, such as
 * CSV files.
 *
 * The resulting string will contain calculated values for applicable output variables for the particular
 * formulation, as determined for the given time step.  However, the string will not contain any
 * representation of the time step itself.
 *
 * An empty string is returned if the time step value is not in the range of valid time steps for which there
 * are calculated values for all variables.
 *
 * The default delimiter is a comma.
 *
 * @param timestep The time step for which data is desired.
 * @return A delimited string with all the output variable values for the given time step.
 */
std::string Simple_Lumped_Model_Realization::get_output_line_for_timestep(int timestep, std::string delimiter) {
    if (timestep >= fluxes.size()) {
        return "";
    }
    double discharge = fluxes[timestep].slow_flow_meters_per_second + fluxes[timestep].runoff_meters_per_second;
    return std::to_string(discharge);
}

void Simple_Lumped_Model_Realization::create_formulation(geojson::PropertyMap properties) {
    this->validate_parameters(properties);

    double seconds_to_day = 3600.0/86400.0;

    double storage = properties.at("storage").as_real_number();
    double smax = properties.at("smax").as_real_number();
    double gw_storage = properties.at("gw_storage").as_real_number();
    double gw_max_storage = properties.at("gw_max_storage").as_real_number();
    double nash_max_storage = properties.at("nash_max_storage").as_real_number();
    double a = properties.at("a").as_real_number();
    double b = properties.at("b").as_real_number();
    double Ks = properties.at("Ks").as_real_number() * seconds_to_day; //Implicitly connected to time used for DAILY dt need to account for hourly dt
    double Kq = properties.at("Kq").as_real_number() * seconds_to_day; //Implicitly connected to time used for DAILY dt need to account for hourly dt
    long n = properties.at("n").as_natural_number();
    double t = properties.at("t").as_real_number();

    params.gw_max_storage_meters = gw_max_storage;
    params.nash_max_storage_meters = nash_max_storage;
    params.min_storage_meters = 0;
    params.activation_threshold_meters_groundwater_reservoir = 0;
    params.activation_threshold_meters_nash_cascade_reservoir = 0;
    params.reservoir_max_velocity_meters_per_second = 100;
    params.smax = smax;
    params.a = a;
    params.b = b;
    params.Ks = Ks;
    params.Kq = Kq;
    params.n = n;

    //Init the first time explicity using passed in data
    fluxes[0] = hymod_fluxes();

    cascade_backing_storage.emplace(0, properties.at("sr").as_real_vector()); //Move ownership of init vector to container
    state[0] = hymod_state(0.0, 0.0, cascade_backing_storage[0].data());
    state[0].storage_meters = storage;
    state[0].groundwater_storage_meters = gw_storage;
}

void Simple_Lumped_Model_Realization::create_formulation(boost::property_tree::ptree &config, geojson::PropertyMap *global) {
    geojson::PropertyMap options = this->interpret_parameters(config, global);
    create_formulation(options);
}
