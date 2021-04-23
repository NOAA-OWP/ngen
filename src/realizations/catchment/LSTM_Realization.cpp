#include "LSTM_Realization.hpp"
#include "Catchment_Formulation.hpp"
#include "CSV_Reader.h"

#ifdef NGEN_LSTM_TORCH_LIB_ACTIVE

using namespace realization;

/**
 * Parameterized constructor for LSTM Realization with lstm_params and lstm_config
 * already constructed.
 *
 * @param forcing_config
 * @param output_stream
 * @param catchment_id
 * @param params
 * @param config
 */
LSTM_Realization::LSTM_Realization(
        forcing_params forcing_config,
        utils::StreamHandler output_stream,
        std::string catchment_id,
        lstm::lstm_params params,
        lstm::lstm_config config)
    : Catchment_Formulation(catchment_id, forcing_config, output_stream),
      catchment_id(catchment_id), params(params), config(config)
{
    model = make_unique<lstm::lstm_model>(lstm::lstm_model(config, params));
}

/**
 * Parameterized constructor for LSTM Realization with individual paths and parameters
 * needed lstm_params and lstm_config passed.
 *
 * @param forcing_config
 * @param output_stream
 * @param catchment_id
 * @param pytorch_model_path
 * @param normalization_path
 * @param initial_state_path
 * @param latitude
 * @param longitude
 * @param area_square_km
 */
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
                                         lstm::lstm_config(pytorch_model_path, normalization_path, initial_state_path, false)
                                         ) {

}

/**
 * Execute the backing model formulation for the given time step, where it is of the specified size, and
 * return the total discharge.
 *
 * Function reads multiple inputs from ``forcing`` member variable.
 *
 * @param t_index The index of the time step for which to run model calculations.
 * @param d_delta_s The duration, in seconds, of the time step for which to run model calculations.
 * @return The total discharge for this time step.
 */
double LSTM_Realization::get_response(time_step_t t_index, time_step_t t_delta_s) {

    double precip = this->forcing.get_next_hourly_precipitation_meters_per_second();

    AORC_data forcing_data = this->forcing.get_AORC_data();
    int error = model->run(t_delta_s, forcing_data.DLWRF_surface_W_per_meters_squared, 
                           forcing_data.PRES_surface_Pa, forcing_data.SPFH_2maboveground_kg_per_kg, 
                           precip, forcing_data.DSWRF_surface_W_per_meters_squared, 
                           forcing_data.TMP_2maboveground_K, forcing_data.UGRD_10maboveground_meters_per_second, 
                           forcing_data.VGRD_10maboveground_meters_per_second);

    return model->get_fluxes()->flow;
}

/** @TODO: Consider updating the below function to match the Tshirt realization and be able to return the
    flux for a given time step. */
/**
 * Get a formatted line of output values for the current time step only, regardless of the time step given,
 * as a delimited string. This function for this LSTM realization can later be updated to match the Tshirt
 * realization and be able to return the flux for the given time step.
 *
 * For this type, the output consists of only the total discharge amount per time step; i.e., the same value
 * that was returned by ``get_response``.
 *
 * This method is useful for preparing calculated data in a representation useful for output files, such as
 * CSV files.
 *
 * The resulting string will contain calculated values for applicable output variables for the particular
 * formulation, as determined for the current time step.  However, the string will not contain any
 * representation of the time step itself.
 *
 * An empty string is returned if the time step value is not in the range of valid time steps for which there
 * are calculated values for all variables.
 *
 * The default delimiter is a comma.
 *
 * @param timestep The time step for which data is desired. (Not currently applicable for this realization)
 * @return A delimited string with all the output variable values for the given time step.
 */
std::string LSTM_Realization::get_output_line_for_timestep(int timestep, std::string delimiter) {
    return std::to_string(model->get_fluxes()->flow);
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
      properties.at("initial_state_path").as_string(),
      properties.at("useGPU").as_boolean()
    };

    this->params = lstm_params;
    this->config = config;
    this->model = make_unique<lstm::lstm_model>(lstm::lstm_model(config, lstm_params));
}

void LSTM_Realization::create_formulation(boost::property_tree::ptree &config, geojson::PropertyMap *global) {
    geojson::PropertyMap options = this->interpret_parameters(config, global);
    create_formulation(options);
}

#endif
