#include "Bmi_C_Formulation.hpp"

using namespace realization;
using namespace models::bmi;

Bmi_C_Formulation::Bmi_C_Formulation(std::string id, Forcing forcing, utils::StreamHandler output_stream)
        : Bmi_Module_Formulation<models::bmi::Bmi_C_Adapter>(id, forcing, output_stream) { }

Bmi_C_Formulation::Bmi_C_Formulation(std::string id, forcing_params forcing_config, utils::StreamHandler output_stream)
    : Bmi_Module_Formulation<models::bmi::Bmi_C_Adapter>(id, forcing_config, output_stream) { }

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
    auto reg_func_itr = properties.find(BMI_REALIZATION_CFG_PARAM_OPT__REGISTRATION_FUNC);
    std::string reg_func =
            reg_func_itr == properties.end() ? BMI_C_DEFAULT_REGISTRATION_FUNC : reg_func_itr->second.as_string();
    return std::make_shared<Bmi_C_Adapter>(
            Bmi_C_Adapter(
                    lib_file,
                    get_bmi_init_config(),
                    get_forcing_file_path(),
                    is_bmi_using_forcing_file(),
                    get_allow_model_exceed_end_time(),
                    is_bmi_model_time_step_fixed(),
                    reg_func,
                    output));
}

time_t Bmi_C_Formulation::convert_model_time(const double &model_time) {
    return (time_t) (get_bmi_model()->convert_model_time_to_seconds(model_time));
}

const vector<string> Bmi_C_Formulation::get_bmi_input_variables() {
    return get_bmi_model()->GetInputVarNames();
}

const vector<string> Bmi_C_Formulation::get_bmi_output_variables() {
    return get_bmi_model()->GetOutputVarNames();
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
        t_delta_model_units = get_bmi_model()->convert_seconds_to_model_time((double)t_delta);
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

bool Bmi_C_Formulation::is_bmi_input_variable(const std::string &var_name) {
    const std::vector<std::string> names = get_bmi_model()->GetInputVarNames();
    return std::any_of(names.cbegin(), names.cend(), [var_name](const std::string &s){ return var_name == s; });
}

bool Bmi_C_Formulation::is_bmi_output_variable(const string &var_name) {
    const std::vector<std::string> names = get_bmi_model()->GetOutputVarNames();
    return std::any_of(names.cbegin(), names.cend(), [var_name](const std::string &s){ return var_name == s; });
}

bool Bmi_C_Formulation::is_model_initialized() {
    return get_bmi_model()->is_model_initialized();
}
