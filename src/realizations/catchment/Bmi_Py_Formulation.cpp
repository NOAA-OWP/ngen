#ifdef ACTIVATE_PYTHON

#include "Bmi_Py_Formulation.hpp"

using namespace realization;

using namespace pybind11::literals;

inline void Bmi_Py_Formulation::create_bmi_model_object(geojson::PropertyMap &properties) {
    // Read the configured name of the model type and use it to get a reference for the BMI model type/class
    // TODO: document this needs to be a fully qualified name for the Python BMI model type
    init_py_bmi_type(properties.at(CFG_REQ_PROP_KEY_MODEL_TYPE).as_string());

    // TODO: later, support reading __init__ args (as done below commented out), but for now, just assume no arg.
    set_bmi_model(std::make_shared<pybind11::object>((*py_bmi_type_ref)()));

    /*
    // Create the BMI model object of the appropriate type using the type reference
    auto it_py_init_args = properties.find(CFG_PROP_KEY_MODEL_PYTHON_INIT_ARGS);
    if (it_py_init_args != properties.end() && !it_py_init_args->second.get_values().empty()) {
        // If any are present, use __init__ args from formulation config when creating the model
        // TODO: document how __init__args for model object should be put into formulation configuration.
        pybind11::dict kwargs = pybind11::dict();
        for (auto& child : it_py_init_args->second.get_values()) {
            const char* name = child.first.c_str();
            pybind11::arg kw_arg(child.first.c_str());

            if (child.second.get_type() == geojson::PropertyType::Boolean) {
                kw_arg = child.second.as_boolean();
            }
            else if (child.second.get_type() == geojson::PropertyType::Natural) {
                kw_arg = child.second.as_natural_number();
            }
            else if (child.second.get_type() == geojson::PropertyType::Real) {
                kw_arg = child.second.as_real_number();
            }
            // TODO: think about handling string explicitly, and handling other types; for now, document restrictions
            else {
                kw_arg = child.second.as_string();
            }
            kwargs = pybind11::dict(**kwargs, kw_arg);
        }

    }
    else {
        // If no __init__ args are present, just go with no-arg __init__
        set_bmi_model(std::make_shared<pybind11::object>((*py_bmi_type_ref)()));
    }
     */
}

void Bmi_Py_Formulation::create_formulation(boost::property_tree::ptree &config, geojson::PropertyMap *global) {
    // In this case, we know interpret_parameters() will call call validate parameters, so they don't need re-validating
    protected_create_formulation(this->interpret_parameters(config, global), false);
}

void Bmi_Py_Formulation::create_formulation(geojson::PropertyMap properties) {
    protected_create_formulation(properties, true);
}

std::string Bmi_Py_Formulation::get_bmi_type_package() const {
    return py_bmi_type_package_name == nullptr ? "" : *py_bmi_type_package_name;
}

std::string Bmi_Py_Formulation::get_bmi_type_simple_name() const {
    return py_bmi_type_simple_name == nullptr ? "" : *py_bmi_type_simple_name;
}

std::string Bmi_Py_Formulation::get_formulation_type() {
    return "bmi_py";
}

inline void Bmi_Py_Formulation::init_py_bmi_type(std::string type_name) {
    // TODO: consider how to handle if this gets called when this is not null
    if (py_bmi_type_ref == nullptr) {
        std::vector<std::string> split_name;
        std::string delimiter = ".";

        size_t pos = 0;
        std::string token;
        while ((pos = type_name.find(delimiter)) != std::string::npos) {
            token = type_name.substr(0, pos);
            split_name.emplace_back(token);
            type_name.erase(0, pos + delimiter.length());
        }

        py_bmi_type_simple_name = std::make_shared<std::string>(split_name.back());
        split_name.pop_back();
        py_bmi_type_package_name = std::make_shared<std::string>(boost::algorithm::join(split_name, delimiter));

        // Equivalent to "from package_name import TypeName"
        const char* type_name_c_string = py_bmi_type_simple_name->c_str();
        py_bmi_type_ref = std::make_shared<pybind11::object>(
                pybind11::module_::import(py_bmi_type_package_name->c_str()).attr(type_name_c_string));
    }
}

/**
 * Parse variable value(s) from within "other_vars" property of formulation config, to a numpy array suitable for
 * passing to the BMI model via the ``set_value`` function.
 *
 * @param other_value_json JSON node containing variable/parameter value(s) needing to be passed to a BMI model.
 * @return A bound Python numpy array to containing values to pass to a BMI model object via ``set_value``.
 */
// TODO: unit test
pybind11::array Bmi_Py_Formulation::parse_other_var_val_for_setter(const geojson::JSONProperty& other_value_json) {
    pybind11::module_ np = pybind11::module_::import("numpy");

    if (other_value_json.get_type() == geojson::PropertyType::Boolean) {
        return pybind11::array(pybind11::dtype::of<bool>(), {1, 1}, {other_value_json.as_boolean()});
    }
    else if (other_value_json.get_type() == geojson::PropertyType::Natural) {
        return pybind11::array(pybind11::dtype::of<long>(), {1, 1}, {other_value_json.as_natural_number()});
    }
    else if (other_value_json.get_type() == geojson::PropertyType::Real) {
        return pybind11::array(pybind11::dtype::of<double>(), {1, 1}, {other_value_json.as_real_number()});
    }
        // TODO: think about handling list explicitly, and handling object type; for now, document restrictions
        //else if (other_value_json.get_type() == geojson::PropertyType::String) {
    else {
        return pybind11::array(pybind11::dtype::of<std::string>(), {1, 1}, {other_value_json.as_string()});
    }
}

void Bmi_Py_Formulation::protected_create_formulation(geojson::PropertyMap properties, bool params_need_validation) {
    // TODO: see comment in Tshirt_C_Realization about how a factory might be better.
    if (params_need_validation) {
        this->validate_parameters(properties);
    }

    // Create BMI model object from config and set member variable
    create_bmi_model_object(properties);

    // Get configured BMI config file path, if available, to pass to BMI initialize function
    // TODO: document requirements around this,
    auto it_cfg_file = properties.find(CFG_PROP_KEY_CONFIG_FILE);
    std::string bmi_config_file_path;
    if (it_cfg_file != properties.end()) {
        bmi_config_file_path = it_cfg_file->second.as_string();
    }

    // Then perform the BMI initialization for the model
    get_bmi_model()->attr("Initialize")(bmi_config_file_path);

    // Get the child map of any other variables that will be manually/individually set after BMI initialize function
    // TODO: document that any listed input variable is allowed to do this
    auto it_other_vars = properties.find(CFG_PROP_KEY_OTHER_INPUT_VARS);
    // TODO: consider adding error message if something in list other than input variables (output and/or unrecognized)
    if (it_other_vars != properties.end()) {
        geojson::JSONProperty other_vars = it_other_vars->second;
        pybind11::tuple in_var_names_tuple = get_bmi_model()->attr("get_input_var_names")();
        for (auto && tuple_item : in_var_names_tuple) {
            std::string var_name = pybind11::str(tuple_item);
            if (other_vars.has_key(var_name)) {
                get_bmi_model()->attr("set_value")(var_name, parse_other_var_val_for_setter(other_vars.at(var_name)));
            }
        }
    }
}

#endif //ACTIVATE_PYTHON