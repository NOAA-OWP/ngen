#ifdef ACTIVATE_PYTHON

#include <utility>

#include "Bmi_Py_Adapter.hpp"
#include "boost/algorithm/string.hpp"

using namespace models::bmi;

Bmi_Py_Adapter::Bmi_Py_Adapter(const std::string &type_name) : Bmi_Py_Adapter(type_name, "") { }

Bmi_Py_Adapter::Bmi_Py_Adapter(const std::string &type_name, const std::string& bmi_init_config) {
    // Read the name of the model type and use it to get a reference for the BMI model type/class
    // TODO: document this needs to be a fully qualified name for the Python BMI model type
    init_py_bmi_type(type_name);

    // TODO: later, support reading __init__ args (as done below commented out), but for now, just assume no arg.
    py_bmi_model_obj = std::make_shared<py::object>((*py_bmi_type_ref)());

    /* ***********************************
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
    *********************************** */

    // Then perform the BMI initialization for the model
    py_bmi_model_obj->attr("Initialize")(bmi_init_config);
}

Bmi_Py_Adapter::Bmi_Py_Adapter(const std::string &type_name, const geojson::JSONProperty& other_input_vars)
    : Bmi_Py_Adapter(type_name, "", other_input_vars) {}

Bmi_Py_Adapter::Bmi_Py_Adapter(const std::string &type_name, const std::string& bmi_init_config,
                               const geojson::JSONProperty& other_vars)
    : Bmi_Py_Adapter(type_name, bmi_init_config)
{
    py::tuple in_var_names_tuple = py_bmi_model_obj->attr("get_input_var_names")();
    // TODO: consider adding error message if something in other vars other than input variables (output and/or unrecognized)
    for (auto && tuple_item : in_var_names_tuple) {
        std::string var_name = pybind11::str(tuple_item);
        if (other_vars.has_key(var_name)) {
            py_bmi_model_obj->attr("set_value")(var_name, parse_other_var_val_for_setter(other_vars.at(var_name)));
        }
    }
}

std::string Bmi_Py_Adapter::get_bmi_type_package() const {
    return py_bmi_type_package_name == nullptr ? "" : *py_bmi_type_package_name;
}

std::string Bmi_Py_Adapter::get_bmi_type_simple_name() const {
    return py_bmi_type_simple_name == nullptr ? "" : *py_bmi_type_simple_name;
}

inline void Bmi_Py_Adapter::init_py_bmi_type(std::string type_name) {
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
py::array Bmi_Py_Adapter::parse_other_var_val_for_setter(const geojson::JSONProperty& other_value_json) {
    pybind11::module_ np = py::module_::import("numpy");

    if (other_value_json.get_type() == geojson::PropertyType::Boolean) {
        return pybind11::array(py::dtype::of<bool>(), {1, 1}, {other_value_json.as_boolean()});
    }
    else if (other_value_json.get_type() == geojson::PropertyType::Natural) {
        return pybind11::array(py::dtype::of<long>(), {1, 1}, {other_value_json.as_natural_number()});
    }
    else if (other_value_json.get_type() == geojson::PropertyType::Real) {
        return pybind11::array(py::dtype::of<double>(), {1, 1}, {other_value_json.as_real_number()});
    }
        // TODO: think about handling list explicitly, and handling object type; for now, document restrictions
        //else if (other_value_json.get_type() == geojson::PropertyType::String) {
    else {
        return pybind11::array(py::dtype::of<std::string>(), {1, 1}, {other_value_json.as_string()});
    }
}

#endif //ACTIVATE_PYTHON