#include "Bmi_Multi_Formulation.hpp"
#include "Formulation_Constructors.hpp"

using namespace realization;

void Bmi_Multi_Formulation::create_multi_formulation(geojson::PropertyMap properties, bool needs_param_validation) {
    if (needs_param_validation) {
        validate_parameters(properties);
    }
    // Required parameters first, except for "modules"
    set_bmi_main_output_var(properties.at(BMI_REALIZATION_CFG_PARAM_REQ__MAIN_OUT_VAR).as_string());
    set_model_type_name(properties.at(BMI_REALIZATION_CFG_PARAM_REQ__MODEL_TYPE).as_string());

    // TODO: go back and set this up properly in required params collection
    auto sub_formulation_it = properties.find(BMI_REALIZATION_CFG_PARAM_REQ__MODULES);
    std::vector<geojson::JSONProperty> sub_formulations_list = sub_formulation_it->second.as_list();
    modules = std::vector<std::shared_ptr<Bmi_Module_Formulation<bmi::Bmi>>>(sub_formulations_list.size());
    module_variable_maps = std::vector<std::shared_ptr<std::map<std::string, std::string>>>(modules.size());

    // TODO: move inner for loops to separate functions
    /* ************************ Begin outer loop: "for sub_formulations_list" ************************ */
    for (size_t i = 0; i < sub_formulations_list.size(); ++i) {
        geojson::JSONProperty formulation_config = sub_formulations_list[i];
        std::string type_name = formulation_config.at("name").as_string();
        std::string identifier = get_catchment_id() + "." + std::to_string(i);
        std::shared_ptr<Bmi_Module_Formulation<bmi::Bmi>> module;
        if (type_name == "bmi_c") {
            module = std::dynamic_pointer_cast<Bmi_Module_Formulation<bmi::Bmi>>(std::make_shared<Bmi_C_Formulation>(identifier, forcing, output));
        }
        else {
            throw runtime_error(get_formulation_type() + " received unexpected subtype formulation " + type_name);
        }
        modules[i] = module;
        // Call create_formulation on each formulation
        module->create_formulation(formulation_config.at("params").get_values());

        // Set this up for placing in the module_variable_maps member variable
        std::shared_ptr<std::map<std::string, std::string>> var_aliases;
        var_aliases = std::make_shared<std::map<std::string, std::string>>(
                std::map<std::string, std::string>());

        for (const std::string &var_name : module->get_bmi_input_variables()) {
            std::string framework_alias = module->get_config_mapped_variable_name(var_name);
            (*var_aliases)[framework_alias] = var_name;
            // If framework_name is not in collection from which we have available data sources ...
            if (availableData.count(framework_alias) != 1) {
                throw std::runtime_error(
                        "Multi BMI cannot be created with module " + module->get_model_type_name() + " with input " +
                        "variable " + framework_alias +
                        (var_name == framework_alias ? "" : " (an alias of BMI variable " + var_name + ")") +
                        " when there is no previously-enabled source for this input.");
            }
            else {
                module->input_forcing_providers[var_name] = availableData[framework_alias];
                module->input_forcing_providers[framework_alias] = availableData[framework_alias];
            }
        }
        // Also add the output variable aliases
        for (const std::string &var_name : module->get_bmi_output_variables()) {
            std::string framework_alias = module->get_config_mapped_variable_name(var_name);
            (*var_aliases)[framework_alias] = var_name;
            if (availableData.count(framework_alias) > 0) {
                throw std::runtime_error(
                        "Multi BMI cannot be created with module " + module->get_model_type_name() +
                        " with output variable " + framework_alias +
                        (var_name == framework_alias ? "" : " (an alias of BMI variable " + var_name + ")") +
                        " because a previous module is using this output variable name/alias.");
            }
            availableData[framework_alias] = module;
        }
        module_variable_maps[i] = var_aliases;
    } /* ************************ End outer loop: "for sub_formulations_list" ************************ */

    // TODO: get synced start_time values for all models
    // TODO: get synced end_time values for all models

    // TODO: set up output_variables (add support for controlling the particular sub-formulation somehow)

    // TODO: account for consistent setting of 'allow_exceed_end_time'

    // Output header fields, if present
    auto out_headers_it = properties.find(BMI_REALIZATION_CFG_PARAM_OPT__OUT_HEADER_FIELDS);
    if (out_headers_it != properties.end()) {
        std::vector<geojson::JSONProperty> out_headers_json_list = out_headers_it->second.as_list();
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
    for (const std::shared_ptr<Bmi_Formulation>& m : modules) {
        if (!m->get_allow_model_exceed_end_time())
            return m->get_allow_model_exceed_end_time();
    }
    return modules.back()->get_allow_model_exceed_end_time();
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

const string &Bmi_Multi_Formulation::get_forcing_file_path() const {
    // TODO: add something that ensures these are set to same path for all modules (allowing some to be unset also)
    return modules[0]->get_forcing_file_path();
}

const vector<std::string> &Bmi_Multi_Formulation::get_output_variable_names() const {
    return modules.back()->get_output_variable_names();
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

    while (next_time_step_index <= t_index) {
        for (size_t i = 0; i < modules.size(); ++i) {
            // By setting up in create function, these will now have their own providers
            modules[i]->get_response(t_index, t_delta);
        }
        next_time_step_index++;
    }
    // We know this safely ...
    std::shared_ptr<Bmi_Module_Formulation<bmi::Bmi>> source = std::dynamic_pointer_cast<Bmi_Module_Formulation<bmi::Bmi>>(availableData[get_bmi_main_output_var()]);
    // TODO: but we may need to add a convert step here in the case of mapping
    return source->get_var_value_as_double(get_bmi_main_output_var());
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
