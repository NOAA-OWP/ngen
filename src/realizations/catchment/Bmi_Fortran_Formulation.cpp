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
    const std::string type = model->GetVarType(var_name);
    // GetValue() throws rather than handing back an empty vector, so indexing its result is safe here.
    try {
        return models::bmi::with_bmi_type(type, [&]<typename T>() -> double {
            return static_cast<double>(models::bmi::GetValue<T>(*model, var_name)[index]);
        });
    } catch (const models::bmi::unknown_bmi_type&) {
        throw std::runtime_error("Unable to get value of variable " + var_name + " from " + get_model_type_name() +
                                 " as double: no logic for converting variable type " + type);
    }
}

#endif // NGEN_WITH_BMI_FORTRAN
