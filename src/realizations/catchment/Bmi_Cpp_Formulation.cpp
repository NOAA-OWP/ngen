#include "Bmi_Cpp_Formulation.hpp"
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

    std::string type = model->GetVarType(var_name);
    if (type == "long double")
        return (double) (model->GetValuePtr<long double>(var_name))[index];

    if (type == "double")
        return (double) (model->GetValuePtr<double>(var_name))[index];

    if (type == "float")
        return (double) (model->GetValuePtr<float>(var_name))[index];

    if (type == "short" || type == "short int" || type == "signed short" || type == "signed short int")
        return (double) (model->GetValuePtr<short>(var_name))[index];

    if (type == "unsigned short" || type == "unsigned short int")
        return (double) (model->GetValuePtr<unsigned short>(var_name))[index];

    if (type == "int" || type == "signed" || type == "signed int")
        return (double) (model->GetValuePtr<int>(var_name))[index];

    if (type == "unsigned" || type == "unsigned int")
        return (double) (model->GetValuePtr<unsigned int>(var_name))[index];

    if (type == "long" || type == "long int" || type == "signed long" || type == "signed long int")
        return (double) (model->GetValuePtr<long>(var_name))[index];

    if (type == "unsigned long" || type == "unsigned long int")
        return (double) (model->GetValuePtr<unsigned long>(var_name))[index];

    if (type == "long long" || type == "long long int" || type == "signed long long" || type == "signed long long int")
        return (double) (model->GetValuePtr<long long>(var_name))[index];

    if (type == "unsigned long long" || type == "unsigned long long int")
        return (double) (model->GetValuePtr<unsigned long long>(var_name))[index];

    throw std::runtime_error("Unable to get value of variable " + var_name + " from " + get_model_type_name() +
                             " as double: no logic for converting variable type " + type);
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
