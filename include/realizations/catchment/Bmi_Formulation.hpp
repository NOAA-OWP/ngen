#ifndef NGEN_BMI_FORMULATION_HPP
#define NGEN_BMI_FORMULATION_HPP

#include <string>
#include <utility>
#include <vector>
#include "Catchment_Formulation.hpp"
#include "ForcingProvider.hpp"

// Define the configuration parameter names used in the realization/formulation config JSON file
// First the required:
#define BMI_REALIZATION_CFG_PARAM_REQ__INIT_CONFIG "init_config"
#define BMI_REALIZATION_CFG_PARAM_REQ__MAIN_OUT_VAR "main_output_variable"
#define BMI_REALIZATION_CFG_PARAM_REQ__MODEL_TYPE "model_type_name"
#define BMI_REALIZATION_CFG_PARAM_REQ__USES_FORCINGS "uses_forcing_file"

// Then the optional
#define BMI_REALIZATION_CFG_PARAM_OPT__FORCING_FILE "forcing_file"
#define BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "variables_names_map"
#define BMI_REALIZATION_CFG_PARAM_OPT__OUT_VARS "output_variables"
#define BMI_REALIZATION_CFG_PARAM_OPT__OUT_HEADER_FIELDS "output_header_fields"
#define BMI_REALIZATION_CFG_PARAM_OPT__ALLOW_EXCEED_END "allow_exceed_end_time"
#define BMI_REALIZATION_CFG_PARAM_OPT__FIXED_TIME_STEP "fixed_time_step"
#define BMI_REALIZATION_CFG_PARAM_OPT__LIB_FILE "library_file"
#define BMI_REALIZATION_CFG_PARAM_OPT__REGISTRATION_FUNC "registration_function"

// Supported Standard Names for BMI variables
// This is needed to provide a calculated potential ET value back to a BMI model
#define NGEN_STD_NAME_POTENTIAL_ET_FOR_TIME_STEP "potential_evapotranspiration"

// Taken from the CSDMS Standard Names list
// TODO: need to add these in for anything BMI model input or output variables we need to know how to recognize
#define CSDMS_STD_NAME_RAIN_RATE "atmosphere_water__rainfall_volume_flux"
#define CSDMS_STD_NAME_POTENTIAL_ET "water_potential_evaporation_flux"

// Forward declaration to provide access to protected items in testing
class Bmi_Formulation_Test;
class Bmi_C_Formulation_Test;
class Bmi_C_Cfe_IT;

using namespace std;

namespace realization {

    /**
     * Abstraction of formulation with backing model object(s) that implements the BMI.
     */
    class Bmi_Formulation : public Catchment_Formulation, public forcing::ForcingProvider {

    public:

        // TODO: probably need to adjust this to let the forcing param (and backing member) be a shared pointer
        Bmi_Formulation(std::string id, Forcing forcing, utils::StreamHandler output_stream)
                : Catchment_Formulation(std::move(id), forcing, output_stream) { };

        /**
         * Minimal constructor for objects initialize using the Formulation_Manager and subsequent calls to
         * ``create_formulation``.
         *
         * @param id
         * @param forcing_config
         * @param output_stream
         */
        Bmi_Formulation(std::string id, forcing_params forcing_config, utils::StreamHandler output_stream)
        : Catchment_Formulation(std::move(id), std::move(forcing_config), output_stream) { };

        virtual ~Bmi_Formulation() {};

        /**
         * Get whether a model may perform updates beyond its ``end_time``.
         *
         * Get whether model ``Update`` calls are allowed and handled in some way by the backing model for time steps
         * after the model's ``end_time``.   Implementations of this type should use this function to safeguard against
         * entering either an invalid or otherwise undesired state as a result of attempting to process a model beyond
         * its available data.
         *
         * As mentioned, even for models that are capable of validly handling processing beyond end time, it may be
         * desired that they do not for some reason (e.g., the way they account for the lack of input data leads to
         * valid but incorrect results for a specific application).  Because of this, whether models are allowed to
         * process beyond their end time is configuration-based.
         *
         * @return Whether a model may perform updates beyond its ``end_time``.
         */
        virtual const bool &get_allow_model_exceed_end_time() const = 0;

        virtual const vector<string> get_bmi_input_variables() = 0;

        const string &get_bmi_main_output_var() const {
            return bmi_main_output_var;
        }

        virtual const time_t &get_bmi_model_start_time_forcing_offset_s() = 0;

        virtual const vector<string> get_bmi_output_variables() = 0;

        /**
         * When possible, translate a variable name for a BMI model to an internally recognized name.
         *
         * @param model_var_name The BMI variable name to translate so its purpose is recognized internally.
         * @return Either the translated equivalent variable name, or the provided name if there is not a mapping entry.
         */
        virtual const std::string &get_config_mapped_variable_name(const std::string &model_var_name) = 0;

        virtual const string &get_forcing_file_path() const = 0;

        /**
         * Get the name of the specific type of the backing model object.
         *
         * @return The name of the backing model object's type.
         */
        std::string get_model_type_name() {
            return model_type_name;
        }

        /**
         * Get the values making up the header line from get_output_header_line(), but organized as a vector of strings.
         *
         * @return The values making up the header line from get_output_header_line() organized as a vector.
         */
        const vector<std::string> &get_output_header_fields() const {
            return output_header_fields;
        }

        /**
         * Get the names of variables in formulation output.
         *
         * Get the names of the variables to include in the output from this formulation, which should be some ordered
         * subset of the output variables from the model.
         *
         * @return
         */
        virtual const vector<std::string> &get_output_variable_names() const = 0;

        const vector<std::string> &get_required_parameters() override {
            return REQUIRED_PARAMETERS;
        }

        virtual bool is_bmi_input_variable(const string &var_name) = 0;

        /**
         * Test whether backing model has fixed time step size.
         *
         * @return Whether backing model has fixed time step size.
         */
        virtual bool is_bmi_model_time_step_fixed() = 0;

        virtual bool is_bmi_output_variable(const string &var_name) = 0;

        /**
         * Whether the backing model uses/reads the forcing file directly for getting input data.
         *
         * @return Whether the backing model uses/reads the forcing file directly for getting input data.
         */
        virtual bool is_bmi_using_forcing_file() const = 0;

        /**
         * Test whether the backing model has been initialize using the BMI standard ``Initialize`` function.
         *
         * @return Whether backing model has been initialize using the BMI standard ``Initialize`` function.
         */
        virtual bool is_model_initialized() = 0;

    protected:

        void set_bmi_main_output_var(const string &main_output_var) {
            bmi_main_output_var = main_output_var;
        }

        /**
         * Set the name of the specific type of the backing model object.
         *
         * @param type_name The name of the backing model object's type.
         */
        virtual void set_model_type_name(std::string type_name) {
            model_type_name = std::move(type_name);
        }

        void set_output_header_fields(const vector<std::string> &output_headers) {
            output_header_fields = output_headers;
        }

    private:

        std::string bmi_main_output_var;
        std::string model_type_name;
        /**
         * Output header field strings corresponding to the variables output by the realization, as defined in
         * `output_variable_names`.
         */
        std::vector<std::string> output_header_fields;

        std::vector<std::string> OPTIONAL_PARAMETERS = {
                BMI_REALIZATION_CFG_PARAM_OPT__FORCING_FILE,
                BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES,
                BMI_REALIZATION_CFG_PARAM_OPT__OUT_VARS,
                BMI_REALIZATION_CFG_PARAM_OPT__OUT_HEADER_FIELDS,
                BMI_REALIZATION_CFG_PARAM_OPT__ALLOW_EXCEED_END,
                BMI_REALIZATION_CFG_PARAM_OPT__FIXED_TIME_STEP,
                BMI_REALIZATION_CFG_PARAM_OPT__LIB_FILE
        };
        std::vector<std::string> REQUIRED_PARAMETERS = {
                BMI_REALIZATION_CFG_PARAM_REQ__INIT_CONFIG,
                BMI_REALIZATION_CFG_PARAM_REQ__MAIN_OUT_VAR,
                BMI_REALIZATION_CFG_PARAM_REQ__MODEL_TYPE,
                BMI_REALIZATION_CFG_PARAM_REQ__USES_FORCINGS
        };

        // Unit test access
        friend class ::Bmi_Formulation_Test;
        friend class ::Bmi_C_Formulation_Test;
    };
}

#endif //NGEN_BMI_FORMULATION_HPP
