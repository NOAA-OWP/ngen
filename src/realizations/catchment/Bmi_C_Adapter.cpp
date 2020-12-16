#include <cstring>
#include <exception>
#include <utility>
#include "FileChecker.h"

#include "Bmi_C_Adapter.hpp"
#include "boost/algorithm/string.hpp"

using namespace models::bmi;

Bmi_C_Adapter::Bmi_C_Adapter(std::string forcing_file_path, bool model_uses_forcing_file, bool allow_exceed_end,
                             utils::StreamHandler output)
        : Bmi_C_Adapter("", std::move(forcing_file_path), model_uses_forcing_file, allow_exceed_end, output) {}

Bmi_C_Adapter::Bmi_C_Adapter(std::string bmi_init_config, std::string forcing_file_path, bool model_uses_forcing_file,
                             bool allow_exceed_end, utils::StreamHandler output)
        : bmi_init_config(std::move(bmi_init_config)), forcing_file_path(std::move(forcing_file_path)),
          bmi_model_uses_forcing_file(model_uses_forcing_file), allow_model_exceed_end_time(allow_exceed_end),
          output(std::move(output)), bmi_model(std::make_shared<Bmi>(Bmi()))
{
    Initialize();
}

Bmi_C_Adapter::Bmi_C_Adapter(std::string forcing_file_path, bool model_uses_forcing_file,
                             bool allow_exceed_end, const geojson::JSONProperty& other_input_vars,
                             utils::StreamHandler output)
        : Bmi_C_Adapter("", std::move(forcing_file_path), model_uses_forcing_file, allow_exceed_end, other_input_vars,
                        output) {}

Bmi_C_Adapter::Bmi_C_Adapter(const std::string& bmi_init_config, std::string forcing_file_path,
                             bool model_uses_forcing_file, bool allow_exceed_end,
                             const geojson::JSONProperty& other_vars, utils::StreamHandler output)
        : Bmi_C_Adapter(bmi_init_config, std::move(forcing_file_path), model_uses_forcing_file, allow_exceed_end, output)
{
    for (const std::string& var_name : GetInputVarNames()) {
        if (other_vars.has_key(var_name)) {
            geojson::JSONProperty other_variable = other_vars.at(var_name);
            geojson::PropertyType var_type = other_variable.get_type();

            // First organize things in a vector of JSON properties, whether we have a list or a single value
            std::vector<geojson::JSONProperty> json_values;
            if (var_type == geojson::PropertyType::List) {
                json_values = other_variable.as_list();
                // Also adjust the var type in this case, to whatever the first element is (leaving if list is empty)
                if (!json_values.empty()) {
                    var_type = json_values[0].get_type();
                }
            }
            else {
                // In this case, we have a single value, so create a single value vector for it
                json_values.emplace_back(other_variable);
            }
            // Store the number of individual values for this input variable for convenience
            const int num_values_from_config = json_values.size();

            // Next, make sure there aren't more values than the model can handle for this variable
            int num_variable_items = GetVarNbytes(var_name) / GetVarItemsize(var_name);
            if (num_values_from_config > num_variable_items) {
                throw std::runtime_error(model_name + " expects variable '" + var_name + "' to be an array of " +
                                         std::to_string(num_variable_items) +
                                         " elements, but config attempted to provided " +
                                         std::to_string(json_values.size()));
            }
            if (num_values_from_config < num_variable_items) {
                output.put("Warning: configuration supplying subset of values for input variable " + var_name);
            }

            // Now create the indexes that will be used by C code to identify indexes of variable's array to set
            int inds[num_values_from_config];
            // In our case, everything aligns; i.e., the value of each inds element should be its index
            for (int i = 0; i < num_values_from_config; i++)
                inds[i] = i;

            // Now, for supported (inner) types, extract the JSON values to a raw array, then use to set values
            if (var_type == geojson::PropertyType::Boolean) {
                bool src[num_values_from_config];
                for (int i = 0; i < num_values_from_config; i++)
                    src[i] = json_values[i].as_boolean();
                SetValueAtIndices(var_name, inds, num_values_from_config, (void*)(src));
            }
            else if (var_type == geojson::PropertyType::Natural) {
                long src[num_values_from_config];
                for (int i = 0; i < num_values_from_config; i++)
                    src[i] = json_values[i].as_natural_number();
                SetValueAtIndices(var_name, inds, num_values_from_config, (void*)(src));
            }
            else if (var_type == geojson::PropertyType::Real) {
                double src[num_values_from_config];
                for (int i = 0; i < num_values_from_config; i++)
                    src[i] = json_values[i].as_real_number();
                SetValueAtIndices(var_name, inds, num_values_from_config, (void*)(src));
            }
            else if (var_type == geojson::PropertyType::String) {
                std::string str_val;
                char* src[num_values_from_config];
                for (int i = 0; i < num_values_from_config; i++) {
                    str_val = json_values[i].as_string();
                    src[i] = {(char*)(str_val.c_str())};
                }
                SetValueAtIndices(var_name, inds, num_values_from_config, (void*)(src));
            }
            // For now, don't support object JSON type
            else {
                std::string type = var_type == geojson::PropertyType::List ? "list" : "object";
                throw std::runtime_error("Unsupported type '" + type + "' for config variable '" + var_name + "'");
            }
        }
        else {
            output.put("Warning: unrecognized 'other_input_variable' `" + var_name +
                       "` presented in formulation config; ignoring");
        }
    }
}

Bmi_C_Adapter::Bmi_C_Adapter(Bmi_C_Adapter &adapter) : model_name(adapter.model_name),
                                                       allow_model_exceed_end_time(adapter.allow_model_exceed_end_time),
                                                       bmi_init_config(adapter.bmi_init_config),
                                                       bmi_model(adapter.bmi_model),
                                                       bmi_model_uses_forcing_file(adapter.bmi_model_uses_forcing_file),
                                                       forcing_file_path(adapter.forcing_file_path),
                                                       init_exception_msg(adapter.init_exception_msg),
                                                       input_var_names(adapter.input_var_names),
                                                       model_initialized(adapter.model_initialized),
                                                       output_var_names(adapter.output_var_names),
                                                       output(std::move(adapter.output)) { }

Bmi_C_Adapter::Bmi_C_Adapter(Bmi_C_Adapter &&adapter) noexcept: model_name(std::move(adapter.model_name)),
                                                                allow_model_exceed_end_time(
                                                                        adapter.allow_model_exceed_end_time),
                                                                bmi_init_config(std::move(adapter.bmi_init_config)),
                                                                bmi_model(std::move(adapter.bmi_model)),
                                                                bmi_model_uses_forcing_file(
                                                                        adapter.bmi_model_uses_forcing_file),
                                                                forcing_file_path(std::move(adapter.forcing_file_path)),
                                                                init_exception_msg(
                                                                        std::move(adapter.init_exception_msg)),
                                                                input_var_names(std::move(adapter.input_var_names)),
                                                                model_initialized(adapter.model_initialized),
                                                                output_var_names(std::move(adapter.output_var_names)),
                                                                output(std::move(std::move(adapter.output))) {}

double Bmi_C_Adapter::convert_model_time_to_seconds(const double& model_time_val) {
    std::string time_units = GetTimeUnits();
    double convert_factor;
    if (time_units == "s" || time_units == "sec" || time_units == "second" || time_units == "seconds")
        convert_factor = 1.0;
    else if (time_units == "m" || time_units == "min" || time_units == "minute" || time_units == "minutes")
        convert_factor = 60.0;
    else if (time_units == "h" || time_units == "hr" || time_units == "hour" || time_units == "hours")
        convert_factor = 3600.0;
    else if (time_units == "d" || time_units == "day" || time_units == "days")
        convert_factor = 86400.0;
    else
        throw std::runtime_error("Invalid model time step units ('" + time_units + "') in BMI C formulation.");
    return model_time_val * convert_factor;
}

void Bmi_C_Adapter::Finalize() {
    if (bmi_model != nullptr) {
        int result = bmi_model->finalize(bmi_model.get());
        if (result != BMI_SUCCESS) {
            throw std::runtime_error("Failed to finalize model successfully");
        }
    }
}

std::shared_ptr<std::vector<std::string>> Bmi_C_Adapter::get_variable_names(bool is_input_variable) {
    int count;
    int count_result;
    if (is_input_variable)
        count_result = bmi_model->get_input_item_count(bmi_model.get(), &count);
    else
        count_result = bmi_model->get_output_item_count(bmi_model.get(), &count);
    if (count_result != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to count of output variable names array.");
    }
    std::shared_ptr<std::vector<std::string>> var_names = std::make_shared<std::vector<std::string>>(std::vector<std::string>(count));

    char* names_array[count];
    for (int i = 0; i < count; i++) {
        names_array[i] = static_cast<char *>(malloc(sizeof(char) * BMI_MAX_VAR_NAME));
    }
    int names_result;
    if (is_input_variable) {
        names_result = bmi_model->get_input_var_names(bmi_model.get(), names_array);
    }
    else {
        names_result = bmi_model->get_output_var_names(bmi_model.get(), names_array);
    }
    if (names_result != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get array of output variables names.");
    }

    for (int i = 0; i < count; ++i) {
        (*var_names)[i] = std::string(names_array[i]);
        free(names_array[i]);
    }
    return var_names;
}

double Bmi_C_Adapter::GetCurrentTime() {
    double current_time;
    int result = bmi_model->get_current_time(bmi_model.get(), &current_time);
    if (result != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get current model time.");
    }
    return current_time;
}

double Bmi_C_Adapter::GetEndTime() {
    double end_time;
    int result = bmi_model->get_end_time(bmi_model.get(), &end_time);
    if (result != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get model end time.");
    }
    return end_time;
}

int Bmi_C_Adapter::GetInputItemCount() {
    return GetInputVarNames().size();
}

std::vector<std::string> Bmi_C_Adapter::GetInputVarNames() {
    if (input_var_names == nullptr) {
        input_var_names = get_variable_names(true);
    }

    return *input_var_names;
}

int Bmi_C_Adapter::GetOutputItemCount() {
    return GetOutputVarNames().size();
}

std::vector<std::string> Bmi_C_Adapter::GetOutputVarNames() {
    if (output_var_names == nullptr) {
        output_var_names = get_variable_names(false);
    }

    return *output_var_names;
}

double Bmi_C_Adapter::GetStartTime() {
    double start_time;
    int result = bmi_model->get_start_time(bmi_model.get(), &start_time);
    if (result != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get model start time.");
    }
    return start_time;
}

/**
 * Get the backing model "next" time step size.
 *
 * Get the time step size used in the backing model for the "next", in the units expressed by
 * `GetTimeUnits()`.
 *
 * This type does not make any assumptions about how the backing model handles time steps, including whether
 * the time step size is fixed.  This creates some complexities.  In particular, this function makes a
 * best-effort for determining the time step size.  It attempts to do this first by analyzing the data in
 * the forcing file at `forcing_file_path`, for whatever the next time step is.  This may not be possible,
 * in which case it falls back to the value returned from the analogous BMI method from the backing model.
 *
 * @return The time step size of the model.
 */
double Bmi_C_Adapter::GetTimeStep() {
    // FIXME: finish implementing properly beyond just reading from model, as noted in docstring above
    double current_time = GetCurrentTime();
    double start_time = GetStartTime();
    std::vector<int> ts_sizes = GetValue<int>("TIME_STEP_DELTA");
    int i = 0;
    while (i < ts_sizes.size() && current_time > start_time)
        current_time -= ts_sizes[i++];
    return (i >= ts_sizes.size()) ? ts_sizes[i-1] : ts_sizes[i];
}

std::string Bmi_C_Adapter::GetTimeUnits() {
    char time_units_cstr[BMI_MAX_UNITS_NAME];
    int result = bmi_model->get_time_units(bmi_model.get(), time_units_cstr);
    if (result != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to read time units from model.");
    }
    return std::string(time_units_cstr);
}

int Bmi_C_Adapter::GetVarItemsize(std::string name) {
    int size;
    int success = bmi_model->get_var_itemsize(bmi_model.get(), name.c_str(), &size);
    if (success != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get variable item size for " + name + ".");
    }
    return size;
}

int Bmi_C_Adapter::GetVarNbytes(std::string name) {
    int size;
    int success = bmi_model->get_var_nbytes(bmi_model.get(), name.c_str(), &size);
    if (success != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get variable array size (i.e., nbytes) for " + name + ".");
    }
    return size;
}

std::string Bmi_C_Adapter::GetVarType(std::string name) {
    char type_c_str[BMI_MAX_TYPE_NAME];
    int success = bmi_model->get_var_type(bmi_model.get(), name.c_str(), type_c_str);
    if (success != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get variable type for " + name + ".");
    }
    std::string type_str(type_c_str);
    return type_str;
}

std::string Bmi_C_Adapter::GetVarUnits(std::string name) {
    char units_c_str[BMI_MAX_TYPE_NAME];
    int success = bmi_model->get_var_units(bmi_model.get(), name.c_str(), units_c_str);
    if (success != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get variable units for " + name + ".");
    }
    std::string units_str(units_c_str);
    return units_str;
}

int Bmi_C_Adapter::get_last_processed_time_step() {
    // FIXME: this will work for now, since we can assume for the moment universal delta_t size, but an alternative is
    //  going to be needed that can go by the data.
    int last_processed_time_step = -1;
    double time_s = convert_model_time_to_seconds(GetStartTime());
    double current_time_s = convert_model_time_to_seconds(GetCurrentTime());
    int ts = GetTimeStep();
    double ts_size_s = convert_model_time_to_seconds(GetTimeStep());

    while (time_s < current_time_s) {
        last_processed_time_step++;
        time_s += ts_size_s;
    }

    // TODO: in theory 'time' and 'current_time' should eventually line up exactly, but do we need to handle case if
    //  they don't?
    return last_processed_time_step;
}

/**
  * Initialize the wrapped BMI model object using the value from the `bmi_init_config` member variable and
  * the object's ``Initialize`` function.
  *
  * If no attempt to initialize the model has yet been made (i.e., ``model_initialized`` is ``false`` when
  * this function is called), then ``model_initialized`` is set to ``true`` and initialization is attempted
  * for the model object. If initialization fails, an exception will be raised, with it's type and message
  * saved as part of this object's state.
  *
  * If an attempt to initialize the model has already been made (i.e., ``model_initialized`` is ``true``),
  * this function will either simply return or will throw a runtime_error, with the message listing the
  * type and message of the exception from the earlier attempt.
  */
void Bmi_C_Adapter::Initialize() {
    // If there was a previous init attempt but with failure exception, throw runtime error and include previous message
    if (model_initialized && !init_exception_msg.empty()) {
        throw std::runtime_error(init_exception_msg + " (from previous attempt)");
    }
    // If there was a previous init attempt with (implicitly) no exception on previous attempt, just return
    else if (model_initialized) {
        return;
    }
    else if (!utils::FileChecker::file_is_readable(bmi_init_config)) {
        init_exception_msg = "Cannot initialize " + model_name + " using unreadable file '" + bmi_init_config + "'";
        throw std::runtime_error(init_exception_msg);
    }
    else {
        // Make sure this is set to 'true' after this function call finishes
        model_initialized = true;
        register_bmi(bmi_model.get());
        int init_result = bmi_model->initialize(bmi_model.get(), bmi_init_config.c_str());
        if (init_result != BMI_SUCCESS) {
            init_exception_msg = "Failure when attempting to initialize " + model_name;
            throw std::runtime_error(init_exception_msg);
        }
    }
}

/**
 * Initialize the wrapped BMI model object using the given config file and the object's ``Initialize``
 * function.
 *
 * If the given file is not the same as what is in `bmi_init_config`` and the model object has not already
 * been initialized, this function will produce a warning message about the difference, then subsequently
 * update `bmi_init_config`` to the given file.  It will then proceed with initialization.
 *
 * However, if the given file is not the same as what is in `bmi_init_config``, but the model has already
 * been initialized, a runtime_error exception is thrown.
 *
 * This otherwise operates using the logic of ``Initialize()``.
 *
 * @param config_file
 * @see Initialize()
 * @throws runtime_error If already initialized but using a different file than the passed argument.
 */
void Bmi_C_Adapter::Initialize(const std::string& config_file) {
    if (config_file != bmi_init_config && model_initialized) {
        throw std::runtime_error(
                model_name + " init previously attempted; cannot change config from " + bmi_init_config + " to " +
                config_file);
    }

    if (config_file != bmi_init_config && !model_initialized) {
        output.put("Warning: initialization call changes " + model_name + " config from " + bmi_init_config + " to " +
                   config_file);
        bmi_init_config = config_file;
    }

    Initialize();
}

void Bmi_C_Adapter::SetValue(std::string name, void *src) {
    int result = bmi_model->set_value(bmi_model.get(), name.c_str(), src);
    if (result != BMI_SUCCESS) {
        throw std::runtime_error("Failed to set values of " + name + " variable for " + model_name);
    }
}

bool Bmi_C_Adapter::is_model_initialized() {
    return model_initialized;
}

template<typename T>
void Bmi_C_Adapter::SetValue(std::string name, std::vector<T> src, size_t src_item_size) {
    // TODO: for safety, check the type and size of src (or what it will be when cast to a void*), which must match the
    //  model’s internal array, and can be determined through get_var_type, get_var_nbytes

    // TODO: may not be able to do this, and will need to resort to manually moving things around.
    SetValue(std::move(name), static_cast<void*>(src.data()));
}

void Bmi_C_Adapter::SetValueAtIndices(std::string name, int *inds, int count, void *src) {
    int result = bmi_model->set_value_at_indices(bmi_model.get(), name.c_str(), inds, count, src);
    if (result != BMI_SUCCESS) {
        throw std::runtime_error("Failed to set specified indexes for " + name + " variable of " + model_name);
    }
}

/*
template<class T>
void Bmi_C_Adapter::SetValueAtIndices(const std::string& name, std::vector<int> inds, std::vector<T> src, size_t src_item_size) {
    void* src_array = malloc(src_item_size * src.size());

    int result = bmi_model->set_value_at_indices(bmi_model.get(), name.c_str(), inds.data(), inds.size(), src_array);
    free(src_array);
    if (result != BMI_SUCCESS) {
        throw std::runtime_error("Failed to set specified indexes for " + name + " variable of " + model_name);
    }
}
 */

void Bmi_C_Adapter::Update() {
    if (!allow_model_exceed_end_time) {
        double current_time = GetCurrentTime();
        double end_time = GetEndTime();
        if (current_time >= end_time) {
            throw std::runtime_error("Model execution update failed due to exceeding end time for " + model_name);
        }
    }

    int result = bmi_model->update(bmi_model.get());
    if (result != BMI_SUCCESS) {
        throw std::runtime_error("Model execution update failed for " + model_name);
    }
}
