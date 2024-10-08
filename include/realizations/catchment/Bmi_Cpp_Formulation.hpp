#ifndef NGEN_BMI_CPP_FORMULATION_H
#define NGEN_BMI_CPP_FORMULATION_H

#include "Bmi_Module_Formulation.hpp"
#include "Bmi_Cpp_Adapter.hpp"

namespace realization {

    /**
     * @brief Encapsulates a bmi::Bmi model in C++ such that it can be loaded as a formulation, as such also so that it can be dynamically loaded from a shared library file.
     * 
     */
    class Bmi_Cpp_Formulation : public Bmi_Module_Formulation {

    public:

        Bmi_Cpp_Formulation(std::string id, std::shared_ptr<data_access::GenericDataProvider> forcing_provider, utils::StreamHandler output_stream);

        std::string get_formulation_type() const override;

        bool is_bmi_input_variable(const std::string &var_name) const override;

        bool is_bmi_output_variable(const std::string &var_name) const override;

    protected:

        std::shared_ptr<models::bmi::Bmi_Adapter> construct_model(const geojson::PropertyMap& properties) override;

        time_t convert_model_time(const double &model_time) const override {
            return (time_t) (get_bmi_model()->convert_model_time_to_seconds(model_time));
        }

        template<class T, class O>
        T get_var_value_as(time_step_t t_index, const std::string& var_name) {
            std::vector<O> outputs = models::bmi::GetValue<O>(*get_bmi_model(), var_name);
            return (T) outputs[t_index];
        }

        double get_var_value_as_double(const int& index, const std::string& var_name) override;

        bool is_model_initialized() const override;

        // Unit test access
        friend class ::Bmi_Formulation_Test;
        friend class ::Bmi_Cpp_Formulation_Test;
    };

}

#endif //NGEN_BMI_CPP_FORMULATION_H
