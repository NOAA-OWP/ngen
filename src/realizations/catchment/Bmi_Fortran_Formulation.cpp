#include "Bmi_Fortran_Formulation.hpp"
#include "Constants.h"

using namespace realization;
using namespace models::bmi;

Bmi_Fortran_Formulation::Bmi_Fortran_Formulation(std::string id, Forcing forcing, utils::StreamHandler output_stream)
: Bmi_C_Formulation(id, forcing, output_stream) { }

Bmi_Fortran_Formulation::Bmi_Fortran_Formulation(std::string id, forcing_params forcing_config, utils::StreamHandler output_stream)
: Bmi_C_Formulation(id, forcing_config, output_stream) { }

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
std::shared_ptr<Bmi_C_Adapter> Bmi_Fortran_Formulation::construct_model(const geojson::PropertyMap& properties) {
    auto library_file_iter = properties.find(BMI_REALIZATION_CFG_PARAM_OPT__LIB_FILE);
    if (library_file_iter == properties.end()) {
        throw std::runtime_error("BMI C formulation requires path to library file, but none provided in config");
    }
    std::string lib_file = library_file_iter->second.as_string();
    auto reg_func_itr = properties.find(BMI_REALIZATION_CFG_PARAM_OPT__REGISTRATION_FUNC);
    std::string reg_func =
            reg_func_itr == properties.end() ? BMI_C_DEFAULT_REGISTRATION_FUNC : reg_func_itr->second.as_string();
    return std::make_shared<Bmi_Fortran_Adapter>(
            Bmi_Fortran_Adapter(
                    get_model_type_name(),
                    lib_file,
                    get_bmi_init_config(),
                    (is_bmi_using_forcing_file() ? get_forcing_file_path() : ""),
                    get_allow_model_exceed_end_time(),
                    is_bmi_model_time_step_fixed(),
                    reg_func,
                    output));
}

std::string Bmi_Fortran_Formulation::get_formulation_type() {
    return "bmi_fortran";
}