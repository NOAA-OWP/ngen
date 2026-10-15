#include <NGenConfig.h>
#include <stdexcept>
#include "ewts_ngen/logger.hpp"
#include "state_save_restore/State_Save_Utils.hpp"

#if NGEN_WITH_PYTHON

#include "Bmi_Py_Formulation.hpp"

#define RETURN_PY_DOUBLE_AT_INDEX_IF_TYPE(target, variable_name, indexes, type) \
    if (cxx_type == #type) {\
        type dest; \
        target.get_value_at_indices(variable_name, &dest, indexes, 1, false); \
        return static_cast<double>(dest); \
    }

using namespace realization;
using namespace models::bmi;

using namespace pybind11::literals;

Bmi_Py_Formulation::Bmi_Py_Formulation(std::string id, std::shared_ptr<data_access::GenericDataProvider> forcing, utils::StreamHandler output_stream)
: Bmi_Module_Formulation(id, std::move(forcing), output_stream) { }

std::shared_ptr<Bmi_Adapter> Bmi_Py_Formulation::construct_model(const geojson::PropertyMap &properties) {
    auto python_type_name_iter = properties.find(BMI_REALIZATION_CFG_PARAM_OPT__PYTHON_TYPE_NAME);
    if (python_type_name_iter == properties.end()) {
        std::string msg = "BMI Python formulation requires Python model class type, but none given in config";
        LOG(LogLevel::FATAL, msg);
        throw std::runtime_error(msg);
    }
    //Load a custom module path, if provided
    auto python_module_path_iter = properties.find(BMI_REALIZATION_CFG_PARAM_OPT__PYTHON_MODULE_PATH);
    if(python_module_path_iter != properties.end()){
        utils::ngenPy::InterpreterUtil::addToPyPath(python_module_path_iter->second.as_string());
    }
    std::string python_type_name = python_type_name_iter->second.as_string();

    return std::make_shared<Bmi_Py_Adapter>(
                    get_model_type_name(),
                    get_bmi_init_config(),
                    python_type_name,
                    is_bmi_model_time_step_fixed());
}

time_t realization::Bmi_Py_Formulation::convert_model_time(const double &model_time) const {
    return (time_t) (get_bmi_model()->convert_model_time_to_seconds(model_time));
}

const std::vector<std::string> Bmi_Py_Formulation::get_bmi_input_variables() const {
    return get_bmi_model()->GetInputVarNames();
}

const std::vector<std::string> Bmi_Py_Formulation::get_bmi_output_variables() const {
    return get_bmi_model()->GetOutputVarNames();
}

std::string Bmi_Py_Formulation::get_formulation_type() const {
    return "bmi_py";
}

double Bmi_Py_Formulation::get_var_value_as_double(const int &index, const std::string &var_name) {
    auto model = std::dynamic_pointer_cast<models::bmi::Bmi_Py_Adapter>(get_bmi_model());

    std::string val_type = model->GetVarType(var_name);
    size_t val_item_size = (size_t)model->GetVarItemsize(var_name);
    std::string cxx_type = model->get_analogous_cxx_type(val_type, val_item_size);

    //void *dest;
    int indices[1];
    indices[0] = index;
    // macro for both checking and converting based on type from get_analogous_cxx_type

    RETURN_PY_DOUBLE_AT_INDEX_IF_TYPE(*model, var_name, indices, signed char)
    else RETURN_PY_DOUBLE_AT_INDEX_IF_TYPE(*model, var_name, indices, unsigned char)
    else RETURN_PY_DOUBLE_AT_INDEX_IF_TYPE(*model, var_name, indices, short)
    else RETURN_PY_DOUBLE_AT_INDEX_IF_TYPE(*model, var_name, indices, unsigned short)
    else RETURN_PY_DOUBLE_AT_INDEX_IF_TYPE(*model, var_name, indices, int)
    else RETURN_PY_DOUBLE_AT_INDEX_IF_TYPE(*model, var_name, indices, unsigned int)
    else RETURN_PY_DOUBLE_AT_INDEX_IF_TYPE(*model, var_name, indices, long)
    else RETURN_PY_DOUBLE_AT_INDEX_IF_TYPE(*model, var_name, indices, unsigned long)
    else RETURN_PY_DOUBLE_AT_INDEX_IF_TYPE(*model, var_name, indices, long long)
    else RETURN_PY_DOUBLE_AT_INDEX_IF_TYPE(*model, var_name, indices, unsigned long long)
    else RETURN_PY_DOUBLE_AT_INDEX_IF_TYPE(*model, var_name, indices, float)
    else RETURN_PY_DOUBLE_AT_INDEX_IF_TYPE(*model, var_name, indices, double)
    else RETURN_PY_DOUBLE_AT_INDEX_IF_TYPE(*model, var_name, indices, long double)
    std::string msg = "Unable to get value of variable " + var_name + " from " + get_model_type_name() +
                      " as double: no logic for converting variable type " + val_type;
    LOG(LogLevel::FATAL, msg);
    throw std::runtime_error(msg);
}

bool Bmi_Py_Formulation::is_bmi_input_variable(const std::string &var_name) const {
    const std::vector<std::string> names = get_bmi_model()->GetInputVarNames();
    return std::any_of(names.cbegin(), names.cend(), [var_name](const std::string &s){ return var_name == s; });
}

bool Bmi_Py_Formulation::is_bmi_output_variable(const std::string &var_name) const {
    const std::vector<std::string> names = get_bmi_model()->GetOutputVarNames();
    return std::any_of(names.cbegin(), names.cend(), [var_name](const std::string &s){ return var_name == s; });
}

bool Bmi_Py_Formulation::is_model_initialized() const {
    return get_bmi_model()->is_model_initialized();
}

void Bmi_Py_Formulation::load_serialization_state(const boost::span<char> state) {
    auto bmi = std::dynamic_pointer_cast<models::bmi::Bmi_Py_Adapter>(get_bmi_model());
    // load the state through the set value function that does not enforce the input size is the same as the current BMI's size
    bmi->set_value_unchecked<char>(StateSaveNames::STATE, state.data(), state.size());
}

#undef RETURN_PY_DOUBLE_AT_INDEX_IF_TYPE
#endif //NGEN_WITH_PYTHON
