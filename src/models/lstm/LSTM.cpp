#include "LSTM.h"
#include "lstm_fluxes.h"
#include "lstm_state.h"
#include "CSV_Reader.h"
#include <iostream>
#include <fstream>

#ifdef NGEN_LSTM_TORCH_LIB_ACTIVE


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
    //Loop from first row to end of data
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

namespace lstm {

    /** @TODO: Make option to construct model without an initial state.
     *  Consider initializing the empty state with: 
     *  lstm_model(config, model_params, make_shared<lstm_state>(lstm_state())) {}     
     */

    /**
     * Constructor for model object based on model parameters only.
     *
     * @param model_params Model parameters lstm_params struct.
     */
    lstm_model::lstm_model(lstm_config config, lstm_params model_params)
            : config(config), model_params(model_params),
            device( torch::Device(torch::kCPU) )
    {
        // ********** Set fluxes to null for now: it is bogus until first call of run function, which initializes it
        fluxes = nullptr;
        useGPU = config.useGPU && torch::cuda::is_available();
        device = torch::Device( useGPU ? torch::kCUDA : torch::kCPU );

        lstm_model::initialize_state(config.initial_state_path.c_str());
 
        //FIXME need to initialize the model states by running a "warmup"
        //so when the model is constructed, run it on N timesteps prior to prediction period
        //OR read those states in and create the initial state tensors

        //std::cout<<"model_params.pytorch_model_path: " << config.pytorch_model_path;

        model = torch::jit::load(config.pytorch_model_path);
        model.to( device );
        // Set to `eval` model (just like Python)
        model.eval();

        //no_grad disables gradient calculations on the tensors.
        //Since gradient calculations are not needed on the forward pass,
        //no_grad reduces memory consumption.
        torch::NoGradGuard no_grad_;

        this->scale = read_scale_params(config.normalization_path);
        this->fluxes = std::make_shared<lstm::lstm_fluxes>(lstm::lstm_fluxes());

    }

    /**
     * Initialize LSTM Model State.
     * Reads the initial state from a specified CSV file. This function
     * might need to change if there is an option to initialize a blank
     * state.
     * @param initial_state_path 
     * @return
     */
    void lstm_model::initialize_state(std::string initial_state_path)
    {

        vector<double> h_vec;
        vector<double> c_vec;
        CSVReader reader(initial_state_path);
        auto data = reader.getData();
        std::vector<std::string> header = data[0];
        //Advance the iterator to the first data row (skip the header)
        auto row = data.begin();
        std::advance(row, 1);
        //Loop form first row to end of data
       
        //Check to ensure that there are 2 columns 
        if(row[0].size() != 2)
        { 
            throw std::runtime_error("ERROR: LSTM Model requires two columns for the initial states input.");
        }

        for(; row != data.end(); ++row)
        {
            h_vec.push_back( std::strtof( (*row)[0].c_str(), NULL ) );
            c_vec.push_back( std::strtof( (*row)[1].c_str(), NULL ) );  
        }

        current_state = std::make_shared<lstm_state>(lstm_state(h_vec, c_vec));
        previous_state = std::make_shared<lstm_state>(lstm_state(h_vec, c_vec));

        lstm::to_device(*current_state, device);
        lstm::to_device(*previous_state, device);

        return;
    }

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
     * @param DLWRF_surface_W_per_meters_squared
     * @param PRES_surface_Pa
     * @param SPFH_2maboveground_kg_per_kg
     * @param precip_meters_per_second
     * @param DSWRF_surface_W_per_meters_squared
     * @param TMP_2maboveground_K
     * @param UGRD_10maboveground_meters_per_second
     * @param VGRD_10maboveground_meters_per_second  
     * @return Returns 0 if the function successfully completes. 
     *         Any other value returned means an error occurred. 
     */
    int lstm_model::run(double dt, double DLWRF_surface_W_per_meters_squared,
                        double PRES_surface_Pa, double SPFH_2maboveground_kg_per_kg,
                        double precip_meters_per_second, double DSWRF_surface_W_per_meters_squared,
                        double TMP_2maboveground_K, double UGRD_10maboveground_meters_per_second,
                        double VGRD_10maboveground_meters_per_second) {
        // Do resetting/housekeeping for new calculations and new state values

        manage_state_before_next_time_step_run();

        std::vector<torch::jit::IValue> inputs;
        torch::Tensor forcing = torch::zeros({1, 11});

        forcing[0][0] = lstm_model::normalize("Precip_rate", precip_meters_per_second);
        forcing[0][1] = lstm_model::normalize("SPFH_2maboveground_kg_per_kg", SPFH_2maboveground_kg_per_kg);
        forcing[0][2] = lstm_model::normalize("TMP_2maboveground_K", TMP_2maboveground_K);
        forcing[0][3] = lstm_model::normalize("DLWRF_surface_W_per_meters_squared", DLWRF_surface_W_per_meters_squared);
        forcing[0][4] = lstm_model::normalize("DSWRF_surface_W_per_meters_squared", DSWRF_surface_W_per_meters_squared);
        forcing[0][5] = lstm_model::normalize("PRES_surface_Pa", PRES_surface_Pa);
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

    /**
     * Denormalizes output from LSTM model.
     *
     * @param forcing_variable_string
     * @param normalized_output
     * @return denormalized_output
     */
    double lstm_model::denormalize(std::string forcing_variable_string, double normalized_output)
    {
        double mean, std_dev;
        mean = this->scale[ forcing_variable_string ]["mean"];
        std_dev = this->scale[ forcing_variable_string ]["std_dev"];
        return (normalized_output * std_dev) + mean;
    }

    /**
     * Normalizes inputs to LSTM model.
     *
     * @param forcing_variable_string
     * @param forcing_variable
     * @return normalized_input
     */
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
    }

}

#endif

