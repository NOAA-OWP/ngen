#ifdef ACTIVATE_PYTHON

#include "Bmi_Py_Formulation.hpp"

using namespace realization;
using namespace models::bmi;

using namespace pybind11::literals;

Bmi_Py_Formulation::Bmi_Py_Formulation(std::string id, std::shared_ptr<data_access::GenericDataProvider> forcing, utils::StreamHandler output_stream)
: Bmi_Module_Formulation(id, std::move(forcing), output_stream) { }

shared_ptr<Bmi_Adapter> Bmi_Py_Formulation::construct_model(const geojson::PropertyMap &properties) {
    auto python_type_name_iter = properties.find(BMI_REALIZATION_CFG_PARAM_OPT__PYTHON_TYPE_NAME);
    if (python_type_name_iter == properties.end()) {
        throw std::runtime_error("BMI Python formulation requires Python model class type, but none given in config");
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
                    get_allow_model_exceed_end_time(),
                    is_bmi_model_time_step_fixed(),
                    output);
}

time_t realization::Bmi_Py_Formulation::convert_model_time(const double &model_time) {
    return (time_t) (get_bmi_model()->convert_model_time_to_seconds(model_time));
}

const std::vector<std::string> Bmi_Py_Formulation::get_bmi_input_variables() {
    return get_bmi_model()->GetInputVarNames();
}

const std::vector<std::string> Bmi_Py_Formulation::get_bmi_output_variables() {
    return get_bmi_model()->GetOutputVarNames();
}

std::string Bmi_Py_Formulation::get_formulation_type() {
    return "bmi_py";
}

double Bmi_Py_Formulation::get_response(time_step_t t_index, time_step_t t_delta) {
    if (get_bmi_model() == nullptr) {
        throw std::runtime_error("Trying to process response of improperly created BMI Python formulation.");
    }
    if (t_index < 0) {
        throw std::invalid_argument("Getting response of negative time step in BMI Python formulation is not allowed.");
    }
    // Use (next_time_step_index - 1) so that second call with current time step index still works
    if (t_index < (next_time_step_index - 1)) {
        // TODO: consider whether we should (optionally) store and return historic values
        throw std::invalid_argument("Getting response of previous time step in BMI Python formulation is not allowed.");
    }

    // The time step delta size, expressed in the units internally used by the model
    double t_delta_model_units = get_bmi_model()->convert_seconds_to_model_time((double)t_delta);
    if (next_time_step_index <= t_index) {
        double model_time = get_bmi_model()->GetCurrentTime();
        // Also, before running, make sure this doesn't cause a problem with model end_time
        if (!get_allow_model_exceed_end_time()) {
            int total_time_steps_to_process = abs((int)t_index - next_time_step_index) + 1;
            if (get_bmi_model()->GetEndTime() < (model_time + (t_delta_model_units * total_time_steps_to_process))) {
                throw std::invalid_argument("Cannot process BMI Python formulation to get response of future time step "
                                            "that exceeds model end time.");
            }
        }
    }

    while (next_time_step_index <= t_index) {
        double model_initial_time = get_bmi_model()->GetCurrentTime();
        set_model_inputs_prior_to_update(model_initial_time, t_delta);
        if (t_delta_model_units == get_bmi_model()->GetTimeStep())
            get_bmi_model()->Update();
        else
            get_bmi_model()->UpdateUntil(model_initial_time + t_delta_model_units);
        // TODO: again, consider whether we should store any historic response, ts_delta, or other var values
        next_time_step_index++;
    }
    return get_var_value_as_double( get_bmi_main_output_var());
}

double Bmi_Py_Formulation::get_var_value_as_double(const std::string &var_name) {
    return get_var_value_as_double(0, var_name);
}

double Bmi_Py_Formulation::get_var_value_as_double(const int &index, const std::string &var_name) {
    auto model = std::dynamic_pointer_cast<models::bmi::Bmi_Py_Adapter>(get_bmi_model());

    std::string val_type = model->GetVarType(var_name);
    size_t val_item_size = (size_t)model->GetVarItemsize(var_name);

    //void *dest;
    int indices[1];
    indices[0] = index;

    // The available types and how they are handled here should match what is in SetValueAtIndices
    if (val_type == "int" && val_item_size == sizeof(short)) {
        short dest;
        model->get_value_at_indices(var_name, &dest, indices, 1, false);
        return (double)dest;
    }
    if (val_type == "int" && val_item_size == sizeof(int)) {
        int dest;
        model->get_value_at_indices(var_name, &dest, indices, 1, false);
        return (double)dest;
    }
    if (val_type == "int" && val_item_size == sizeof(long)) {
        long dest;
        model->get_value_at_indices(var_name, &dest, indices, 1, false);
        return (double)dest;
    }
    if (val_type == "int" && val_item_size == sizeof(long long)) {
        long long dest;
        model->get_value_at_indices(var_name, &dest, indices, 1, false);
        return (double)dest;
    }
    if (val_type == "float" || val_type == "float16" || val_type == "float32" || val_type == "float64") {
        if (val_item_size == sizeof(float)) {
            float dest;
            model->get_value_at_indices(var_name, &dest, indices, 1, false);
            return (double) dest;
        }
        if (val_item_size == sizeof(double)) {
            double dest;
            model->get_value_at_indices(var_name, &dest, indices, 1, false);
            return dest;
        }
        if (val_item_size == sizeof(long double)) {
            long double dest;
            model->get_value_at_indices(var_name, &dest, indices, 1, false);
            return (double) dest;
        }
    }

    throw std::runtime_error("Unable to get value of variable " + var_name + " from " + get_model_type_name() +
    " as double: no logic for converting variable type " + val_type);
}

bool Bmi_Py_Formulation::is_bmi_input_variable(const std::string &var_name) {
    const std::vector<std::string> names = get_bmi_model()->GetInputVarNames();
    return std::any_of(names.cbegin(), names.cend(), [var_name](const std::string &s){ return var_name == s; });
}

bool Bmi_Py_Formulation::is_bmi_output_variable(const std::string &var_name) {
    const std::vector<std::string> names = get_bmi_model()->GetOutputVarNames();
    return std::any_of(names.cbegin(), names.cend(), [var_name](const std::string &s){ return var_name == s; });
}

bool Bmi_Py_Formulation::is_model_initialized() {
    return get_bmi_model()->is_model_initialized();
}

#endif //ACTIVATE_PYTHON
