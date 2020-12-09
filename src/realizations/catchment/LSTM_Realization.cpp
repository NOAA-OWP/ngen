#include "LSTM_Realization.hpp"
#include "Catchment_Formulation.hpp"
#include "CSV_Reader.h"

using namespace realization;

LSTM_Realization::LSTM_Realization(
        forcing_params forcing_config,
        utils::StreamHandler output_stream,
        std::string catchment_id,
        lstm::lstm_params params,
        lstm::lstm_config config)
    : Catchment_Formulation(catchment_id, forcing_config, output_stream),
      catchment_id(catchment_id), params(params), config(config)
{
    state = std::make_shared<lstm::lstm_state>(lstm::lstm_state());

    model = make_unique<lstm::lstm_model>(lstm::lstm_model(config, params, state));

    fluxes = std::make_shared<lstm::lstm_fluxes>(lstm::lstm_fluxes());
}

LSTM_Realization::LSTM_Realization(
        forcing_params forcing_config,
        utils::StreamHandler output_stream,
        std::string catchment_id,
        std::string pytorch_model_path,
        std::string normalization_path,
        std::string initial_state_path,
        double latitude,
        double longitude,
        double area_square_km)
    : LSTM_Realization::LSTM_Realization(forcing_config, output_stream,
                                           catchment_id,
                                           lstm::lstm_params(latitude, longitude, area_square_km),
                                           lstm::lstm_config(pytorch_model_path, normalization_path, initial_state_path)
                                           ) {

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
    //FIXME has to get N previous timesteps of forcing to pass to the model
    //FIXME doesn't do anything, don't call???
    //add_time(t+1, params.nash_n);
    // TODO: this is problematic, because what happens if the wrong t_index is passed?
    double precip = this->forcing.get_next_hourly_precipitation_meters_per_second();
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

    AORC_data forcing_data = this->forcing.get_AORC_data();

    int error = model->run(t_index, forcing_data.DLWRF_surface_W_per_meters_squared, forcing_data.PRES_surface_Pa, forcing_data.SPFH_2maboveground_kg_per_kg, precip, forcing_data.DSWRF_surface_W_per_meters_squared, forcing_data.TMP_2maboveground_K, forcing_data.UGRD_10maboveground_meters_per_second, forcing_data.VGRD_10maboveground_meters_per_second);

    return model->get_fluxes()->flow;
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
    /*if (timestep >= fluxes.size()) {
        return "";
    }*/
    if( fluxes == nullptr )
      std::cout<<"NULL POINTER\n";
    return std::to_string(fluxes->flow);
}

void LSTM_Realization::create_formulation(geojson::PropertyMap properties) {
    this->validate_parameters(properties);

    this->catchment_id = this->get_id();

    lstm::lstm_params lstm_params{
        properties.at("latitude").as_real_number(),
        properties.at("longitude").as_real_number(),
        properties.at("area_square_km").as_real_number(),

    };
    lstm::lstm_config config{
      properties.at("pytorch_model_path").as_string(),
      properties.at("normalization_path").as_string(),
      properties.at("initial_state_path").as_string()
    };

    this->params = lstm_params;
    this->config = config;
    vector<double> h_vec;
    vector<double> c_vec;
    //FIXME decide on best place to read this initial state
    // Confirm data JSON file exists and is readable
    if (FILE *file = fopen(config.initial_state_path.c_str(), "r")) {
        fclose(file);
        CSVReader reader(config.initial_state_path);
        auto data = reader.getData();
        std::vector<std::string> header = data[0];
        //Advance the iterator to the first data row (skip the header)
        auto row = data.begin();
        std::advance(row, 1);
        //Loop form first row to end of data
        //FIXME better map header/name and row[index]
        for(; row != data.end(); ++row)
        {
          h_vec.push_back( std::strtof( (*row)[0].c_str(), NULL ) );
          c_vec.push_back( std::strtof( (*row)[1].c_str(), NULL ) );
        }

    } else {
        throw std::runtime_error("LSTM initial state path: "+config.initial_state_path+" does not exist.");
    }
    this->state = std::make_shared<lstm::lstm_state>(lstm::lstm_state(h_vec, c_vec));
    this->model = make_unique<lstm::lstm_model>(lstm::lstm_model(config, lstm_params, this->state));
    this->fluxes = std::make_shared<lstm::lstm_fluxes>(lstm::lstm_fluxes());
}

void LSTM_Realization::create_formulation(boost::property_tree::ptree &config, geojson::PropertyMap *global) {
    geojson::PropertyMap options = this->interpret_parameters(config, global);
    create_formulation(options);
}
