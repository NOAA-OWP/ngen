#ifndef NGEN_BMI_FORTRAN_ADAPTER_HPP
#define NGEN_BMI_FORTRAN_ADAPTER_HPP

#include "Bmi_C_Adapter.hpp"

// Forward declaration to provide access to protected items in testing
class Bmi_Fortran_Adapter_Test;

namespace models {
    namespace bmi {
        /**
         * An adapter class to serve as a C++ interface to the essential aspects of external models written in the
         * Fortran language that implement the BMI.
         */
        class Bmi_Fortran_Adapter : public Bmi_C_Adapter {

        public:

            explicit Bmi_Fortran_Adapter(const string &type_name, std::string library_file_path,
                                         std::string forcing_file_path,
                                         bool allow_exceed_end, bool has_fixed_time_step,
                                         const std::string &registration_func, utils::StreamHandler output)
                    : Bmi_Fortran_Adapter(type_name, library_file_path, "", forcing_file_path, allow_exceed_end,
                                          has_fixed_time_step,
                                          registration_func, output) {}

            Bmi_Fortran_Adapter(const string &type_name, std::string library_file_path, std::string bmi_init_config,
                                std::string forcing_file_path, bool allow_exceed_end, bool has_fixed_time_step,
                                std::string registration_func,
                                utils::StreamHandler output) : Bmi_C_Adapter(type_name,
                                                                             library_file_path,
                                                                             bmi_init_config,
                                                                             forcing_file_path,
                                                                             allow_exceed_end,
                                                                             has_fixed_time_step,
                                                                             registration_func,
                                                                             output) {}

            void *GetValuePtr(std::string name) override {
                throw std::runtime_error(model_name + " cannot currently get pointers for Fortran-based BMI modules.");
            }

        };

    }
}

#endif //NGEN_BMI_FORTRAN_ADAPTER_HPP
