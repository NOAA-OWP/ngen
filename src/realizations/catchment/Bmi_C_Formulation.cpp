#include "Bmi_C_Formulation.hpp"

using namespace realization;
using namespace models::bmi;

Bmi_C_Formulation::Bmi_C_Formulation(std::string id, forcing_params forcing_config, utils::StreamHandler output_stream)
    : Bmi_Formulation<models::bmi::Bmi_C_Adapter>(id, forcing_config, output_stream) { }

std::string Bmi_C_Formulation::get_formulation_type() {
    return "bmi_c";
}

/**
 * Construct model and its shared pointer.
 *
 * @param properties Configuration properties for the formulation.
 * @return A shared pointer to a newly constructed model adapter object.
 */
std::shared_ptr<Bmi_C_Adapter> Bmi_C_Formulation::construct_model(const geojson::PropertyMap& properties) {
    auto library_file_iter = properties.find(BMI_REALIZATION_CFG_PARAM_OPT__LIB_FILE);
    if (library_file_iter == properties.end()) {
        throw std::runtime_error("BMI C formulation requires path to library file, but none provided in config");
    }
    std::string lib_file = library_file_iter->second.as_string();

    return std::make_shared<Bmi_C_Adapter>(
            Bmi_C_Adapter(
                    lib_file,
                    get_bmi_init_config(),
                    get_forcing_file_path(),
                    is_bmi_using_forcing_file(),
                    get_allow_model_exceed_end_time(),
                    is_bmi_model_time_step_fixed(),
                    output));
}

/**
 * Determine and set the offset time of the model in seconds, compared to forcing data.
 *
 * BMI models frequently have their model start time be set to 0.  As such, to know what the forcing time is
 * compared to the model time, an offset value is needed.  This becomes important in situations when the size of
 * the time steps for forcing data versus model execution are not equal.  This method will determine and set
 * this value.
 */
void Bmi_C_Formulation::determine_model_time_offset() {
    set_bmi_model_start_time_forcing_offset_s(forcing.get_time_epoch() -
                                              (time_t) get_bmi_model()->convert_model_time_to_seconds(
                                                      get_bmi_model()->GetStartTime()));
}

/**
 * Get model input values from forcing data, accounting for model and forcing time steps not aligning.
 *
 * Get values to use to set model input variables, sourced from forcing data.  Account for if model time step
 * (MTS) does not align with forcing time step (FTS), either due to MTS starting after the start of FTS, MTS
 * extending beyond the end of FTS, or both.
 *
 * @param t_delta The size of the model's time step in seconds.
 * @param model_initial_time The model's current time in its internal units and representation.
 * @param params An ordered collection of desired forcing param names from which data for inputs is needed.
 * @param param_units An ordered collection units of strings representing the BMI model's expected units for the
 *                    corresponding input, so that value conversions of the proportional contributions are done.
 * @param summed_contributions A referenced ordered collection that will contain the returned summed contributions.
 */
inline void Bmi_C_Formulation::get_forcing_data_ts_contributions(time_step_t t_delta, const double &model_initial_time,
                                                                 const std::vector<std::string> &params,
                                                                 const std::vector<std::string> &param_units,
                                                                 std::vector<double> &summed_contributions)
{
    time_t model_ts_start_offset, model_ts_seconds_contained_in_forcing_ts;
    bool increment_to_next_forcing_ts;
    // Keep track of how much of the model ts delta has not yet had its contribution pulled from some forcing ts
    time_step_t contribution_seconds_remaining = t_delta;

    // The sum of these first two is essentially the model time step's epoch start time in seconds
    model_ts_start_offset = (time_t) (get_bmi_model()->convert_model_time_to_seconds(model_initial_time)) +
                            get_bmi_model_start_time_forcing_offset_s() -
                            forcing.get_time_epoch();

    while (contribution_seconds_remaining > 0) {
        // If model ts, after having start time shifted forward within forcing step (if needed), goes beyond forcing ts
        if ((time_t)contribution_seconds_remaining + model_ts_start_offset >= forcing.get_time_step_size()) {
            increment_to_next_forcing_ts = true;
            model_ts_seconds_contained_in_forcing_ts = forcing.get_time_step_size() - model_ts_start_offset;
        }
        else {
            increment_to_next_forcing_ts = false;
            model_ts_seconds_contained_in_forcing_ts = (time_t)contribution_seconds_remaining;
        }

        // Get the contributions from this forcing ts for all the forcing params needed.
        for (size_t i = 0; i < params.size(); ++i) {
            // This is proportional to the ratio of (model ts seconds in this forcing ts) to (forcing ts total seconds)
            if (forcing.is_param_sum_over_time_step(params[i])) {
                summed_contributions[i] += forcing.get_converted_value_for_param_in_units(params[i], param_units[i]) *
                                           (double) model_ts_seconds_contained_in_forcing_ts /
                                           (double) forcing.get_time_step_size();
            }
            // This is proportional to the ratio of (model ts seconds in this forcing ts) to (model ts total seconds)
            else {
                summed_contributions[i] += forcing.get_converted_value_for_param_in_units(params[i], param_units[i]) *
                                           (double) model_ts_seconds_contained_in_forcing_ts /
                                           (double) t_delta;
            }
        }

        // Account for the processed model time compared to the entire delta
        contribution_seconds_remaining -= (time_step_t)model_ts_seconds_contained_in_forcing_ts;
        // The offset should only possibly be non-zero the first time (after, if the model ts extends beyond the first
        // forcing ts, it always picks up at the beginning of the subsequent forcing ts), so set to 0 now.
        model_ts_start_offset = 0;
        // Also, when appropriate ...
        if (increment_to_next_forcing_ts) {
            forcing.get_next_hourly_precipitation_meters_per_second();
        }
    }
}

std::string Bmi_C_Formulation::get_output_header_line(std::string delimiter) {
    return boost::algorithm::join(get_output_header_fields(), delimiter);
}

std::string Bmi_C_Formulation::get_output_line_for_timestep(int timestep, std::string delimiter) {
    // TODO: something must be added to store values if more than the current time step is wanted
    // TODO: if such a thing is added, it should probably be configurable to turn it off
    if (timestep != (next_time_step_index - 1)) {
        throw std::invalid_argument("Only current time step valid when getting output for BMI C formulation");
    }
    std::string output_str;

    for (const std::string& name : get_output_variable_names()) {
        output_str += (output_str.empty() ? "" : ",") + std::to_string(get_var_value_as_double(name));
    }
    return output_str;
}

/**
 * Get the model response for a time step.
 *
 * Get the model response for the provided time step, executing the backing model formulation one or more times as
 * needed.
 *
 * Function assumes the backing model has been fully initialized an that any additional input values have been applied.
 *
 * The function throws an error if the index of a previously processed time step is supplied, except if it is the last
 * processed time step.  In that case, the appropriate value is returned as described below, but without executing any
 * model update.
 *
 * Assuming updating to the implied time is valid for the model, the function executes one or more model updates to
 * process future time steps for the necessary indexes.  Multiple time steps updates occur when the given future time
 * step index is not the next time step index to be processed.  Regardless, all processed time steps have the size
 * supplied in `t_delta`.
 *
 * However, it is possible to provide `t_index` and `t_delta` values that would result in the aggregate updates taking
 * the model's time beyond its `end_time` value.  In such cases, if the formulation config indicates this model is
 * not allow to exceed its set `end_time`, the function does not update the model and throws an error.
 *
 * The function will return the value of the primary output variable (see `get_bmi_main_output_var()`) for the given
 * time step after the model has been updated to that point. The type returned will always be a `double`, with other
 * numeric types being cast if necessary.
 *
 * The BMI spec requires for variable values to be passed to/from models via as arrays.  This function essentially
 * treats the variable array reference as if it were just a raw pointer and returns the `0`-th array value.
 *
 * @param t_index The index of the time step for which to run model calculations.
 * @param d_delta_s The duration, in seconds, of the time step for which to run model calculations.
 * @return The total discharge of the model for the given time step.
 */
double Bmi_C_Formulation::get_response(time_step_t t_index, time_step_t t_delta) {
    if (get_bmi_model() == nullptr) {
        throw std::runtime_error("Trying to process response of improperly created BMI C formulation.");
    }
    if (t_index < 0) {
        throw std::invalid_argument("Getting response of negative time step in BMI C formulation is not allowed.");
    }
    // Use (next_time_step_index - 1) so that second call with current time step index still works
    if (t_index < (next_time_step_index - 1)) {
        // TODO: consider whether we should (optionally) store and return historic values
        throw std::invalid_argument("Getting response of previous time step in BMI C formulation is not allowed.");
    }

    // The time step delta size, expressed in the units internally used by the model
    double t_delta_model_units;
    if (next_time_step_index <= t_index) {
        t_delta_model_units = get_bmi_model()->convert_seconds_to_model_time(t_delta);
        double model_time = get_bmi_model()->GetCurrentTime();
        // Also, before running, make sure this doesn't cause a problem with model end_time
        if (!get_allow_model_exceed_end_time()) {
            int total_time_steps_to_process = abs((int)t_index - next_time_step_index) + 1;
            if (get_bmi_model()->GetEndTime() < (model_time + (t_delta_model_units * total_time_steps_to_process))) {
                throw std::invalid_argument("Cannot process BMI C formulation to get response of future time step "
                                            "that exceeds model end time.");
            }
        }
    }

    while (next_time_step_index <= t_index) {
        double model_initial_time = get_bmi_model()->GetCurrentTime();
        set_model_inputs_prior_to_update(model_initial_time, t_delta);
        if (t_delta_model_units == get_bmi_model()->GetTimeStep())
            get_bmi_model()->Update();
        else
            get_bmi_model()->UpdateUntil(model_initial_time + t_delta_model_units);
        // TODO: again, consider whether we should store any historic response, ts_delta, or other var values
        next_time_step_index++;
    }
    return get_var_value_as_double( get_bmi_main_output_var());
}

double Bmi_C_Formulation::get_var_value_as_double(const std::string& var_name) {
    return get_var_value_as_double(0, var_name);
}

double Bmi_C_Formulation::get_var_value_as_double(const int& index, const std::string& var_name) {
    // TODO: consider different way of handling (and how to document) cases like long double or unsigned long long that
    //  don't fit or might convert inappropriately
    std::string type = get_bmi_model()->GetVarType(var_name);
    if (type == "long double")
        return (double) (get_bmi_model()->GetValuePtr<long double>(var_name))[index];

    if (type == "double")
        return (double) (get_bmi_model()->GetValuePtr<double>(var_name))[index];

    if (type == "float")
        return (double) (get_bmi_model()->GetValuePtr<float>(var_name))[index];

    if (type == "short" || type == "short int" || type == "signed short" || type == "signed short int")
        return (double) (get_bmi_model()->GetValuePtr<short>(var_name))[index];

    if (type == "unsigned short" || type == "unsigned short int")
        return (double) (get_bmi_model()->GetValuePtr<unsigned short>(var_name))[index];

    if (type == "int" || type == "signed" || type == "signed int")
        return (double) (get_bmi_model()->GetValuePtr<int>(var_name))[index];

    if (type == "unsigned" || type == "unsigned int")
        return (double) (get_bmi_model()->GetValuePtr<unsigned int>(var_name))[index];

    if (type == "long" || type == "long int" || type == "signed long" || type == "signed long int")
        return (double) (get_bmi_model()->GetValuePtr<long>(var_name))[index];

    if (type == "unsigned long" || type == "unsigned long int")
        return (double) (get_bmi_model()->GetValuePtr<unsigned long>(var_name))[index];

    if (type == "long long" || type == "long long int" || type == "signed long long" || type == "signed long long int")
        return (double) (get_bmi_model()->GetValuePtr<long long>(var_name))[index];

    if (type == "unsigned long long" || type == "unsigned long long int")
        return (double) (get_bmi_model()->GetValuePtr<unsigned long long>(var_name))[index];

    throw std::runtime_error("Unable to get value of variable " + var_name + " from " + get_model_type_name() +
                             " as double: no logic for converting variable type " + type);
}

bool Bmi_C_Formulation::is_model_initialized() {
    return get_bmi_model()->is_model_initialized();
}

/**
 * Set BMI input variable values for the model appropriately prior to calling its `BMI `update()``.
 *
 * @param model_initial_time The model's time prior to the update, in its internal units and representation.
 * @param t_delta The size of the time step over which the formulation is going to update the model, which might be
 *                different than the model's internal time step.
 */
void Bmi_C_Formulation::set_model_inputs_prior_to_update(const double &model_initial_time, time_step_t t_delta) {
    std::string std_name;

    std::vector<std::string> in_var_names = get_bmi_model()->GetInputVarNames();
    // Get the BMI variables' units and associated internal standard names, keeping them in these
    std::vector<std::string> standard_names(in_var_names.size());
    std::vector<std::string> bmi_var_units(in_var_names.size());
    // We may need to sum contributions from multiple time steps before setting, so do that with this (indexes align)
    std::vector<double> variable_values(in_var_names.size(), 0.0);
    // Right now, the only known field names are the standard names for forcings.
    std::shared_ptr<std::set<std::string>> known_field_names = Forcing::get_forcing_field_names();

    // First get field name mappings and associated model-side units
    for (auto & in_var_name : in_var_names) {
        std_name = get_config_mapped_variable_name(in_var_name);
        // Also, recognize some external CSDMS standard names, whether from model or configured mapping ...
        // TODO: right now handle just rainfall, but in the future, support more possibly
        if (std_name == CSDMS_STD_NAME_RAIN_RATE) {
            std_name = AORC_FIELD_NAME_PRECIP_RATE;
        }
        // With any mapping translation done, if std_name is not in set of standard internal names, that is a problem
        if (known_field_names->find(std_name) == known_field_names->end()) {
            throw std::runtime_error(
                    "Unrecognized BMI input variable '" + std_name + "' cannot be set in '" + get_model_type_name()
                    + "' as it is not a preset standard and not mapped to such a field name via configuration.");
        }
        // FIXME: later perhaps handle input variables that are themselves arrays, but for now do not support
        if (get_bmi_model()->GetVarItemsize(in_var_name) != get_bmi_model()->GetVarNbytes(in_var_name)) {
            throw std::runtime_error(
                    "BMI input variable '" + in_var_name + "' is an array, which is not currently supported");
        }
        // Assign associated standard name and model-side unit string to those collections at corresponding index
        standard_names.push_back(std_name);
        bmi_var_units.push_back(get_bmi_model()->GetVarUnits(in_var_name));
    }
    // Run separate (hopefully inline) function for getting input values for each variable, potentially by summing
    // proportionaly contributions from multiple forcing time steps if model time step doesn't align
    get_forcing_data_ts_contributions(t_delta, model_initial_time, standard_names, bmi_var_units, variable_values);

    // Finally, set the model input values
    for (int i = 0; i < in_var_names.size(); ++i) {
        get_bmi_model()->SetValue(in_var_names[i], (void*)&(variable_values[i]));
    }
}
