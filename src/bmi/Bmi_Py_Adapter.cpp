#include <NGenConfig.h>

#if NGEN_WITH_PYTHON

#include <exception>
#include <utility>
#include <iostream>

#include "bmi/Bmi_Py_Adapter.hpp"

using namespace models::bmi;
using namespace pybind11::literals; // to bring in the `_a` literal for pybind11 keyword args functionality

Bmi_Py_Adapter::Bmi_Py_Adapter(const std::string &type_name, std::string bmi_init_config, const std::string &bmi_python_type,
                               bool has_fixed_time_step)
        : Bmi_Adapter(type_name + " (BMI Py)", std::move(bmi_init_config),
                                  has_fixed_time_step),
          bmi_type_py_full_name(bmi_python_type),
          np(utils::ngenPy::InterpreterUtil::getPyModule("numpy")) /* like 'import numpy as np' */
{
    try {
        construct_and_init_backing_model_for_py_adapter();
        // Make sure this is set to 'true' after this function call finishes
        model_initialized = true;
        bmi_model_time_convert_factor = get_time_convert_factor();
    }
    catch (std::runtime_error& e){ //Catch specific exception and re-throw so type/message isn't erased
        model_initialized = true;
        throw e;
    }
    // Record the exception message before re-throwing to handle subsequent function calls properly
    catch (std::exception &e) {
        // Make sure this is set to 'true' after this function call finishes
        model_initialized = true;
        throw e;
    }
}

std::string Bmi_Py_Adapter::GetComponentName() {
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

std::vector<std::string> Bmi_Py_Adapter::GetInputVarNames() {
    std::vector<std::string> in_var_names(GetInputItemCount());
    py::tuple in_var_names_tuple = bmi_model->attr("get_input_var_names")();
    int i = 0;
    for (auto && tuple_item : in_var_names_tuple) {
        std::string var_name = py::str(tuple_item);
        in_var_names[i++] = var_name;
    }
    return in_var_names;
}

int Bmi_Py_Adapter::GetOutputItemCount() {
    return py::int_(bmi_model->attr("get_output_item_count")());
}

std::vector<std::string> Bmi_Py_Adapter::GetOutputVarNames() {
    std::vector<std::string> out_var_names(GetOutputItemCount());
    py::tuple out_var_names_tuple = bmi_model->attr("get_output_var_names")();
    int i = 0;
    for (auto && tuple_item : out_var_names_tuple) {
        std::string var_name = py::str(tuple_item);
        out_var_names[i++] = var_name;
    }
    return out_var_names;
}

double Bmi_Py_Adapter::GetStartTime() {
    // TODO: will need to verify the implicit casting for this works as expected
    return py::float_(bmi_model->attr("get_start_time")());
}

std::string Bmi_Py_Adapter::GetTimeUnits() {
    return py::str(bmi_model->attr("get_time_units")());
}

double Bmi_Py_Adapter::GetTimeStep() {
    return py::float_(bmi_model->attr("get_time_step")());
}

void Bmi_Py_Adapter::GetValue(std::string name, void *dest) {
    models::bmi::primitive_type type;
    try {
        type = models::bmi::resolve_var_type(*this, name);
    }
    catch (std::runtime_error &e) {
        std::string msg = "Encountered error getting C++ type during call to GetValue: \n";
        msg += e.what();
        throw std::runtime_error(msg);
    }

    models::bmi::with_primitive_type(type, [&]<typename T>() {
        copy_to_array<T>(name, static_cast<T *>(dest));
    });
}

void Bmi_Py_Adapter::GetValueAtIndices(std::string name, void *dest, int *inds, int count) {
    std::string val_type = GetVarType(name);
    std::vector<std::string> in_v = GetInputVarNames();
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
    return py::int_(bmi_model->attr("get_var_itemsize")(name));
}

std::string Bmi_Py_Adapter::GetVarLocation(std::string name) {
    return py::str(bmi_model->attr("get_var_location")(name));
}

int Bmi_Py_Adapter::GetVarNbytes(std::string name) {
    return py::int_(bmi_model->attr("get_var_nbytes")(name));
}

std::string Bmi_Py_Adapter::GetVarType(std::string name) {
    return py::str(bmi_model->attr("get_var_type")(name));
}

std::string Bmi_Py_Adapter::GetVarUnits(std::string name) {
    return py::str(bmi_model->attr("get_var_units")(name));
}

std::string Bmi_Py_Adapter::get_bmi_type_package() const {
    return bmi_type_py_module_name == nullptr ? "" : *bmi_type_py_module_name;
}

std::string Bmi_Py_Adapter::get_bmi_type_simple_name() const {
    return bmi_type_py_class_name == nullptr ? "" : *bmi_type_py_class_name;
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
    models::bmi::with_primitive_type(models::bmi::resolve_var_type(*this, name), [&]<typename T>() {
        set_value_at_indices<T>(name, inds, count, src);
    });
}

void Bmi_Py_Adapter::Update() {
    bmi_model->attr("update")();
}

void Bmi_Py_Adapter::UpdateUntil(double time) {
    bmi_model->attr("update_until")(time);
}

#endif //NGEN_WITH_PYTHON
