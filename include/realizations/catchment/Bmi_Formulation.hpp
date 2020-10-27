#ifndef NGEN_BMI_FORMULATION_H
#define NGEN_BMI_FORMULATION_H

#include <utility>

#include "Catchment_Formulation.hpp"

namespace realization {

    /**
     * Abstraction of a formulation with a backing model object that implements the BMI.
     *
     * @tparam M The type for the backing BMI model object.
     */
    template <class M>
    class Bmi_Formulation : public Catchment_Formulation {

    public:

        /**
         * Minimal constructor for objects initialize using the Formulation_Manager and subsequent calls to
         * ``create_formulation``.
         *
         * @param id
         * @param forcing_config
         * @param output_stream
         */
        Bmi_Formulation(std::string id, forcing_params forcing_config, utils::StreamHandler output_stream)
                : Catchment_Formulation(std::move(id), std::move(forcing_config), output_stream) { };

        virtual ~Bmi_Formulation() {};

    protected:

        /**
         * Get the backing model object implementing the BMI.
         *
         * @return Shared pointer to the backing model object that implements the BMI.
         */
        std::shared_ptr<M> get_bmi_model() {
            return bmi_model;
        }

        /**
         * Get the name of the specific type of the backing model object.
         *
         * @return The name of the backing model object's type.
         */
        std::string get_model_type_name() {
            return model_type_name;
        };

        /**
         * Test whether the backing model object has been initialize using the BMI standard ``Initialize`` function.
         *
         * @return Whether backing model object has been initialize using the BMI standard ``Initialize`` function.
         */
        bool is_model_initialized() const {
            return model_initialized;
        };

        /**
         * Set the backing model object implementing the BMI.
         *
         * @param model Shared pointer to the BMI model.
         */
        void set_bmi_model(std::shared_ptr<M> model) {
            bmi_model = model;
        }

        /**
         * Set whether the backing model object has been initialize using the BMI standard ``Initialize`` function.
         *
         * @param is_initialized Whether model object has been initialize using the BMI standard ``Initialize``.
         */
        void set_model_initialized(bool is_initialized) {
            model_initialized = is_initialized;
        }

        /**
         * Set the name of the specific type of the backing model object.
         *
         * @param type_name The name of the backing model object's type.
         */
        void set_model_type_name(std::string type_name) {
            model_type_name = std::move(type_name);
        }

    private:
        std::shared_ptr<M> bmi_model;
        bool model_initialized = false;
        std::string model_type_name;

    };

}

#endif //NGEN_BMI_FORMULATION_H
