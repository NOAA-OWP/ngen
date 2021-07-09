#ifdef ACTIVATE_PYTHON

#include "Bmi_Py_Formulation.hpp"

using namespace realization;
using namespace models::bmi;

using namespace pybind11::literals;

std::string Bmi_Py_Formulation::get_formulation_type() {
    return "bmi_py";
}

bool Bmi_Py_Formulation::is_bmi_input_variable(const string &var_name) {
    const std::vector<std::string> names = get_bmi_model()->GetInputVarNames();
    return std::any_of(names.cbegin(), names.cend(), [var_name](const std::string &s){ return var_name == s; });
}

bool Bmi_Py_Formulation::is_bmi_output_variable(const string &var_name) {
    const std::vector<std::string> names = get_bmi_model()->GetOutputVarNames();
    return std::any_of(names.cbegin(), names.cend(), [var_name](const std::string &s){ return var_name == s; });
}

#endif //ACTIVATE_PYTHON
