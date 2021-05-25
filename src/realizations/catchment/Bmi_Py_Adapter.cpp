#ifdef ACTIVATE_PYTHON

#include <exception>
#include <utility>

#include "Bmi_Py_Adapter.hpp"
#include "boost/algorithm/string.hpp"

using namespace models::bmi;

Bmi_Py_Adapter::Bmi_Py_Adapter(const std::string &type_name, utils::StreamHandler output)
    : Bmi_Py_Adapter(type_name, "", output) { }

Bmi_Py_Adapter::Bmi_Py_Adapter(const std::string &type_name, std::string bmi_init_config,
                               utils::StreamHandler output)
   : bmi_init_config(std::move(bmi_init_config)), output(std::move(output))
{
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
    Initialize();
}

Bmi_Py_Adapter::Bmi_Py_Adapter(const std::string &type_name, const geojson::JSONProperty& other_input_vars,
                               utils::StreamHandler output) : Bmi_Py_Adapter(type_name, "", other_input_vars, output) {}

Bmi_Py_Adapter::Bmi_Py_Adapter(const std::string &type_name, const std::string& bmi_init_config,
                               const geojson::JSONProperty& other_vars, utils::StreamHandler output)
    : Bmi_Py_Adapter(type_name, bmi_init_config, output)
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

int Bmi_Py_Adapter::get_input_item_count() {
    return get_input_var_names().size();
}

std::vector<std::string> Bmi_Py_Adapter::get_input_var_names() {
    if (input_var_names == nullptr) {
        int count = py::int_(py_bmi_model_obj->attr("get_input_item_count")());
        input_var_names = std::make_shared<std::vector<std::string>>(std::vector<std::string>(count));

        py::tuple in_var_names_tuple = py_bmi_model_obj->attr("get_input_var_names")();
        for (auto && tuple_item : in_var_names_tuple) {
            std::string var_name = py::str(tuple_item);
            input_var_names->emplace_back(var_name);
        }
    }

    return *input_var_names;
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
void Bmi_Py_Adapter::Initialize() {
    // If there was a previous init attempt but with failure exception, throw runtime error and include previous message
    if (model_initialized && !init_exception_msg.empty()) {
        throw std::runtime_error("Previous Python model init attempt had exception: \n\t" + init_exception_msg);
    }
    // If there was a previous init attempt with (implicitly) no exception on previous attempt, just return
    else if (model_initialized) {
        return;
    }
    else {
        // Make sure this is set to 'true' after this function call finishes
        model_initialized = true;
        try {
            py_bmi_model_obj->attr("Initialize")(bmi_init_config);
        }
        // Record the exception message before re-throwning to handle subsequent function calls properly
        catch (std::exception& e) {
            init_exception_msg = std::string(e.what());
            // Make sure this is non-empty to be consistent with the above logic
            if (init_exception_msg.empty()) {
                init_exception_msg = "Unknown Python model initialization exception.";
            }
            throw e;
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
void Bmi_Py_Adapter::Initialize(const std::string& config_file) {
    if (config_file != bmi_init_config && model_initialized) {
        throw std::runtime_error(
                "Model init previously attempted; cannot change config from " + bmi_init_config + " to " + config_file);
    }

    if (config_file != bmi_init_config && !model_initialized) {
        output.put("Warning: initialization call changes model config from " + bmi_init_config + " to " + config_file);
        bmi_init_config = config_file;
    }

    Initialize();
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