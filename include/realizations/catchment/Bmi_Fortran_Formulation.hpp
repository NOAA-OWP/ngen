#ifndef NGEN_BMI_FORTRAN_FORMULATION_HPP
#define NGEN_BMI_FORTRAN_FORMULATION_HPP

#include <NGenConfig.h>

#if NGEN_WITH_BMI_FORTRAN

#include "Bmi_Module_Formulation.hpp"
#include <GenericDataProvider.hpp>

// TODO: consider merging this somewhere with the C value in that formulation header
#define BMI_FORTRAN_DEFAULT_REGISTRATION_FUNC "register_bmi"

class Bmi_Fortran_Formulation_Test;

namespace realization {

    class Bmi_Fortran_Formulation : public Bmi_Module_Formulation {

    public:

        Bmi_Fortran_Formulation(std::string id, std::shared_ptr<data_access::GenericDataProvider> forcing, utils::StreamHandler output_stream);

        std::string get_formulation_type() const override;

    protected:

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
        std::shared_ptr<models::bmi::Bmi_Adapter> construct_model(const geojson::PropertyMap& properties) override;

        time_t convert_model_time(const double &model_time) const override {
            return (time_t) (get_bmi_model()->convert_model_time_to_seconds(model_time));
        }

        double get_var_value_as_double(const int &index, const std::string &var_name) override;

        // Unit test access
        friend class ::Bmi_Multi_Formulation_Test;
        friend class ::Bmi_Formulation_Test;
        friend class ::Bmi_Fortran_Formulation_Test;
    };

}

#endif // NGEN_WITH_BMI_FORTRAN

#endif //NGEN_BMI_FORTRAN_FORMULATION_HPP
