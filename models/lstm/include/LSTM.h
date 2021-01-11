#ifndef LSTM_H
#define LSTM_H

#include "all.h"
#include "lstm_fluxes.h"
#include "lstm_params.h"
#include "lstm_config.h"
#include "lstm_state.h"
#include <unordered_map>

#ifdef NGEN_LSTM_TORCH_LIB_ACTIVE
#include <torch/torch.h>

typedef std::unordered_map< std::string, std::unordered_map< std::string, double> > ScaleParams;

using namespace std;

namespace lstm {

    class lstm_model {

    public:

        /** @TODO: Make option to construct model without an initial state. */

        /**
         * Constructor for model object with parameters only.
         *
         * Constructor creates a "default" initial state, with soil_storage_meters and groundwater_storage_meters set to
         * 0.0, and then otherwise proceeds as the constructor accepting the second parameter for initial state.
         *
         * @param model_params Model parameters lstm_params struct.
         */
        lstm_model(lstm_config config, lstm_params model_params);

        /**
         * Initialize LSTM Model State.
         * Reads the initial state from a specified CSV file. This function
         * might need to change if there is an option to initialize a blank
         * state.
         * @param initial_state_path
         * @return
         */
        void initialize_state(std::string initial_state_path);

        /**
         * Return the smart pointer to the lstm::lstm_model struct for holding this object's current state.
         *
         * @return The smart pointer to the lstm_model struct for holding this object's current state.
         */
        shared_ptr<lstm_state> get_current_state();

        /**
         * Return the shared pointer to the lstm::lstm_fluxes struct for holding this object's current fluxes.
         *
         * @return The shared pointer to the lstm_fluxes struct for holding this object's current fluxes.
         */
        shared_ptr<lstm_fluxes> get_fluxes();

        /**
         * Run the model to one time step, after performing initial housekeeping steps via a call to
         * `manage_state_before_next_time_step_run`.
         *
         * @param dt the time step
         * @param DLWRF_surface_W_per_meters_squared
         * @param PRES_surface_Pa
         * @param SPFH_2maboveground_kg_per_kg
         * @param precip_meters_per_second
         * @param DSWRF_surface_W_per_meters_squared 
         * @param TMP_2maboveground_K
         * @param UGRD_10maboveground_meters_per_second
         * @param VGRD_10maboveground_meters_per_second
         * @return
         */
        int run(double dt, double DLWRF_surface_W_per_meters_squared, double PRES_surface_Pa, 
                double SPFH_2maboveground_kg_per_kg, double precip_meters_per_second, 
                double DSWRF_surface_W_per_meters_squared, double TMP_2maboveground_K, 
                double UGRD_10maboveground_meters_per_second, double VGRD_10maboveground_meters_per_second);

        /**
         * Denormalizes output from LSTM model.
         *
         * @param forcing_variable_string
         * @param normalized_output
         * @return denormalized_output
         */ 
        double denormalize(std::string forcing_variable_string, double normalized_output);

        /**
         * Normalizes inputs to LSTM model.
         *
         * @param forcing_variable_string
         * @param forcing_variable
         * @return normalized_input
         */ 
        double normalize(std::string forcing_variable_string, double forcing_variable);

    protected:

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
        virtual void manage_state_before_next_time_step_run();

    private:
        /** torch confiruation variables */
        bool useGPU;
        torch::Device device;
        
        /** Model state for the "current" time step, which may not be calculated yet. */
        shared_ptr<lstm_state> current_state;
        
        /** Model execution parameters. */
        lstm_params model_params;
        
        /** Model configuration parameters */
        lstm_config config;
        
        /** Model state from that previous time step before the current. */
        shared_ptr<lstm_state> previous_state;

        /** Model fluxes */
        shared_ptr<lstm_fluxes> fluxes;

        /** torch model */
        torch::jit::script::Module model;

        /** Parameter scaling */
        ScaleParams scale;
    };
}

#endif
#endif //LSTM_H
