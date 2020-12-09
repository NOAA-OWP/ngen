#include "giuh_kernel.hpp"
#include "LSTM_Realization.hpp"
#include "lstmErrorCodes.h"
#include "Catchment_Formulation.hpp"
using namespace realization;

LSTM_Realization::LSTM_Realization(
        forcing_params forcing_config,
        utils::StreamHandler output_stream,
        double soil_storage_meters,
        double groundwater_storage_meters,
        std::string catchment_id,
        giuh::GiuhJsonReader &giuh_json_reader,
        lstm::lstm_params params,
        //const vector<double> &nash_storage,
        time_step_t t)
    : Catchment_Formulation(catchment_id, forcing_config, output_stream), catchment_id(catchment_id), dt(t)
{
    this->params = &params;
    giuh_kernel = giuh_json_reader.get_giuh_kernel_for_id(this->catchment_id);

    // If the look-up failed in the reader for some reason, and we got back a null pointer ...
    if (this->giuh_kernel == nullptr) {
        // ... revert to a pass-through kernel
        this->giuh_kernel = std::make_shared<giuh::giuh_kernel>(
                giuh::giuh_kernel(this->catchment_id, giuh_json_reader.get_associated_comid(this->catchment_id)));
    }

    //FIXME not really used, don't call???
    //add_time(t, params.nash_n);
    /////////
    //state[0] = std::make_shared<lstm::lstm_state>(lstm::lstm_state(soil_storage_meters, groundwater_storage_meters, nash_storage));
    //state[0] = std::make_shared<lstm::lstm_state>(lstm::lstm_state(0.0));
    state[0] = std::make_shared<lstm::lstm_state>(lstm::lstm_state());

    model = make_unique<lstm::lstm_model>(lstm::lstm_model(params, state[0]));
}

LSTM_Realization::LSTM_Realization(
        forcing_params forcing_config,
        utils::StreamHandler output_stream,
        double soil_storage_meters,
        double groundwater_storage_meters,
        std::string catchment_id,
        giuh::GiuhJsonReader &giuh_json_reader,


       std::string pytorch_model_path,
       std::string normalization_path,
       double latitude,
       double longitude,
       double area_square_km,

        time_step_t t
) : LSTM_Realization::LSTM_Realization(forcing_config, output_stream, soil_storage_meters, groundwater_storage_meters,
                                           catchment_id, giuh_json_reader,
                                           //lstm::lstm_params(maxsmc, wltsmc, satdk, satpsi, slope, b, multiplier,
                                           //alpha_fc, Klf, Kn, nash_n, Cgw, expon, max_gw_storage),
                                           //nash_storage, t) {

                                           lstm::lstm_params(pytorch_model_path, normalization_path, latitude, longitude, area_square_km),
                                           t) {


}
double LSTM_Realization::calc_et(double soil_m) {
    return 0;
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
double LSTM_Realization::get_response(time_step_t t_index, time_step_t t_delta_s) {
    //FIXME doesn't do anything, don't call???
    //add_time(t+1, params.nash_n);
    // TODO: this is problematic, because what happens if the wrong t_index is passed?
    //double precip = this->forcing.get_next_hourly_precipitation_meters_per_second();
    //FIXME should this run "daily" or hourly (t) which should really be dt
    //Do we keep an "internal dt" i.e. this->dt and reconcile with t?
    //int error = model->run(t_index, precip * t_delta_s / 1000, get_et_params_ptr());
    //int error = model->run(t_index);
  
    /*
    if(error == lstm::LSTM_MASS_BALANCE_ERROR){
      std::cout<<"WARNING LSTM_Realization::model mass balance error"<<std::endl;
    }
    state[t_index + 1] = model->get_current_state();
    fluxes[t_index] = model->get_fluxes();
    double giuh = giuh_kernel->calc_giuh_output(t_index, fluxes[t_index]->surface_runoff_meters_per_second);
    return fluxes[t_index]->soil_lateral_flow_meters_per_second + fluxes[t_index]->groundwater_flow_meters_per_second +
           giuh;
    */
    return 0.0;
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
std::string LSTM_Realization::get_output_line_for_timestep(int timestep, std::string delimiter) {
    if (timestep >= fluxes.size()) {
        return "";
    }

    double discharge = 0.0;
    return std::to_string(discharge);
}

void LSTM_Realization::create_formulation(geojson::PropertyMap properties) {
    this->validate_parameters(properties);

    this->catchment_id = this->get_id();
    this->dt = properties.at("timestep").as_natural_number();

    lstm::lstm_params lstm_params{
        properties.at("pytorch_model_path").as_string(),
        properties.at("normalization_path").as_string(),
        properties.at("latitude").as_real_number(),
        properties.at("longitude").as_real_number(),
        properties.at("area_square_km").as_real_number(),

    };

    this->params = &lstm_params;

    //this->state[0] = std::make_shared<lstm::lstm_state>(lstm::lstm_state(0.0));
    this->state[0] = std::make_shared<lstm::lstm_state>(lstm::lstm_state());


    this->model = make_unique<lstm::lstm_model>(lstm::lstm_model(lstm_params, this->state[0]));

}

void LSTM_Realization::create_formulation(boost::property_tree::ptree &config, geojson::PropertyMap *global) {
    geojson::PropertyMap options = this->interpret_parameters(config, global);

    this->catchment_id = this->get_id();
    this->dt = options.at("timestep").as_natural_number();

    lstm::lstm_params lstm_params{

        options.at("pytorch_model_path").as_string(),
        options.at("normalization_path").as_string(),
        options.at("latitude").as_real_number(),
        options.at("longitude").as_real_number(),
        options.at("area_square_km").as_real_number(),

    };

    this->params = &lstm_params;

    //double soil_storage_meters =  0.0; // lstm_params.max_soil_storage_meters * options.at("soil_storage_percentage").as_real_number();

    //this->state[0] = std::make_shared<lstm::lstm_state>(lstm::lstm_state(soil_storage_meters));
    this->state[0] = std::make_shared<lstm::lstm_state>(lstm::lstm_state());

    this->model = make_unique<lstm::lstm_model>(lstm::lstm_model(lstm_params, this->state[0]));

}

