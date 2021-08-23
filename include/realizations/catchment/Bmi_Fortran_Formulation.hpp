#ifndef NGEN_BMI_FORTRAN_FORMULATION_HPP
#define NGEN_BMI_FORTRAN_FORMULATION_HPP

#include "Bmi_C_Formulation.hpp"
#include "Bmi_Fortran_Adapter.hpp"

namespace realization {

    class Bmi_Fortran_Formulation : public Bmi_C_Formulation {

    public:

        Bmi_Fortran_Formulation(std::string id, Forcing forcing, utils::StreamHandler output_stream);

        Bmi_Fortran_Formulation(std::string id, forcing_params forcing_config, utils::StreamHandler output_stream);

        std::string get_formulation_type() override;

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
        std::shared_ptr<models::bmi::Bmi_C_Adapter> construct_model(const geojson::PropertyMap& properties) override;
    };

}

#endif //NGEN_BMI_FORTRAN_FORMULATION_HPP
