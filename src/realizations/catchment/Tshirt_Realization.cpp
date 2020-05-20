#include "Tshirt_Realization.hpp"

using namespace realization;

Tshirt_Realization::Tshirt_Realization(
        forcing_params forcing_config,
        double soil_storage_meters,
        double groundwater_storage_meters,
        unique_ptr<giuh::giuh_kernel> giuh_kernel,
        tshirt::tshirt_params params,
        const vector<double> &nash_storage,
        Tshirt_Realization::time_step_t t
        )
    : HY_CatchmentArea(forcing_config), giuh_kernel(move(giuh_kernel)), params(params), dt(t)
{
    //FIXME not really used, don't call???
    //add_time(t, params.nash_n);
    state[0] = std::make_shared<tshirt::tshirt_state>(tshirt::tshirt_state(soil_storage_meters, groundwater_storage_meters, nash_storage));
    //state[0]->soil_storage_meters = soil_storage_meters;
    //state[0]->groundwater_storage_meters = groundwater_storage_meters;

    for (int i = 0; i < params.nash_n; ++i) {

        state[0]->nash_cascade_storeage_meters[i] = nash_storage[i];
    }

    model = make_unique<tshirt::tshirt_model>(tshirt::tshirt_model(params, state[0]));
}

Tshirt_Realization::Tshirt_Realization(
        forcing_params forcing_config,
        double soil_storage_meters,
        double groundwater_storage_meters,
        unique_ptr<giuh::giuh_kernel> giuh_kernel,
        double maxsmc,
        double wltsmc,
        double satdk,
        double satpsi,
        double slope,
        double b,
        double multiplier,
        double alpha_fc,
        double Klf,
        double Kn,
        int nash_n,
        double Cgw,
        double expon,
        double max_gw_storage,
        const std::vector<double> &nash_storage,
        time_step_t t
) : Tshirt_Realization::Tshirt_Realization(forcing_config, soil_storage_meters, groundwater_storage_meters, move(giuh_kernel),
        tshirt::tshirt_params(maxsmc, wltsmc, satdk, satpsi, slope, b, multiplier, alpha_fc, Klf, Kn, nash_n, Cgw,
                              expon, max_gw_storage), nash_storage, t) {

}

Tshirt_Realization::~Tshirt_Realization()
{
  //destructor
}
void Tshirt_Realization::add_time(time_t t, double n) {
    // TODO: is this really needed anymore?
    if (state.find(t) != state.end()) {

        // create storage for fluxes is now done by the stateful model class, where the beginning of 'run' creates (with
        // this class's get_response method getting the pointer after run finishes)

        // create a safe backing array for the nash storage arrays that are part of state (also handled in stateful class)
        //cascade_backing_storage[t].resize(params.nash_n);

        // create storage for state handled by model run function, similarly to fluxes as described above
    }

}
double Tshirt_Realization::get_response(double input_flux, Tshirt_Realization::time_step_t t, time_step_t dt, void* et_params)
{
  return get_response(input_flux, dt, std::make_shared<pdm03_struct>( *(pdm03_struct*) et_params ));
}

double Tshirt_Realization::get_response(double input_flux, time_step_t t,
                                        const shared_ptr<pdm03_struct> &et_params) {
    //FIXME doesn't do anything, don't call???
    //add_time(t+1, params.nash_n);
    double precip = this->forcing.get_next_hourly_precipitation_meters_per_second();
    //FIXME should this run "daily" or hourly (t) which should really be dt
    //Do we keep an "internal dt" i.e. this->dt and reconcile with t?
    model->run(t, precip, et_params);
    state[t+1] = model->get_current_state();
    fluxes[t] = model->get_fluxes();

    return fluxes[t]->soil_lateral_flow_meters_per_second + fluxes[t]->groundwater_flow_meters_per_second +
           giuh_kernel->calc_giuh_output(t, fluxes[t]->surface_runoff_meters_per_second);
}
