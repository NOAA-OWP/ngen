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
    //state = std::make_shared<lstm::lstm_state>(lstm::lstm_state());


//Can remove state and fluxes init here
    //model = make_unique<lstm::lstm_model>(lstm::lstm_model(config, params, state));
    model = make_unique<lstm::lstm_model>(lstm::lstm_model(config, params));

    //fluxes = std::make_shared<lstm::lstm_fluxes>(lstm::lstm_fluxes());
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
    int error = model->run(t_index, forcing_data.DLWRF_surface_W_per_meters_squared, forcing_data.PRES_surface_Pa, forcing_data.SPFH_2maboveground_kg_per_kg, precip, forcing_data.DSWRF_surface_W_per_meters_squared, forcing_data.TMP_2maboveground_K, forcing_data.UGRD_10maboveground_meters_per_second, forcing_data.VGRD_10maboveground_meters_per_second);

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
//    if( fluxes == nullptr )
//      return "";
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
    vector<double> h_vec;
    vector<double> c_vec;
    //std::unordered_map< std::string, double> h_map;
    //std::unordered_map< std::string, double> c_map;


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
    //FIXME what is going on with state/fluxes!!!!!!!!
    //this->state = std::make_shared<lstm::lstm_state>(lstm::lstm_state(h_vec, c_vec));    
    //this->model = make_unique<lstm::lstm_model>(lstm::lstm_model(config, lstm_params, this->state));
    this->model = make_unique<lstm::lstm_model>(lstm::lstm_model(config, lstm_params));


//    this->fluxes = std::make_shared<lstm::lstm_fluxes>(lstm::lstm_fluxes());
}

void LSTM_Realization::create_formulation(boost::property_tree::ptree &config, geojson::PropertyMap *global) {
    geojson::PropertyMap options = this->interpret_parameters(config, global);
    create_formulation(options);
}
