#ifndef NGEN_BMI_MODULE_FORMULATION_H
#define NGEN_BMI_MODULE_FORMULATION_H

#include <utility>
#include <memory>
#include "Bmi_Formulation.hpp"
#include "Bmi_Adapter.hpp"
#include <DataProvider.hpp>
#include "bmi_utilities.hpp"

using data_access::MEAN;
using data_access::SUM;

// Forward declaration to provide access to protected items in testing
class Bmi_Formulation_Test;
class Bmi_Multi_Formulation_Test;
class Bmi_C_Formulation_Test;
class Bmi_Cpp_Formulation_Test;
class Bmi_C_Pet_IT;
class Bmi_Cpp_Multi_Array_Test;

namespace realization {

    /**
     * Abstraction of a formulation with a single backing model object that implements the BMI.
     */
    class Bmi_Module_Formulation : public Bmi_Formulation {

    public:

        /**
         * Minimal constructor for objects initialize using the Formulation_Manager and subsequent calls to
         * ``create_formulation``.
         *
         * @param id
         * @param forcing_provider
         * @param output_stream
         */
        Bmi_Module_Formulation(std::string id, std::shared_ptr<data_access::GenericDataProvider> forcing_provider, utils::StreamHandler output_stream)
                : Bmi_Formulation(std::move(id), forcing_provider, output_stream) { }

        ~Bmi_Module_Formulation() override = default;

        void create_formulation(boost::property_tree::ptree &config, geojson::PropertyMap *global = nullptr) override;
        void create_formulation(geojson::PropertyMap properties) override;

        /**
         * Get the collection of forcing output property names this instance can provide.
         *
         * This is part of the @ref ForcingProvider interface.  This interface must be implemented for items of this
         * type to be usable as "forcing" providers for situations when some other object needs to receive as an input
         * (i.e., one of its forcings) a data property output from this object.
         *
         * For this type, this is the collection of BMI output variables, plus any aliases included in the formulation
         * config's output variable mapping.
         *
         * @return The collection of forcing output property names this instance can provide.
         * @see ForcingProvider
         */
        boost::span<const std::string> get_available_variable_names() const override;

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
         * accessible.  Note that, for this type, only the last processed time step is accessible, because formulations
         * do not save results from previous time steps.  This also has the consequence of there being no valid set of
         * arguments before a least one call to @ref get_response has been made.
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

        /**
         * Get the inclusive beginning of the period of time over which this instance can provide data for this forcing.
         *
         * This is part of the @ref ForcingProvider interface.  This interface must be implemented for items of this
         * type to be usable as "forcing" providers for situations when some other object needs to receive as an input
         * (i.e., one of its forcings) a data property output from this object.
         *
         * @return The inclusive beginning of the period of time over which this instance can provide this data.
         */
        time_t get_variable_time_begin(const std::string &variable_name);

        /**
         * Get the inclusive beginning of the period of time over which this instance can provide data for this forcing.
         *
         * This is part of the @ref DataProvider interface.  This interface must be implemented for items of this
         * type to be usable as "forcing" providers for situations when some other object needs to receive as an input
         * (i.e., one of its forcings) a data property output from this object.
         *
         * @return The inclusive beginning of the period of time over which this instance can provide this data.
         */
        long get_data_start_time() const override;

        long get_data_stop_time() const override;

        long record_duration() const override;

        /**
         * Get the current time for the backing BMI model in its native format and units.
         *
         * @return The current time for the backing BMI model in its native format and units.
         */
        const double get_model_current_time() const override;

        /**
         * Get the end time for the backing BMI model in its native format and units.
         *
         * @return The end time for the backing BMI model in its native format and units.
         */
        const double get_model_end_time() const override;

        const std::vector<std::string> &get_required_parameters() const override;

        /**
         * When possible, translate a variable name for a BMI model to an internally recognized name.
         *
         * Translate some BMI variable name to something recognized in some internal context for use within NGen.  Do
         * this according to the map of variable names supplied in the external formulation config.  If no mapping for
         * the given variable name was configured, return the variable name itself.
         *
         * For example, perhaps a BMI model has the input variable "RAIN_RATE."  Configuring this variable name to map
         * to "precip_rate" will allow the formulation to understand that this particular forcing field should be used
         * to set the model's "RAIN_RATE" variable.
         *
         * @param model_var_name The BMI variable name to translate so its purpose is recognized internally.
         * @return Either the internal equivalent variable name, or the provided name if there is not a mapping entry.
         */
        const std::string &get_config_mapped_variable_name(const std::string &model_var_name) const override;

        /**
         * Get the index of the forcing time step that contains the given point in time.
         *
         * This is part of the @ref ForcingProvider interface.  This interface must be implemented for items of this
         * type to be usable as "forcing" providers for situations when some other object needs to receive as an input
         * (i.e., one of its forcings) a data property output from this object.
         *
         * An @ref std::out_of_range exception should be thrown if the time is not in any time step.
         *
         * @param epoch_time The point in time, as a seconds-based epoch time.
         * @return The index of the forcing time step that contains the given point in time.
         * @throws std::out_of_range If the given point is not in any time step.
         */
        size_t get_ts_index_for_time(const time_t &epoch_time) const override;

        /**
         * @brief Get the 1D values of a forcing property for an arbitrary time period, converting units if needed.
         * 
         * @param output_name The name of the forcing property of interest.
         * @param init_time The epoch time (in seconds) of the start of the time period.
         * @param duration_s The length of the time period, in seconds.
         * @param output_units The expected units of the desired output value.
         * @return std::vector<double> The 1D values of the forcing property for the described time period, with units converted if needed.
         * @throws std::out_of_range If data for the time period is not available.
         * @throws std::runtime_error output_name is not one of the available outputs of this provider instance.
         */
        std::vector<double> get_values(const CatchmentAggrDataSelector& selector, data_access::ReSampleMethod m=SUM) override;

        /**
         * Get the value of a forcing property for an arbitrary time period, converting units if needed.
         *
         * This is part of the @ref ForcingProvider interface.  This interface must be implemented for items of this
         * type to be usable as "forcing" providers for situations when some other object needs to receive as an input
         * (i.e., one of its forcings) a data property output from this object.
         *
         * An @ref std::out_of_range exception should be thrown if the data for the time period is not available.
         *
         * @param output_name The name of the forcing property of interest.
         * @param init_time_epoch The epoch time (in seconds) of the start of the time period.
         * @param duration_seconds The length of the time period, in seconds.
         * @param output_units The expected units of the desired output value.
         * @return The value of the forcing property for the described time period, with units converted if needed.
         * @throws std::out_of_range If data for the time period is not available.
         * @see ForcingProvider::get_value
         */
        double get_value(const CatchmentAggrDataSelector& selector, data_access::ReSampleMethod m) override;

        bool is_bmi_input_variable(const std::string &var_name) const override;
        bool is_bmi_output_variable(const std::string &var_name) const override;

        /**
         * Get whether a property's per-time-step values are each an aggregate sum over the entire time step.
         *
         * This is part of the @ref ForcingProvider interface.  This interface must be implemented for items of this
         * type to be usable as "forcing" providers for situations when some other object needs to receive as an input
         * (i.e., one of its forcings) a data property output from this object.
         *
         * Certain properties, like rain fall, are aggregated sums over an entire time step.  Others, such as pressure,
         * are not such sums and instead something else like an instantaneous reading or an average value.
         *
         * It may be the case that forcing data is needed for some discretization different than the forcing time step.
         * This aspect must be known in such cases to perform the appropriate value interpolation.
         *
         * For instances of this type, all output forcings fall under this category.
         *
         * @param name The name of the forcing property for which the current value is desired.
         * @return Whether the property's value is an aggregate sum, which is always ``true`` for this type.
         */
        bool is_property_sum_over_time_step(const std::string& name) const override;

        const std::vector<std::string> get_bmi_input_variables() const override;
        const std::vector<std::string> get_bmi_output_variables() const override;

    protected:

        /**
         * @brief Get correct BMI variable name, which may be the output or something mapped to this output.
         *
         * @param name
         * @param bmi_var_name
         */
        void get_bmi_output_var_name(const std::string &name, std::string &bmi_var_name);

        /**
         * Construct model and its shared pointer, potentially supplying input variable values from config.
         *
         * Construct a model (and a shared pointer to it), checking whether additional input variable values are present
         * in the configuration properties and need to be used during model construction.
         *
         * @param properties Configuration properties for the formulation, potentially containing values for input
         *                   variables
         * @return A shared pointer to a newly constructed model object
         */
        virtual std::shared_ptr<models::bmi::Bmi_Adapter> construct_model(const geojson::PropertyMap& properties) = 0;

        /**
         * Determine and set the offset time of the model in seconds, compared to forcing data.
         *
         * BMI models frequently have their model start time be set to 0.  As such, to know what the forcing time is
         * compared to the model time, an offset value is needed.  This becomes important in situations when the size of
         * the time steps for forcing data versus model execution are not equal.  This method will determine and set
         * this value.
         */
        void determine_model_time_offset();

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
        const bool &get_allow_model_exceed_end_time() const override;

        const std::string &get_bmi_init_config() const;

        /**
         * Get the backing model object implementing the BMI.
         *
         * @return Shared pointer to the backing model object that implements the BMI.
         */
        std::shared_ptr<models::bmi::Bmi_Adapter> get_bmi_model() const;

        const time_t &get_bmi_model_start_time_forcing_offset_s() const override;

        /**
         * Universal logic applied when creating a BMI-backed formulation from NGen config.
         *
         * This performs all the necessary steps to initialize this formulation from provided configuration
         * properties. It is written in such a way that it can be used in appropriately crafted nested calls from both
         * public `create_formulation` implementations, thus allowing the primary formulation initialization logic to
         * be centralized and not duplicated.
         *
         * @param properties
         * @param needs_param_validation
         */
        void inner_create_formulation(geojson::PropertyMap properties, bool needs_param_validation);

        /**
         * @brief Check configuration properties for `model_params` and attempt to set them in the bmi model.
         * 
         * This checks for a key named `model_params` in the parsed properties, and for each property
         * it will attempt to call `SetValue` using the property's key as the BMI variable
         * and the property's value as the value to set.
         * 
         * This function should only be called once @p bmi_model is properly constructed.
         * If @p bmi_model is a nullptr, this function becomes a no-op.
         * 
         */
        void set_initial_bmi_parameters(geojson::PropertyMap properties);

        /**
         * Test whether backing model has fixed time step size.
         *
         * @return Whether backing model has fixed time step size.
         */
        bool is_bmi_model_time_step_fixed() const override;

        /**
         * Test whether the backing model object has been initialize using the BMI standard ``Initialize`` function.
         *
         * @return Whether backing model object has been initialize using the BMI standard ``Initialize`` function.
         */
        bool is_model_initialized() const override;

        void set_allow_model_exceed_end_time(bool allow_exceed_end);

        void set_bmi_init_config(const std::string &init_config);

        /**
         * Set the backing model object implementing the BMI.
         *
         * @param model Shared pointer to the BMI model.
         */
        void set_bmi_model(std::shared_ptr<models::bmi::Bmi_Adapter> model);

        void set_bmi_model_start_time_forcing_offset_s(const time_t &offset_s);

        void set_bmi_model_time_step_fixed(bool is_fix_time_step);

        /**
         * Set whether the backing model object has been initialize using the BMI standard ``Initialize`` function.
         *
         * @param is_initialized Whether model object has been initialize using the BMI standard ``Initialize``.
         */
        virtual void set_model_initialized(bool is_initialized);

        /**
         * Set BMI input variable values for the model appropriately prior to calling its `BMI `update()``.
         *
         * @param model_initial_time The model's time prior to the update, in its internal units and representation.
         * @param t_delta The size of the time step over which the formulation is going to update the model, which might
         *                be different than the model's internal time step.
         */
        void set_model_inputs_prior_to_update(const double &model_init_time, time_step_t t_delta);

        /** The delta of the last model update execution (typically, this is time step size). */
        time_step_t last_model_response_delta = 0;
        /** The epoch time of the model at the beginning of its last update. */
        time_t last_model_response_start_time = 0;
        std::map<std::string, std::shared_ptr<data_access::GenericDataProvider>> input_forcing_providers;

        // Access for multi-BMI
        friend class Bmi_Multi_Formulation;

        // Unit test access
        friend class ::Bmi_Formulation_Test;
        friend class ::Bmi_C_Formulation_Test;
        friend class ::Bmi_Multi_Formulation_Test;
        friend class ::Bmi_Cpp_Formulation_Test;
        friend class ::Bmi_Cpp_Multi_Array_Test;

        /**
         * Index value (0-based) of the time step that will be processed by the next update of the model.
         *
         * A formulation time step for BMI types can be thought of as the execution of a call to any of the functions of
         * the underlying BMI model that advance the model (either `update` or `update_until`). This member stores the
         * ordinal index of the next time step to be executed.  Except in the initial formulation state, this will be
         * one greater than the index of the last executed time step.
         *
         * E.g., on initialization, before any calls to @ref get_response, this value will be ``0``.  After a call to
         * @ref get_response (assuming ``0`` as the passed ``t_index`` argument), time step ``0`` will be processed, and
         * this member would be incremented by 1, thus making it ``1``.
         *
         * The member serves as an implicit marker of how many time steps have been processed so far.  Knowing this is
         * required to maintain valid behavior in certain things, such as @ref get_response (we may want to process
         * multiple time steps forward to a particular index other than the next, but it would not be valid to receive
         * a ``t_index`` earlier than the last processed time step) and @ref get_output_line_for_timestep (because
         * formulations do not save results from previous time steps, only the results from the last processed time step
         * can be used to generate output).
         */
        int next_time_step_index = 0;

    private:
        /**
         * Whether model ``Update`` calls are allowed and handled in some way by the backing model for time steps after
         * the model's ``end_time``.
         */
        bool allow_model_exceed_end_time = false;
        /** The set of available "forcings" (output variables, plus their mapped aliases) that the model can provide. */
        std::vector<std::string> available_forcings;
        std::string bmi_init_config;
        std::shared_ptr<models::bmi::Bmi_Adapter> bmi_model;
        /** Whether backing model has fixed time step size. */
        bool bmi_model_time_step_fixed = true;
        /**
         * The offset, converted to seconds, from the model's start time to the start time of the initial forcing time
         * step.
         */
        time_t bmi_model_start_time_forcing_offset_s;
        /** A configured mapping of BMI model variable names to standard names for use inside the framework. */
        std::map<std::string, std::string> bmi_var_names_map;
        bool model_initialized = false;

        std::vector<std::string> OPTIONAL_PARAMETERS = {
                BMI_REALIZATION_CFG_PARAM_OPT__USES_FORCINGS
                BMI_REALIZATION_CFG_PARAM_OPT__FORCING_FILE,
                BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES,
                BMI_REALIZATION_CFG_PARAM_OPT__OUT_VARS,
                BMI_REALIZATION_CFG_PARAM_OPT__OUT_HEADER_FIELDS,
                BMI_REALIZATION_CFG_PARAM_OPT__OUTPUT_PRECISION,
                BMI_REALIZATION_CFG_PARAM_OPT__ALLOW_EXCEED_END,
                BMI_REALIZATION_CFG_PARAM_OPT__FIXED_TIME_STEP,
                BMI_REALIZATION_CFG_PARAM_OPT__LIB_FILE
        };
        std::vector<std::string> REQUIRED_PARAMETERS = {
                BMI_REALIZATION_CFG_PARAM_REQ__INIT_CONFIG,
                BMI_REALIZATION_CFG_PARAM_REQ__MAIN_OUT_VAR,
                BMI_REALIZATION_CFG_PARAM_REQ__MODEL_TYPE,
        };

    };
/*
    template<class M>
    void Bmi_Formulation<M>::create_formulation(boost::property_tree::ptree &config, geojson::PropertyMap *global) {
        geojson::PropertyMap options = this->interpret_parameters(config, global);
        inner_create_formulation(options, false);
    }

    template<class M>
    void Bmi_Formulation<M>::create_formulation(geojson::PropertyMap properties) {
        inner_create_formulation(properties, true);
    }

    template<class M>
    void Bmi_Formulation<M>::inner_create_formulation(geojson::PropertyMap properties, bool needs_param_validation) {
        if (needs_param_validation) {
            validate_parameters(properties);
        }
        // Required parameters first
        set_bmi_init_config(properties.at(BMI_REALIZATION_CFG_PARAM_REQ__INIT_CONFIG).as_string());
        set_bmi_main_output_var(properties.at(BMI_REALIZATION_CFG_PARAM_REQ__MAIN_OUT_VAR).as_string());
        set_model_type_name(properties.at(BMI_REALIZATION_CFG_PARAM_REQ__MODEL_TYPE).as_string());

        // Then optional ...

        if (properties.find(BMI_REALIZATION_CFG_PARAM_OPT__ALLOW_EXCEED_END) != properties.end()) {
            set_allow_model_exceed_end_time(
                    properties.at(BMI_REALIZATION_CFG_PARAM_OPT__ALLOW_EXCEED_END).as_boolean());
        }

        // Do this next, since after checking whether other input variables are present in the properties, we can
        // now construct the adapter and init the model
        set_bmi_model(construct_model(properties));

        // Output variable subset and order, if present
        auto out_var_it = properties.find(BMI_REALIZATION_CFG_PARAM_OPT__OUT_VARS);
        if (out_var_it != properties.end()) {
            std::vector<geojson::JSONProperty> out_vars_json_list = out_var_it->second.as_list();
            std::vector<std::string> out_vars(out_vars_json_list.size());
            for (int i = 0; i < out_vars_json_list.size(); ++i) {
                out_vars[i] = out_vars_json_list[i].as_string();
            }
            set_output_variable_names(out_vars);
        }
            // Otherwise, just take what literally is provided by the model
        else {
            set_output_variable_names(get_bmi_model()->GetOutputVarNames());
        }

        // Output header fields, if present
        auto out_headers_it = properties.find(BMI_REALIZATION_CFG_PARAM_OPT__OUT_HEADER_FIELDS);
        if (out_headers_it != properties.end()) {
            std::vector<geojson::JSONProperty> out_headers_json_list = out_var_it->second.as_list();
            std::vector<std::string> out_headers(out_headers_json_list.size());
            for (int i = 0; i < out_headers_json_list.size(); ++i) {
                out_headers[i] = out_headers_json_list[i].as_string();
            }
            set_output_header_fields(out_headers);
        }
        else {
            set_output_header_fields(get_output_variable_names());
        }
    }
    */

}

#endif //NGEN_BMI_MODULE_FORMULATION_H
