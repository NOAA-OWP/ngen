#include <NGenConfig.h>

#if NGEN_WITH_BMI_FORTRAN

#include "Bmi_Fortran_Formulation.hpp"
#include "Bmi_Fortran_Adapter.hpp"
#include "Constants.h"

using namespace realization;
using namespace models::bmi;

Bmi_Fortran_Formulation::Bmi_Fortran_Formulation(std::string id, std::shared_ptr<data_access::GenericDataProvider> forcing, utils::StreamHandler output_stream)
: Bmi_Module_Formulation(id, forcing, output_stream) { }

/**
 * Construct model and its shared pointer.
 *
 * Construct a backing BMI model/module along with a shared pointer to it, with the latter being returned.
 *
 * This implementation is very much like that of the superclass, except that the pointer is actually to a
 * @see Bmi_Fortran_Adapter object, a type which extends @see Bmi_C_Adapter.  This is to support the necessary subtle
 * differences in behavior, though the two are largely the same.
 *
 * @param properties Configuration properties for the formulation.
 * @return A shared pointer to a newly constructed model adapter object.
 */
std::shared_ptr<Bmi_Adapter> Bmi_Fortran_Formulation::construct_model(const geojson::PropertyMap& properties) {
    auto library_file_iter = properties.find(BMI_REALIZATION_CFG_PARAM_OPT__LIB_FILE);
    if (library_file_iter == properties.end()) {
        throw std::runtime_error("BMI C formulation requires path to library file, but none provided in config");
    }
    std::string lib_file = library_file_iter->second.as_string();
    auto reg_func_itr = properties.find(BMI_REALIZATION_CFG_PARAM_OPT__REGISTRATION_FUNC);
    std::string reg_func =
            reg_func_itr == properties.end() ? BMI_FORTRAN_DEFAULT_REGISTRATION_FUNC : reg_func_itr->second.as_string();
    return std::make_shared<Bmi_Fortran_Adapter>(
            get_model_type_name(),
            lib_file,
            get_bmi_init_config(),
            is_bmi_model_time_step_fixed(),
            reg_func);
}

std::string Bmi_Fortran_Formulation::get_formulation_type() const {
    return "bmi_fortran";
}

double Bmi_Fortran_Formulation::get_var_value_as_double(const int &index, const std::string &var_name) {
    auto model = std::dynamic_pointer_cast<models::bmi::Bmi_Fortran_Adapter>(get_bmi_model());

    // TODO: consider different way of handling (and how to document) cases like long double or unsigned long long that
    //  don't fit or might convert inappropriately
    std::string type = model->GetVarType(var_name);
    //Can cause a segfault here if GetValue returns an empty vector...a "fix" in bmi_utilities GetValue
    //will throw a relevant runtime_error if the vector is empty, so this is safe to use this way for now...
    if (type == "long double")
        return (double) (models::bmi::GetValue<long double>(*model, var_name))[index];

    if (type == "double" || type == "double precision")
        return (double) (models::bmi::GetValue<double>(*model, var_name))[index];

    if (type == "float" || type == "real")
        return (double) (models::bmi::GetValue<float>(*model, var_name))[index];

    if (type == "short" || type == "short int" || type == "signed short" || type == "signed short int")
        return (double) (models::bmi::GetValue<short>(*model, var_name))[index];

    if (type == "unsigned short" || type == "unsigned short int")
        return (double) (models::bmi::GetValue<unsigned short>(*model, var_name))[index];

    if (type == "int" || type == "signed" || type == "signed int" || type == "integer")
        return (double) (models::bmi::GetValue<int>(*model, var_name))[index];

    if (type == "unsigned" || type == "unsigned int")
        return (double) (models::bmi::GetValue<unsigned int>(*model, var_name))[index];

    if (type == "long" || type == "long int" || type == "signed long" || type == "signed long int")
        return (double) (models::bmi::GetValue<long>(*model, var_name))[index];

    if (type == "unsigned long" || type == "unsigned long int")
        return (double) (models::bmi::GetValue<unsigned long>(*model, var_name))[index];

    if (type == "long long" || type == "long long int" || type == "signed long long" || type == "signed long long int")
        return (double) (models::bmi::GetValue<long long>(*model, var_name))[index];

    if (type == "unsigned long long" || type == "unsigned long long int")
        return (double) (models::bmi::GetValue<unsigned long long>(*model, var_name))[index];

    throw std::runtime_error("Unable to get value of variable " + var_name + " from " + get_model_type_name() +
    " as double: no logic for converting variable type " + type);
}

#endif // NGEN_WITH_BMI_FORTRAN
