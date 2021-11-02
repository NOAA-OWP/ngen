#include "Bmi_Multi_Formulation.hpp"
#include "Formulation_Constructors.hpp"
#include "Bmi_Formulation.hpp"
#include <iostream>
#include "Bmi_Py_Formulation.hpp"
#include <WrappedForcingProvider.hpp>

using namespace realization;

void Bmi_Multi_Formulation::create_multi_formulation(geojson::PropertyMap properties, bool needs_param_validation) {
    if (needs_param_validation) {
        validate_parameters(properties);
    }
    // Required parameters first, except for "modules"
    set_bmi_main_output_var(properties.at(BMI_REALIZATION_CFG_PARAM_REQ__MAIN_OUT_VAR).as_string());
    set_model_type_name(properties.at(BMI_REALIZATION_CFG_PARAM_REQ__MODEL_TYPE).as_string());

    std::shared_ptr<forcing::WrappedForcingProvider> forcing_provider = std::make_shared<forcing::WrappedForcingProvider>(forcing.get());
    for (const std::string &forcing_name_or_alias : forcing->get_available_forcing_outputs()) {
        availableData[forcing_name_or_alias] = forcing_provider;
    }

    // TODO: go back and set this up properly in required params collection
    auto nested_module_configs_it = properties.find(BMI_REALIZATION_CFG_PARAM_REQ__MODULES);
    std::vector<geojson::JSONProperty> nested_module_configs = nested_module_configs_it->second.as_list();
    // By default, have the "primary" module be the last
    primary_module_index = nested_module_configs.size() - 1;
    modules = std::vector<nested_module_ptr>(nested_module_configs.size());
    module_types = std::vector<std::string>(nested_module_configs.size());
    module_variable_maps = std::vector<std::shared_ptr<std::map<std::string, std::string>>>(modules.size());

    /* ************************ Begin outer loop: "for sub_formulations_list" ************************ */
    for (size_t i = 0; i < nested_module_configs.size(); ++i) {
        geojson::JSONProperty formulation_config = nested_module_configs[i];
        std::string type_name = formulation_config.at("name").as_string();
        std::string identifier = get_catchment_id() + "." + std::to_string(i);
        nested_module_ptr module = nullptr;
        bool inactive_type_requested = false;
        module_types[i] = type_name;
        if (type_name == "bmi_c") {
            #ifdef NGEN_BMI_C_LIB_ACTIVE
            module = init_nested_module<Bmi_C_Formulation>(i, identifier, formulation_config.at("params").get_values());
            #else  // NGEN_BMI_C_LIB_ACTIVE
            inactive_type_requested = true;
            #endif // NGEN_BMI_C_LIB_ACTIVE
        }
        if (type_name == "bmi_fortran") {

            #ifdef NGEN_BMI_FORTRAN_ACTIVE
            module = init_nested_module<Bmi_Fortran_Formulation>(i, identifier, formulation_config.at("params").get_values());
            #else // NGEN_BMI_FORTRAN_ACTIVE
            inactive_type_requested = true;
            #endif // NGEN_BMI_FORTRAN_ACTIVE
        }
        if (type_name == "bmi_python") {
            #ifdef ACTIVATE_PYTHON
            module = init_nested_module<Bmi_Py_Formulation>(i, identifier, formulation_config.at("params").get_values());
            #else // ACTIVATE_PYTHON
            inactive_type_requested = true;
            #endif // ACTIVATE_PYTHON
        }
        if (inactive_type_requested) {
            throw runtime_error(
                    get_formulation_type() + " could not initialize sub formulation of type " + type_name +
                    " due to support for this type not being activated.");
        }
        if (module == nullptr) {
            throw runtime_error(get_formulation_type() + " received unexpected subtype formulation " + type_name);
        }
        modules[i] = module;

    } /* ************************ End outer loop: "for sub_formulations_list" ************************ */

    // TODO: get synced start_time values for all models
    // TODO: get synced end_time values for all models

    // Setup formulation output variable subset and order, if present
    auto out_var_it = properties.find(BMI_REALIZATION_CFG_PARAM_OPT__OUT_VARS);
    if (out_var_it != properties.end()) {
        std::vector<geojson::JSONProperty> out_vars_json_list = out_var_it->second.as_list();
        std::vector<std::string> out_vars(out_vars_json_list.size());
        for (int i = 0; i < out_vars_json_list.size(); ++i) {
            out_vars[i] = out_vars_json_list[i].as_string();
        }
        set_output_variable_names(out_vars);
    }
    // Otherwise, for multi BMI, the BMI output variables of the last nested module should be used.
    else {
        is_out_vars_from_last_mod = true;
        set_output_variable_names(modules.back()->get_output_variable_names());
    }
    // TODO: consider warning if nested module formulations have formulation output variables, as that level of the
    //  config is (at present) going to be ignored (though strictly speaking, this doesn't apply to the last module in
    //  a certain case).

    // Output header fields, if present
    auto out_headers_it = properties.find(BMI_REALIZATION_CFG_PARAM_OPT__OUT_HEADER_FIELDS);
    if (out_headers_it != properties.end()) {
        std::vector<geojson::JSONProperty> out_headers_json_list = out_headers_it->second.as_list();
        std::vector<std::string> out_headers(out_headers_json_list.size());
        for (int i = 0; i < out_headers_json_list.size(); ++i) {
            out_headers[i] = out_headers_json_list[i].as_string();
        }
        // Make sure that we have the same number of headers as we have output values
        if (get_output_variable_names().size() == out_headers.size()) {
            set_output_header_fields(out_headers);
        }
        else {
            std::cerr << "WARN: configured output headers have " << out_headers.size() << " fields, but there are "
                      << get_output_variable_names().size() << " variables in the output" << std::endl;
            set_output_header_fields(get_output_variable_names());
        }
    }
    else {
        set_output_header_fields(get_output_variable_names());
    }
}

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
const bool &Bmi_Multi_Formulation::get_allow_model_exceed_end_time() const {
    for (const nested_module_ptr &m : modules) {
        if (!m->get_allow_model_exceed_end_time())
            return m->get_allow_model_exceed_end_time();
    }
    return modules.back()->get_allow_model_exceed_end_time();
}

/**
 * Get the collection of forcing output property names this instance can provide.
 *
 * For this type, this is the collection of the names/aliases of the BMI output variables for nested modules;
 * i.e., the config-mapped alias for the variable when set in the realization config, or just the name when no
 * alias was included in the configuration.
 *
 * This is part of the @ref ForcingProvider interface.  This interface must be implemented for items of this
 * type to be usable as "forcing" providers for situations when some other object needs to receive as an input
 * (i.e., one of its forcings) a data property output from this object.
 *
 * @return The collection of forcing output property names this instance can provide.
 * @see ForcingProvider
 */
const vector<std::string> &Bmi_Multi_Formulation::get_available_forcing_outputs() {
    if (is_model_initialized() && available_forcings.empty()) {
        for (const nested_module_ptr &module: modules) {
            for (const std::string &out_var_name: module->get_bmi_output_variables()) {
                available_forcings.push_back(module->get_config_mapped_variable_name(out_var_name));
            }
        }
    }
    return available_forcings;
}

const time_t &Bmi_Multi_Formulation::get_bmi_model_start_time_forcing_offset_s() {
    return modules[0]->get_bmi_model_start_time_forcing_offset_s();
}


/**
 * When possible, translate a variable name for a BMI model to an internally recognized name.
 *
 * Because of the implementation of this type, this function can only translate variable names for input or
 * output variables of either the first or last nested BMI module.  In cases when this is not possible, it will
 * return the original parameter.
 *
 * The function will check the first module first, returning if it finds a translation.  Only then will it check the
 * last module.
 *
 * To perform a similar translation between modules, see the overloaded function
 * @ref get_config_mapped_variable_name(string, shared_ptr, shared_ptr).
 *
 * @param model_var_name The BMI variable name to translate so its purpose is recognized internally.
 * @return Either the translated equivalent variable name, or the provided name if there is not a mapping entry.
 * @see get_config_mapped_variable_name(string, shared_ptr, shared_ptr)
 */
const string &Bmi_Multi_Formulation::get_config_mapped_variable_name(const string &model_var_name) {
    return get_config_mapped_variable_name(model_var_name, true, true);
}

const string &Bmi_Multi_Formulation::get_config_mapped_variable_name(const string &model_var_name, bool check_first,
                                                                     bool check_last)
{
    if (check_first) {
        // If an input var in first module, see if we get back a mapping (i.e., not the same thing), and return if so
        if (modules[0]->is_bmi_input_variable(model_var_name)) {
            const string &mapped_name = modules[0]->get_config_mapped_variable_name(model_var_name);
            if (mapped_name != model_var_name)
                return mapped_name;
        }
        // But otherwise, we must continue and (potentially) try the last module
    }

    if (check_last && modules.back()->is_bmi_output_variable(model_var_name))
        return modules.back()->get_config_mapped_variable_name(model_var_name);

    // If we haven't already  returned, there isn't a valid mapping in anything we can check, so return the original
    return model_var_name;
}

/**
 * When possible, translate the name of an output variable for one BMI model to an input variable for another.
 *
 * This function behaves similarly to @ref get_config_mapped_variable_name(string), except that is performs the
 * translation between modules (rather than between a module and the framework).  As such, it is designed for
 * translation between two sequential models, although this is not a requirement for valid execution.
 *
 * The function will first request the mapping for the parameter name from the outputting module, which will either
 * return a mapped name or the original param.  It will check if the returned value is one of the advertised BMI input
 * variable names of the inputting module; if so, it returns that name.  Otherwise, it proceeds.
 *
 * The function then iterates through all the BMI input variable names for the inputting module.  If it finds any that
 * maps to either the original parameter or the mapped name from the outputting module, it returns it.
 *
 * If neither of those find a mapping, then the original parameter is returned.
 *
 * Note that if this is not an output variable name of the outputting module, the function treats this as a no-mapping
 * condition and returns the parameter.
 *
 * @param output_var_name The output variable to be translated.
 * @param out_module The module having the output variable.
 * @param in_module The module needing a translation of ``output_var_name`` to one of its input variable names.
 * @return Either the translated equivalent variable name, or the provided name if there is not a mapping entry.
 */
const string &Bmi_Multi_Formulation::get_config_mapped_variable_name(const string &output_var_name,
                                                                     const shared_ptr<Bmi_Formulation>& out_module,
                                                                     const shared_ptr<Bmi_Formulation>& in_module)
{
    if (!out_module->is_bmi_output_variable(output_var_name))
        return output_var_name;

    const string &mapped_output = out_module->get_config_mapped_variable_name(output_var_name);
    if (in_module->is_bmi_input_variable(mapped_output))
        return mapped_output;

    for (const string &s : in_module->get_bmi_input_variables()) {
        const string &mapped_s = in_module->get_config_mapped_variable_name(s);
        if (mapped_s == output_var_name || mapped_s == mapped_output)
            return mapped_s;
    }
    return output_var_name;
}

// TODO: remove from this level - it belongs (perhaps) as part of the ForcingProvider interface, but is general to it
const string &Bmi_Multi_Formulation::get_forcing_file_path() const {
    return modules[0]->get_forcing_file_path();
}

string Bmi_Multi_Formulation::get_output_line_for_timestep(int timestep, std::string delimiter) {
    // TODO: have to do some figuring out to make sure this isn't ambiguous (i.e., same output var name from two modules)
    // TODO: need to verify that output variable names are valid, or else warn and return default

    // TODO: something must be added to store values if more than the current time step is wanted
    // TODO: if such a thing is added, it should probably be configurable to turn it off
    if (timestep != (next_time_step_index - 1)) {
        throw std::invalid_argument("Only current time step valid when getting multi-module BMI formulation output");
    }
    std::string output_str;

    // Start by first checking whether we are NOT just using the last module's values
    // Doing it this way so that, if there is a problem with the config, we can later fall back to this method
    if (!is_out_vars_from_last_mod) {
        for (const std::string &out_var_name: get_output_variable_names()) {
            auto output_data_provider_iter = availableData.find(out_var_name);
            // If something unrecognized was requested to be in the formulation output ...
            if (output_data_provider_iter == availableData.end()) {
                // ... print warning ...
                std::cerr << "WARN: unrecognized variable '" << out_var_name
                          << "' configured for formulation output; reverting to default behavior for multi-BMI "
                             "formulation type (using last module)";
                output_str.clear();                 // ... clear any temporary output contents being staged ...
                is_out_vars_from_last_mod = true;   // ... revert to default behavior (just use last nested module) ...
                break;                              // ... and break out of this loop through out_var_names
            }
            else {
                // TODO: right now, not a easy way to properly support an output coming from something other than a
                //  nested formulation, so restrict this for now, but come back to improve things later.
                try {
                    std::shared_ptr <Bmi_Module_Formulation<bmi::Bmi>> nested_module =
                            std::dynamic_pointer_cast < Bmi_Module_Formulation <
                            bmi::Bmi >> (output_data_provider_iter->second);
                    output_str += (output_str.empty() ? "" : ",") +
                                  std::to_string(nested_module->get_var_value_as_double(out_var_name));
                }
                // Similarly, if there was any problem with the cast and extraction of the value, revert
                catch (std::exception &e) {
                    std::cerr << "WARN: variable '" << out_var_name
                            << "' is not an expected output from a nest module of this multi-module BMI formulation"
                               "; reverting to used of outputs from last module";
                    output_str.clear();
                    is_out_vars_from_last_mod = true;
                    break;
                }
            }
        }
    }

    // Now, check this again (i.e., we could have either not entered the previous "if" block, or started it but then
    // exited after a problem and needed to revert to this method)
    if (!is_out_vars_from_last_mod) {
        // If we are (still) not using vars from the last module, this means the last "if" has built the output
        return output_str;
    }
    else {
        return modules.back()->get_output_line_for_timestep(timestep, delimiter);
    }
}

double Bmi_Multi_Formulation::get_response(time_step_t t_index, time_step_t t_delta) {
    if (modules.empty()) {
        throw std::runtime_error("Trying to get response of improperly created empty BMI multi-module formulation.");
    }
    if (t_index < 0) {
        throw std::invalid_argument(
                "Getting response of negative time step in BMI multi-module formulation is not allowed.");
    }
    // Use (next_time_step_index - 1) so that second call with current time step index still works
    if (t_index < (next_time_step_index - 1)) {
        // Make sure to support if we ever in the future (optionally) store and return historic values
        throw std::invalid_argument("Getting response of previous time step in BMI C formulation is not allowed.");
    }

    // The time step delta size, expressed in the units internally used by the model
    double t_delta_model_units;
    if (next_time_step_index <= t_index && is_time_step_beyond_end_time(t_index)) {
        throw std::invalid_argument("Cannot process BMI multi-module formulation to get response of future time "
                                    "step that exceeds model end time.");
    }

    // We need to make sure we can safely move forward this far in time for all the modules
    time_t initial_time_seconds, model_end_time_seconds, post_processing_time_seconds;
    // I.e., "current" time step is one less than the "next" one
    int num_time_steps_to_process = t_index - (next_time_step_index - 1);
    // Also, we should only need this once, because the models should remain in sync (just use first module)
    initial_time_seconds = modules[0]->convert_model_time(modules[0]->get_model_current_time());
    post_processing_time_seconds = initial_time_seconds + (num_time_steps_to_process * t_delta);

    for (nested_module_ptr &module : modules) {
        // We are good for any that are allow to exceed end_time
        if (module->get_allow_model_exceed_end_time()) {
            continue;
        }
        model_end_time_seconds = module->convert_model_time(module->get_model_end_time());

        if (post_processing_time_seconds > model_end_time_seconds) {
            throw std::invalid_argument("Cannot process BMI multi-module formulation to get response of future time "
                                        "step that exceeds model end time.");
        }
    }

    while (next_time_step_index <= t_index) {
        for (nested_module_ptr &module : modules) {
            // By setting up in create function, these will now have their own providers
            module->get_response(t_index, t_delta);
        }
        next_time_step_index++;
    }
    // Find the right module for the main output, checking primary first
    int index = get_index_for_primary_module();
    std::vector<std::string> out_var_names = modules[index]->get_output_variable_names();
    // If we don't find it there, look through the others
    if (std::find(out_var_names.begin(), out_var_names.end(), get_bmi_main_output_var()) == out_var_names.end()) {
        for (int i = 0; i < modules.size(); ++i) {
            if (i == index) continue;
            out_var_names = modules[i]->get_output_variable_names();
            if (std::find(out_var_names.begin(), out_var_names.end(), get_bmi_main_output_var()) != out_var_names.end()) {
                index = i;
                break;
            }
        }
    }
    // With the module index, we can also pick the type
    #ifdef NGEN_BMI_C_LIB_ACTIVE
    if (module_types[index] == "bmi_c") {
        return get_module_var_value_as_double<Bmi_C_Formulation>(get_bmi_main_output_var(), modules[index]);
    }
    #endif // NGEN_BMI_C_LIB_ACTIVE
    #ifdef NGEN_BMI_FORTRAN_ACTIVE
    if (module_types[index] == "bmi_fortran") {
        return get_module_var_value_as_double<Bmi_Fortran_Formulation>(get_bmi_main_output_var(), modules[index]);
    }
    #endif // NGEN_BMI_FORTRAN_ACTIVE
    #ifdef ACTIVATE_PYTHON
    if (module_types[index] == "bmi_python") {
        return get_module_var_value_as_double<Bmi_Py_Formulation>(get_bmi_main_output_var(), modules[index]);
    }
    #endif // ACTIVATE_PYTHON
    throw runtime_error(get_formulation_type() + " unimplemented type " + module_types[index] +
                        " in get_response for main return value");
}

bool Bmi_Multi_Formulation::is_bmi_input_variable(const string &var_name) {
    return modules[0]->is_bmi_input_variable(var_name);
}

bool Bmi_Multi_Formulation::is_bmi_model_time_step_fixed() {
    return std::all_of(modules.cbegin(), modules.cend(),
                       [](const std::shared_ptr<Bmi_Formulation>& m) { return m->is_bmi_model_time_step_fixed(); });
}

bool Bmi_Multi_Formulation::is_bmi_output_variable(const string &var_name) {
    return modules.back()->is_bmi_output_variable(var_name);
}

bool Bmi_Multi_Formulation::is_bmi_using_forcing_file() const {
    return std::any_of(modules.cbegin(), modules.cend(),
                       [](const std::shared_ptr<Bmi_Formulation>& m) { return m->is_bmi_using_forcing_file(); });
}

bool Bmi_Multi_Formulation::is_model_initialized() {
    return std::all_of(modules.cbegin(), modules.cend(),
                       [](const std::shared_ptr<Bmi_Formulation>& m) { return m->is_model_initialized(); });
}

/**
 * Get whether this time step goes beyond this formulations (i.e., any of it's modules') end time.
 *
 * @param t_index The time step index in question.
 * @return Whether this time step goes beyond this formulations (i.e., any of it's modules') end time.
 */
bool Bmi_Multi_Formulation::is_time_step_beyond_end_time(time_step_t t_index) {
    // TODO: implement
    return false;
}
