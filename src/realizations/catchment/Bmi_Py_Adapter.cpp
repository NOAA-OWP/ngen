#ifdef ACTIVATE_PYTHON

#include <exception>
#include <utility>
#include "pybind11/numpy.h"
#include "Bmi_Py_Adapter.hpp"

using namespace models::bmi;
using namespace std;
using namespace pybind11::literals; // to bring in the `_a` literal for pybind11 keyword args functionality

Bmi_Py_Adapter::Bmi_Py_Adapter(const string &type_name, string bmi_init_config, bool allow_exceed_end,
                               bool has_fixed_time_step, const geojson::JSONProperty& other_input_vars,
                               utils::StreamHandler output)
    : Bmi_Py_Adapter(type_name, move(bmi_init_config), "", allow_exceed_end, has_fixed_time_step, other_input_vars,
                     move(output)) {}

Bmi_Py_Adapter::Bmi_Py_Adapter(const string &type_name, string bmi_init_config, string forcing_file_path,
                               bool allow_exceed_end, bool has_fixed_time_step,
                               const geojson::JSONProperty& other_input_vars, utils::StreamHandler output)
                               : Bmi_Adapter<py::object>(type_name +" (BMI Py)", move(bmi_init_config),
                                                         move(forcing_file_path), allow_exceed_end, has_fixed_time_step,
                                                         other_input_vars, output),
                                 np(py::module_::import("numpy")) /* like 'import numpy as np' */ { }

string Bmi_Py_Adapter::GetComponentName() {
    return py::str(bmi_model->attr("get_component_name")());
}

double Bmi_Py_Adapter::GetCurrentTime() {
    // TODO: will need to verify the implicit casting for this works as expected
    return py::float_(bmi_model->attr("get_current_time")());
}

double Bmi_Py_Adapter::GetEndTime() {
    // TODO: will need to verify the implicit casting for this works as expected
    return py::float_(bmi_model->attr("get_end_time")());
}

int Bmi_Py_Adapter::GetInputItemCount() {
    return py::int_(bmi_model->attr("get_input_item_count")());
}

vector<string> Bmi_Py_Adapter::GetInputVarNames() {
    vector<string> in_var_names(GetInputItemCount());
    py::tuple in_var_names_tuple = bmi_model->attr("get_input_var_names")();
    int i = 0;
    for (auto && tuple_item : in_var_names_tuple) {
        string var_name = py::str(tuple_item);
        in_var_names[i++] = var_name;
    }
    return in_var_names;
}

int Bmi_Py_Adapter::GetOutputItemCount() {
    return py::int_(bmi_model->attr("get_output_item_count")());
}

vector<string> Bmi_Py_Adapter::GetOutputVarNames() {
    vector<string> out_var_names(GetOutputItemCount());
    py::tuple out_var_names_tuple = bmi_model->attr("get_output_var_names")();
    int i = 0;
    for (auto && tuple_item : out_var_names_tuple) {
        string var_name = py::str(tuple_item);
        out_var_names[i++] = var_name;
    }
    return out_var_names;
}

double Bmi_Py_Adapter::GetStartTime() {
    // TODO: will need to verify the implicit casting for this works as expected
    return py::float_(bmi_model->attr("get_start_time")());
}

string Bmi_Py_Adapter::GetTimeUnits() {
    return py::str(bmi_model->attr("get_time_units")());
}

double Bmi_Py_Adapter::GetTimeStep() {
    return py::float_(bmi_model->attr("get_time_step")());
}

void Bmi_Py_Adapter::GetValue(string name, void *dest) {
    int num_items = GetVarNbytes(name) / GetVarItemsize(name);
    int indices[num_items];
    get_value_at_indices(name, dest, indices, num_items, true);
}

void Bmi_Py_Adapter::GetValueAtIndices(std::string name, void *dest, int *inds, int count) {
    string val_type = GetVarType(name);
    vector<string> in_v = GetInputVarNames();
    int var_total_items = GetVarNbytes(name) / GetVarItemsize(name);
    get_value_at_indices(name, dest, inds, count, count == var_total_items);
}

void *Bmi_Py_Adapter::GetValuePtr(std::string name) {
    auto ptr_array = bmi_model->attr("get_value_ptr")(name);
    return ((py::array)ptr_array).request().ptr;
}

int Bmi_Py_Adapter::GetVarGrid(std::string name) {
    return py::int_(bmi_model->attr("get_var_grid")(name));
}

int Bmi_Py_Adapter::GetVarItemsize(std::string name) {
    return py::int_(bmi_model->attr("get_var_grid")(name));
}

string Bmi_Py_Adapter::GetVarLocation(std::string name) {
    return py::str(bmi_model->attr("get_var_location")(name));
}

int Bmi_Py_Adapter::GetVarNbytes(std::string name) {
    return py::int_(bmi_model->attr("get_var_nbytes")(name));
}

string Bmi_Py_Adapter::GetVarType(std::string name) {
    return py::str(bmi_model->attr("get_var_type")(name));
}

string Bmi_Py_Adapter::GetVarUnits(std::string name) {
    return py::str(bmi_model->attr("get_var_units")(name));
}

std::string Bmi_Py_Adapter::get_bmi_type_package() const {
    return py_bmi_type_package_name == nullptr ? "" : *py_bmi_type_package_name;
}

std::string Bmi_Py_Adapter::get_bmi_type_simple_name() const {
    return py_bmi_type_simple_name == nullptr ? "" : *py_bmi_type_simple_name;
}

/**
 * Set values for a model's BMI variable at specified indices.
 *
 * This function is required to fulfill the @ref ::bmi::Bmi interface.  It essentially gets the advertised
 * type and size of the variable in question via @ref GetVarType and @ref GetVarItemsize to infer the native
 * type for this variable (i.e., the actual type for the values pointed to by ``src``).  It then uses this
 * as the type param in a nested called to the template-based @ref set_value_at_indices.  If such a type
 * param cannot be determined, a ``runtime_error`` is thrown.
 *
 * @param name The name of the involved BMI variable.
 * @param inds A C++ integer array of indices to update, corresponding to each value in ``src``.
 * @param count Number of elements in the ``inds`` and ``src`` arrays.
 * @param src A C++ array containing the new values to be set in the BMI variable.
 * @throws runtime_error Thrown if @ref GetVarType and @ref GetVarItemsize functions return a combination for
 *                       which there is not support for mapping to a native type in the framework.
 * @see set_value_at_indices
 */
void Bmi_Py_Adapter::SetValueAtIndices(std::string name, int *inds, int count, void *src) {
    string val_type = GetVarType(name);
    size_t val_item_size = (size_t)GetVarItemsize(name);

    // The available types and how they are handled here should match what is in get_value_at_indices
    if (val_type == "int" && val_item_size == sizeof(short)) {
        set_value_at_indices<short>(name, inds, count, src, val_type.c_str());
    } else if (val_type == "int" && val_item_size == sizeof(int)) {
        set_value_at_indices<int>(name, inds, count, src, val_type.c_str());
    } else if (val_type == "int" && val_item_size == sizeof(long)) {
        set_value_at_indices<long>(name, inds, count, src, val_type.c_str());
    } else if (val_type == "int" && val_item_size == sizeof(long long)) {
        set_value_at_indices<long long>(name, inds, count, src, val_type.c_str());
    } else if (val_type == "float" && val_item_size == sizeof(float)) {
        set_value_at_indices<float>(name, inds, count, src, val_type.c_str());
    } else if (val_type == "float" && val_item_size == sizeof(double)) {
        set_value_at_indices<double>(name, inds, count, src, val_type.c_str());
    } else if (val_type == "float" && val_item_size == sizeof(long double)) {
        set_value_at_indices<long double>(name, inds, count, src, val_type.c_str());
    } else {
        throw runtime_error(
                "(Bmi_Py_Adapter) Failed attempt to SET values of BMI variable '" + name + "' from '" +
                model_name + "' model:  model advertises unsupported combination of type (" + val_type +
                ") and size (" + std::to_string(val_item_size) + ").");
    }
}

void Bmi_Py_Adapter::Update() {
    bmi_model->attr("update")();
}

void Bmi_Py_Adapter::UpdateUntil(double time) {
    bmi_model->attr("update_until")(time);
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
        //return pybind11::array(py::dtype::of<string>(), {1, 1}, {other_value_json.as_string()});
        int a = 1;
    }
}

#endif //ACTIVATE_PYTHON
