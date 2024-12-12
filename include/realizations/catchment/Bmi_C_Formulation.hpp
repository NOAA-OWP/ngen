#ifndef NGEN_BMI_C_FORMULATION_H
#define NGEN_BMI_C_FORMULATION_H

#include <memory>
#include "Bmi_Module_Formulation.hpp"
#include "Bmi_C_Adapter.hpp"
#include "GenericDataProvider.hpp"

#define BMI_C_DEFAULT_REGISTRATION_FUNC "register_bmi"

namespace realization {

    class Bmi_C_Formulation : public Bmi_Module_Formulation {

    public:

        Bmi_C_Formulation(std::string id, std::shared_ptr<data_access::GenericDataProvider> forcing_provider, utils::StreamHandler output_stream);

        std::string get_formulation_type() const override;

        bool is_bmi_input_variable(const std::string &var_name) const override;

        bool is_bmi_output_variable(const std::string &var_name) const override;

    protected:

        /**
         * Construct model and its shared pointer, potentially supplying input variable values from config.
         *
         * Construct a model (and a shared pointer to it), checking whether additional input variable values are present
         * in the configuration properties and need to be used during model construction.
         *
         * @param properties Configuration properties for the formulation, potentially containing values for input
         *                   variables
         * @return A shared pointer to a newly constructed model adapter object
         */
        std::shared_ptr<models::bmi::Bmi_Adapter> construct_model(const geojson::PropertyMap& properties) override;

        time_t convert_model_time(const double &model_time) const override {
            return (time_t) (get_bmi_model()->convert_model_time_to_seconds(model_time));
        }

        /**
         * Get a value, converted to specified type, for an output variable at a time step.
         *
         * Function gets the value for a provided output variable at a provided time step index, and returns the value
         * converted to some particular type specified by the template param.
         *
         * The function makes several assumptions:
         *
         *     1. `t_index` is a time step that has already been processed for the model
         *     2. `var_name` is in the set of valid output variable names for the model
         *     3. it is possible to either implicitly or explicitly convert (i.e., cast) the output value to the
         *        template parameter type (or it is already of that type)
         *     4. conversions do not lead to invalid (e.g., runtime errors) or unexpected (e.g., loss of precision)
         *        behavior
         *
         * It falls to users (functions) of this function to ensure these assumptions hold before invoking.
         *
         * @tparam T The type that should be returned, and to which the BMI variable value should be cast.
         * @param t_index The index of some already-processed time step for which a variable value is being requested
         * @param var_name The name of the variable for which a value is being requested
         * @return
         */
        template<class T, class O>
        T get_var_value_as(time_step_t t_index, const std::string& var_name) {
            std::vector<O> outputs = models::bmi::GetValue<O>(*get_bmi_model(), var_name);
            return (T) outputs[t_index];
        }

        /**
         * Get value for some BMI model variable at a specific index.
         *
         * Function gets the value for a provided variable, returned from the backing model as an array, and returns the
         * specific value at the desired index cast as a double type.
         *
         * The function makes several assumptions:
         *
         *     1. `index` is within array bounds
         *     2. `var_name` is in the set of valid variable names for the model
         *     3. the type for output variable allows the value to be cast to a `double` appropriately
         *
         * It falls to user (functions) of this function to ensure these assumptions hold before invoking.
         *
         * @param index
         * @param var_name
         * @return
         */
        double get_var_value_as_double(const int& index, const std::string& var_name) override;

        /**
         * Test whether backing model has run BMI ``Initialize``.
         *
         * Test whether the backing model object has been initialize using the BMI standard ``Initialize`` function.
         *
         * This overrides the super class implementation and checks the model directly.  As such, the associated setter
         * does not serve any purpose.
         *
         * @return Whether backing model object has been initialize using the BMI standard ``Initialize`` function.
         */
        bool is_model_initialized() const override;

        // Unit test access
        friend class ::Bmi_Formulation_Test;
        friend class ::Bmi_C_Formulation_Test;
        friend class ::Bmi_C_Pet_IT;
    };

}

#endif //NGEN_BMI_C_FORMULATION_H
