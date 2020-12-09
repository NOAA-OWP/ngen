#include "LSTM.h"
#include "lstmErrorCodes.h"
#include "lstm_fluxes.h"
#include "lstm_state.h"
#include "CSV_Reader.h"
#include <iostream>

#include <fstream>


using namespace std;

typedef std::unordered_map< std::string, std::unordered_map< std::string, double> > ScaleParams;


ScaleParams read_scale_params(std::string path)
{
    //Read the mean and standard deviation and put them into a map
    ScaleParams params;
    CSVReader reader(path);
    auto data = reader.getData();
    std::vector<std::string> header = data[0];
    //Advance the iterator to the first data row (skip the header)
    auto row = data.begin();
    std::advance(row, 1);
    //Loop form first row to end of data
    for(; row != data.end(); ++row)
    {
	for(int i = 0; i < (*row).size(); i++)
	{   //row has var, mean, std_dev
 	    //read into map keyed on var name, param name
    	    params[ (*row)[0] ]["mean"] = strtof( (*row)[1].c_str(), NULL);
  	    params[ (*row)[0] ]["std_dev"] = strtof( (*row)[2].c_str(), NULL);
	}
    }

    return params;
}

/*
double Normalize(std::string forcing_variable_string, double forcing_variable)
{
   double temp, mean, std_dev;
   mean = scale[ forcing_variable_string ]["mean"];
   std_dev = scale[ forcing_variable_string ]["std_dev"];
   return  (forcing_variable - mean) / std_dev;
}
*/


namespace lstm {

    /**
     * Constructor for model object based on model parameters and initial state.
     *
     * @param model_params Model parameters lstm_params struct.
     * @param initial_state Shared smart pointer to lstm_state struct hold initial state values
     */
    lstm_model::lstm_model(lstm_config config, lstm_params model_params, const shared_ptr<lstm_state> &initial_state)
            : config(config), model_params(model_params), previous_state(initial_state), current_state(initial_state),
            device( torch::Device(torch::kCPU) )
    {
        // ********** Set fluxes to null for now: it is bogus until first call of run function, which initializes it
        fluxes = nullptr;
        //manually set torch seed for reproducibility
        torch::manual_seed(0);
        useGPU = torch::cuda::is_available();
        device = torch::Device( useGPU ? torch::kCUDA : torch::kCPU );
        lstm::to_device(*current_state, device);
        lstm::to_device(*previous_state, device);

        //FIXME need to initialize the model states by running a "warmup"
        //so when the model is constructed, run it on N timesteps prior to prediction period
        //OR read those states in and create the initial state tensors

         //CURRENTLY HAVING ERROR LOADING MODEL
         std::cout<<"model_params.pytorch_model_path: " << config.pytorch_model_path;


        model = torch::jit::load(config.pytorch_model_path);
        model.to( device );
        // Set to `eval` model (just like Python)
        model.eval();



        //FIXME what is the SUPPOSED to do?
        torch::NoGradGuard no_grad_;

        this->scale = read_scale_params(config.normalization_path);
        this->fluxes = std::make_shared<lstm::lstm_fluxes>(lstm::lstm_fluxes());

    }

    /**
     * Constructor for model object with parameters only.
     *
     * Constructor creates a "default" initial state, with soil_storage_meters and groundwater_storage_meters set to
     * 0.0, and then otherwise proceeds as the constructor accepting the second parameter for initial state.
     *
     * @param model_params Model parameters lstm_params struct.
     */
    lstm_model::lstm_model(lstm_config config, lstm_params model_params) :
    lstm_model(config, model_params, make_shared<lstm_state>(lstm_state())) {}

    /**
     * Return the smart pointer to the lstm::lstm_model struct for holding this object's current state.
     *
     * @return The smart pointer to the lstm_model struct for holding this object's current state.
     */
    shared_ptr<lstm_state> lstm_model::get_current_state() {
        return current_state;
    }

    /**
     * Return the shared pointer to the lstm::lstm_fluxes struct for holding this object's current fluxes.
     *
     * @return The shared pointer to the lstm_fluxes struct for holding this object's current fluxes.
     */
    shared_ptr<lstm_fluxes> lstm_model::get_fluxes() {
        return fluxes;
    }

    /**
     * Run the model to one time step, after performing initial housekeeping steps via a call to
     * `manage_state_before_next_time_step_run`.
     *
     * @param dt the time step size in seconds
     * @param input_storage_m the amount water entering the system this time step, in meters
     * @return
     */
    //int lstm_model::run(double dt, double input_storage_m, shared_ptr<pdm03_struct> et_params) {
    int lstm_model::run(double dt, double DLWRF_surface_W_per_meters_squared,
                        double PRES_surface_Pa, double SPFH_2maboveground_kg_per_kg,
                        double precip, double DSWRF_surface_W_per_meters_squared,
                        double TMP_2maboveground_K, double UGRD_10maboveground_meters_per_second,
                        double VGRD_10maboveground_meters_per_second) {
        // Do resetting/housekeeping for new calculations and new state values

        manage_state_before_next_time_step_run();

        std::vector<torch::jit::IValue> inputs;
        torch::Tensor forcing = torch::zeros({1, 11});

        forcing[0][0] = lstm_model::normalize("DLWRF_surface_W_per_meters_squared", DLWRF_surface_W_per_meters_squared);
        forcing[0][1] = lstm_model::normalize("PRES_surface_Pa", PRES_surface_Pa);
        forcing[0][2] = lstm_model::normalize("SPFH_2maboveground_kg_per_kg", SPFH_2maboveground_kg_per_kg);
        forcing[0][3] = lstm_model::normalize("Precip_rate", precip);
        forcing[0][4] = lstm_model::normalize("DSWRF_surface_W_per_meters_squared", DSWRF_surface_W_per_meters_squared);
        forcing[0][5] = lstm_model::normalize("TMP_2maboveground_K", TMP_2maboveground_K);
        forcing[0][6] = lstm_model::normalize("UGRD_10maboveground_meters_per_second", UGRD_10maboveground_meters_per_second);
        forcing[0][7] = lstm_model::normalize("VGRD_10maboveground_meters_per_second", VGRD_10maboveground_meters_per_second);
        forcing[0][8] = lstm_model::normalize("Area_Square_km", model_params.area);
        forcing[0][9] = lstm_model::normalize("Latitude", model_params.latitude);
        forcing[0][10] = lstm_model::normalize("Longitude", model_params.longitude);

        // Create the model input for one time step
      	inputs.push_back(forcing.to(device));
        inputs.push_back(previous_state->h_t);
        inputs.push_back(previous_state->c_t);
      	// Run the model
        auto output = model.forward(inputs).toTuple()->elements();
      	//Get the outputs
        double out_flow = lstm_model::denormalize( "obs", output[0].toTensor().item<double>() );
        out_flow = out_flow * 0.028316847; //convert cfs to cms
        fluxes = std::make_shared<lstm_fluxes>( lstm_fluxes( out_flow ) ) ;

        current_state = std::make_shared<lstm_state>( lstm_state(output[1].toTensor(), output[2].toTensor()) );

        return 0;
    }

    double lstm_model::denormalize(std::string forcing_variable_string, double normalized_output)
        {
          double mean, std_dev;
          mean = this->scale[ forcing_variable_string ]["mean"];
          std_dev = this->scale[ forcing_variable_string ]["std_dev"];
          return (normalized_output * std_dev) + mean;
        }

    double lstm_model::normalize(std::string forcing_variable_string, double forcing_variable)
    {
      double mean, std_dev;
      mean = this->scale[ forcing_variable_string ]["mean"];
      std_dev = this->scale[ forcing_variable_string ]["std_dev"];
      return  (forcing_variable - mean) / std_dev;
    }


    /**
     * Perform necessary steps prior to the execution of model calculations for a new time step, for managing member
     * variables that contain model state.
     *
     * This function is intended to be run only at the start of a new execution of the lstm_model::run method.  It
     * performs three housekeeping tasks needed before running the next group of time step modeling operations:
     *
     *      * the initial maintained `current_state` is moved to `previous_state`
     *      * a new `current_state` is created
     *      * a new `fluxes` is created
     */
    void lstm_model::manage_state_before_next_time_step_run()
    {
        previous_state = current_state;
        //current_state = make_shared<lstm_state>(lstm_state(0.0, 0.0, vector<double>(model_params.nash_n)));
        //fluxes = make_shared<lstm_fluxes>(lstm_fluxes(0.0, 0.0, 0.0, 0.0, 0.0));
    }

}
