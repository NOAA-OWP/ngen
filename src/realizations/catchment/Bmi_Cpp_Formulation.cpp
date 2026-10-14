#include "Bmi_Cpp_Formulation.hpp"
#include "bmi_utilities.hpp"
using namespace realization;
using namespace models::bmi;

using ModelCreator = bmi::Bmi* (*)();
using ModelDestoryer = void (*)(bmi::Bmi*);

Bmi_Cpp_Formulation::Bmi_Cpp_Formulation(std::string id, std::shared_ptr<data_access::GenericDataProvider> forcing_provider, utils::StreamHandler output_stream)
    : Bmi_Module_Formulation(id, forcing_provider, output_stream) { }

std::string Bmi_Cpp_Formulation::get_formulation_type() const {
    return "bmi_c++";
}

/**
 * Construct model and its shared pointer.
 *
 * @param properties Configuration properties for the formulation.
 * @return A shared pointer to a newly constructed model adapter object.
 */
std::shared_ptr<Bmi_Adapter> Bmi_Cpp_Formulation::construct_model(const geojson::PropertyMap& properties) {
    auto json_prop_itr = properties.find(BMI_REALIZATION_CFG_PARAM_OPT__LIB_FILE);
    if (json_prop_itr == properties.end()) {
        throw std::runtime_error("BMI C++ formulation requires path to library file, but none provided in config");
    }
    std::string lib_file = json_prop_itr->second.as_string();

    json_prop_itr = properties.find(BMI_REALIZATION_CFG_PARAM_OPT__CPP_CREATE_FUNC);
    std::string model_create_fname =
            json_prop_itr == properties.end() ? BMI_REALIZATION_CFG_PARAM_OPT__CPP_CREATE_FUNC_DEFAULT : json_prop_itr->second.as_string();
            
    json_prop_itr = properties.find(BMI_REALIZATION_CFG_PARAM_OPT__CPP_DESTROY_FUNC);
    std::string model_destroy_fname =
            json_prop_itr == properties.end() ? BMI_REALIZATION_CFG_PARAM_OPT__CPP_DESTROY_FUNC_DEFAULT : json_prop_itr->second.as_string();

    return std::make_shared<Bmi_Cpp_Adapter>(
                    get_model_type_name(),
                    lib_file,
                    get_bmi_init_config(),
                    is_bmi_model_time_step_fixed(),
                    model_create_fname,
                    model_destroy_fname);
}

double Bmi_Cpp_Formulation::get_var_value_as_double(const int& index, const std::string& var_name) {
    // TODO: consider different way of handling (and how to document) cases like long double or unsigned long long that
    //  don't fit or might convert inappropriately
    auto model = std::dynamic_pointer_cast<Bmi_Cpp_Adapter>(get_bmi_model());

    const std::string type = model->GetVarType(var_name);
    try {
        return models::bmi::with_bmi_type(type, [&]<typename T>() -> double {
            return static_cast<double>(model->GetValuePtr<T>(var_name)[index]);
        });
    } catch (const models::bmi::unknown_bmi_type&) {
        throw std::runtime_error("Unable to get value of variable " + var_name + " from " + get_model_type_name() +
                                 " as double: no logic for converting variable type " + type);
    }
}

bool Bmi_Cpp_Formulation::is_bmi_input_variable(const std::string &var_name) const {
    const std::vector<std::string> names = get_bmi_model()->GetInputVarNames();
    return std::any_of(names.cbegin(), names.cend(), [var_name](const std::string &s){ return var_name == s; });
}

bool Bmi_Cpp_Formulation::is_bmi_output_variable(const std::string &var_name) const {
    const std::vector<std::string> names = get_bmi_model()->GetOutputVarNames();
    return std::any_of(names.cbegin(), names.cend(), [var_name](const std::string &s){ return var_name == s; });
}

bool Bmi_Cpp_Formulation::is_model_initialized() const {
    return get_bmi_model()->is_model_initialized();
}
