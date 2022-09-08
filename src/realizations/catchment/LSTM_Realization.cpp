#include "LSTM_Realization.hpp"
#include "Catchment_Formulation.hpp"
#include "CSV_Reader.h"

#ifdef NGEN_LSTM_TORCH_LIB_ACTIVE

using namespace realization;

/**
 * Parameterized constructor for LSTM Realization with lstm_params and lstm_config
 * already constructed.
 *
 * @param catchment_id
 * @param forcing
 * @param output_stream
 * @param params
 * @param config
 */
LSTM_Realization::LSTM_Realization(
        std::string id,
        std::shared_ptr<data_access::GenericDataProvider> gdp,
        utils::StreamHandler output_stream,
        lstm::lstm_params params,
        lstm::lstm_config config
    ) : Catchment_Formulation(id, gdp, output_stream), params(params), config(config) {
        model = make_unique<lstm::lstm_model>(lstm::lstm_model(config, params));
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

    //Checking the time step used is consistent with that provided in forcing data
    time_t t_delta = this->forcing->record_duration();
    if (t_delta != t_delta_s) {    //Checking the time step used is consistent with that provided in forcing data
        throw std::invalid_argument("Getting response using insonsistent time step with provided forcing data");
    }
    //Negative t_index is not allowed
    if (t_index < 0) {
        throw std::invalid_argument("Getting response of negative time step in Tshirt C Realization is not allowed.");
    }
    time_t start_time = this->forcing->get_data_start_time();
    time_t stop_time = this->forcing->get_data_stop_time();
    time_t t_current = start_time + t_index * t_delta_s;
    //Ensure model run does not exceed the end time of forcing
    if (t_current > stop_time) {
        throw std::invalid_argument("Getting response beyond time with available forcing.");
    }

    int error = model->run(t_delta_s, 
        this->forcing->get_value(CatchmentAggrDataSelector(this->catchment_id, CSDMS_STD_NAME_SOLAR_LONGWAVE, t_current, t_delta_s, ""), data_access::MEAN), 
        this->forcing->get_value(CatchmentAggrDataSelector(this->catchment_id, CSDMS_STD_NAME_SURFACE_AIR_PRESSURE, t_current, t_delta_s, ""), data_access::MEAN), 
        this->forcing->get_value(CatchmentAggrDataSelector(this->catchment_id, NGEN_STD_NAME_SPECIFIC_HUMIDITY, t_current, t_delta_s, ""), data_access::MEAN), 
        this->forcing->get_value(CatchmentAggrDataSelector(this->catchment_id, CSDMS_STD_NAME_LIQUID_EQ_PRECIP_RATE, t_current, t_delta_s, ""), data_access::SUM),
        this->forcing->get_value(CatchmentAggrDataSelector(this->catchment_id, CSDMS_STD_NAME_SOLAR_SHORTWAVE, t_current, t_delta_s, ""), data_access::MEAN), 
        this->forcing->get_value(CatchmentAggrDataSelector(this->catchment_id, CSDMS_STD_NAME_SURFACE_TEMP, t_current, t_delta_s, ""), data_access::MEAN), 
        this->forcing->get_value(CatchmentAggrDataSelector(this->catchment_id, CSDMS_STD_NAME_WIND_U_X, t_current, t_delta_s, ""), data_access::MEAN), 
        this->forcing->get_value(CatchmentAggrDataSelector(this->catchment_id, CSDMS_STD_NAME_WIND_V_Y, t_current, t_delta_s, ""), data_access::MEAN)
        );

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
