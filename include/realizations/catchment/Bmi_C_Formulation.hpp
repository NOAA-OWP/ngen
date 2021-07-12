#ifndef NGEN_BMI_C_FORMULATION_H
#define NGEN_BMI_C_FORMULATION_H

#include <memory>
#include "Bmi_Singular_Formulation.hpp"
#include "Bmi_C_Adapter.hpp"

#define BMI_C_DEFAULT_REGISTRATION_FUNC "register_bmi"

namespace realization {

    class Bmi_C_Formulation : public Bmi_Singular_Formulation<models::bmi::Bmi_C_Adapter> {

    public:

        Bmi_C_Formulation(std::string id, forcing_params forcing_config, utils::StreamHandler output_stream);

        std::string get_formulation_type() override;

        /**
         * Get a header line appropriate for a file made up of entries from this type's implementation of
         * ``get_output_line_for_timestep``.
         *
         * Note that like the output generating function, this line does not include anything for time step.
         *
         * @return An appropriate header line for this type.
         */
        std::string get_output_header_line(std::string delimiter) override;

        /**
         * Get a delimited string with all the output variable values for the given time step.
         *
         * This method is useful for preparing calculated data in a representation useful for output files, such as
         * CSV files.
         *
         * The resulting string contains only the calculated output values for the time step, and not the time step
         * index itself.
         *
         * An empty string is returned if the time step value is not in the range of valid time steps for which there
         * are calculated values for all variables.
         *
         * The default delimiter is a comma.
         *
         * Implementations will throw `invalid_argument` exceptions if data for the provided time step parameter is not
         * accessible.
         *
         * @param timestep The time step for which data is desired.
         * @return A delimited string with all the output variable values for the given time step.
         */
        std::string get_output_line_for_timestep(int timestep, std::string delimiter) override;

        /**
         * Get the model response for a time step.
         *
         * Get the model response for the provided time step, executing the backing model formulation one or more times
         * as needed.
         *
         * Function assumes the backing model has been fully initialized an that any additional input values have been
         * applied.
         *
         * The function throws an error if the index of a previously processed time step is supplied, except if it is
         * the last processed time step.  In that case, the appropriate value is returned as described below, but
         * without executing any model update.
         *
         * Assuming updating to the implied time is valid for the model, the function executes one or more model updates
         * to process future time steps for the necessary indexes.  Multiple time steps updates occur when the given
         * future time step index is not the next time step index to be processed.  Regardless, all processed time steps
         * have the size supplied in `t_delta`.
         *
         * However, it is possible to provide `t_index` and `t_delta` values that would result in the aggregate updates
         * taking the model's time beyond its `end_time` value.  In such cases, if the formulation config indicates this
         * model is not allow to exceed its set `end_time`, the function does not update the model and throws an error.
         *
         * The function will return the value of the primary output variable (see `get_bmi_main_output_var()`) for the
         * given time step after the model has been updated to that point. The type returned will always be a `double`,
         * with other numeric types being cast if necessary.
         *
         * The BMI spec requires for variable values to be passed to/from models via as arrays.  This function
         * essentially  treats the variable array reference as if it were just a raw pointer and returns the `0`-th
         * array value.
         *
         * @param t_index The index of the time step for which to run model calculations.
         * @param d_delta_s The duration, in seconds, of the time step for which to run model calculations.
         * @return The total discharge of the model for the given time step.
         */
        double get_response(time_step_t t_index, time_step_t t_delta) override;

        inline bool is_bmi_input_variable(const std::string &var_name) override;

        inline bool is_bmi_output_variable(const std::string &var_name) override;

    protected:

        /**
         * Construct model and its shared pointer, potentially supplying input variable values from config.
         *
         * Construct a model (and a shared pointer to it), checking whether additional input variable values are present
         * in the configuration properties and need to be used during model construction.
         *
         * @param properties Configuration properties for the formulation, potentially containing values for input
         *                   variables
         * @return A shared pointer to a newly constructed model adapter object
         */
        std::shared_ptr<models::bmi::Bmi_C_Adapter> construct_model(const geojson::PropertyMap& properties) override;

        /**
         * Determine and set the offset time of the model in seconds, compared to forcing data.
         *
         * BMI models frequently have their model start time be set to 0.  As such, to know what the forcing time is
         * compared to the model time, an offset value is needed.  This becomes important in situations when the size of
         * the time steps for forcing data versus model execution are not equal.  This method will determine and set
         * this value.
         */
        void determine_model_time_offset() override;

        /**
         * Get model input values from forcing data, accounting for model and forcing time steps not aligning.
         *
         * Get values to use to set model input variables for forcings, sourced from this instance's forcing data.  Skip
         * any params in the collection that are not forcing params, as indicated by the given collection.  Account for
         * if model time step (MTS) does not align with forcing time step (FTS), either due to MTS starting after the
         * start of FTS, MTS extending beyond the end of FTS, or both.
         *
         * @param t_delta The size of the model's time step in seconds.
         * @param model_initial_time The model's current time in its internal units and representation.
         * @param params An ordered collection of desired forcing param names from which data for inputs is needed.
         * @param is_aorc_param Whether the param at each index is a forcing param, or a different model param (which
         *                         thus does not need to be processed here).
         * @param param_units An ordered collection units of strings representing the BMI model's expected units for the
         *                    corresponding input, so that value conversions of the proportional contributions are done.
         * @param summed_contributions A referenced ordered collection that will contain returned summed contributions.
         */
        inline void get_forcing_data_ts_contributions(time_step_t t_delta, const double &model_initial_time,
                                                      const std::vector<std::string> &params,
                                                      const std::vector<bool> &is_aorc_param,
                                                      const std::vector<std::string> &param_units,
                                                      std::vector<double> &summed_contributions);

        /**
         * Get a value, converted to specified type, for an output variable at a time step.
         *
         * Function gets the value for a provided output variable at a provided time step index, and returns the value
         * converted to some particular type specified by the template param.
         *
         * The function makes several assumptions:
         *
         *     1. `t_index` is a time step that has already been processed for the model
         *     2. `var_name` is in the set of valid output variable names for the model
         *     3. it is possible to either implicitly or explicitly convert (i.e., cast) the output value to the
         *        template parameter type (or it is already of that type)
         *     4. conversions do not lead to invalid (e.g., runtime errors) or unexpected (e.g., loss of precision)
         *        behavior
         *
         * It falls to users (functions) of this function to ensure these assumptions hold before invoking.
         *
         * @tparam T The type that should be returned, and to which the BMI variable value should be cast.
         * @param t_index The index of some already-processed time step for which a variable value is being requested
         * @param var_name The name of the variable for which a value is being requested
         * @return
         */
        template<class T, class O>
        T get_var_value_as(time_step_t t_index, const std::string& var_name) {
            std::vector<O> outputs = get_bmi_model()->GetValue<O>(var_name);
            return (T) outputs[t_index];
        }

        /**
         * Get value for some BMI model variable.
         *
         * This function assumes that the given variable, while returned by the model within an array per the BMI spec,
         * is actual a single, scalar value.  Thus, it returns what is at index 0 of the array reference.
         *
         * @param index
         * @param var_name
         * @return
         */
        double get_var_value_as_double(const std::string& var_name) override;

        /**
         * Get value for some BMI model variable at a specific index.
         *
         * Function gets the value for a provided variable, returned from the backing model as an array, and returns the
         * specific value at the desired index cast as a double type.
         *
         * The function makes several assumptions:
         *
         *     1. `index` is within array bounds
         *     2. `var_name` is in the set of valid variable names for the model
         *     3. the type for output variable allows the value to be cast to a `double` appropriately
         *
         * It falls to user (functions) of this function to ensure these assumptions hold before invoking.
         *
         * @param index
         * @param var_name
         * @return
         */
        double get_var_value_as_double(const int& index, const std::string& var_name) override;

        /**
         * Test whether backing model has run BMI ``Initialize``.
         *
         * Test whether the backing model object has been initialize using the BMI standard ``Initialize`` function.
         *
         * This overrides the super class implementation and checks the model directly.  As such, the associated setter
         * does not serve any purpose.
         *
         * @return Whether backing model object has been initialize using the BMI standard ``Initialize`` function.
         */
        bool is_model_initialized() override;

        /**
         * Set BMI input variable values for the model appropriately prior to calling its `BMI `update()``.
         *
         * @param model_initial_time The model's time prior to the update, in its internal units and representation.
         * @param t_delta The size of the time step over which the formulation is going to update the model, which might
         *                be different than the model's internal time step.
         */
        void set_model_inputs_prior_to_update(const double &model_initial_time, time_step_t t_delta) override;

        // Unit test access
        friend class ::Bmi_Formulation_Test;
        friend class ::Bmi_C_Formulation_Test;
        friend class ::Bmi_C_Cfe_IT;

    private:

        /** Index value (0-based) of the time step that will be processed by the next update of the model. */
        int next_time_step_index = 0;

    };

}

#endif //NGEN_BMI_C_FORMULATION_H
