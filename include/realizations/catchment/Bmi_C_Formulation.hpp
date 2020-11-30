#ifndef NGEN_BMI_C_FORMULATION_H
#define NGEN_BMI_C_FORMULATION_H

#include <memory>
#include "Bmi_Formulation.hpp"
#include "Bmi_C_Adapter.hpp"

namespace realization {

    class Bmi_C_Formulation : public Bmi_Formulation<models::bmi::Bmi_C_Adapter> {

    public:

        Bmi_C_Formulation(std::string id, forcing_params forcing_config, utils::StreamHandler output_stream);

        void create_formulation(boost::property_tree::ptree &config, geojson::PropertyMap *global = nullptr) override;

        void create_formulation(geojson::PropertyMap properties) override;

        std::string get_formulation_type() override;

        /**
         * Get a header line appropriate for a file made up of entries from this type's implementation of
         * ``get_output_line_for_timestep``.
         *
         * Note that like the output generating function, this line does not include anything for time step.
         *
         * @return An appropriate header line for this type.
         */
        std::string get_output_header_line(std::string delimiter) override;

        /**
         * Get a delimited string with all the output variable values for the given time step.
         *
         * This method is useful for preparing calculated data in a representation useful for output files, such as
         * CSV files.
         *
         * The resulting string contains only the calculated output values for the time step, and not the time step
         * index itself.
         *
         * An empty string is returned if the time step value is not in the range of valid time steps for which there
         * are calculated values for all variables.
         *
         * The default delimiter is a comma.
         *
         * @param timestep The time step for which data is desired.
         * @return A delimited string with all the output variable values for the given time step.
         */
        std::string get_output_line_for_timestep(int timestep, std::string delimiter) override;

        /**
         * Get the model response for this time step.
         *
         * Get the model response for this time step, execute the backing model formulation one or more times if the
         * time step of the given index has not already been processed.
         *
         * Function assumes the backing model has been fully initialized an that any additional input values have been
         * applied.
         *
         * The function will return the value of the primary output variable (see `get_bmi_main_output_var()`) for the
         * given time step. The type returned will always be a `double`, with other numeric types being cast if
         * necessary.
         *
         * Because of the nature of BMI, the `t_delta` parameter is ignored, as this cannot be passed meaningfully via
         * the `update()` BMI function.
         *
         * @param t_index The index of the time step for which to run model calculations.
         * @param d_delta_s The duration, in seconds, of the time step for which to run model calculations.
         * @return The total discharge of the model for the given time step.
         */
        double get_response(time_step_t t_index, time_step_t t_delta) override;

    protected:

        /**
         * Get value for some output variable at some time step, cast as a double.
         *
         * Function gets the value for a provided output variable at a provided time step index, and returns the value
         * cast as a double type.
         *
         * The function makes several assumptions:
         *
         *     1. `t_index` is a time step that has already been processed for the model
         *     2. `var_name` is in the set of valid output variable names for the model
         *     3. the type for output variable ``var_name`` is `double`, `float`, `int`, or `long`
         *
         * It falls to user (functions) of this function to ensure these assumptions hold before invoking.
         *
         * @param t_index
         * @param var_name
         * @return
         */
        double get_var_value_as_double(time_step_t t_index, const std::string& var_name);

        void inner_create_formulation(geojson::PropertyMap properties, bool needs_param_validation);

        /**
         * Test whether the backing model object has been initialize using the BMI standard ``Initialize`` function.
         *
         * This overrides the super class implementation and checks the model directly.  As such, the associated setter
         * does not serve any purpose.
         *
         * @return Whether backing model object has been initialize using the BMI standard ``Initialize`` function.
         */
        bool is_model_initialized() override;

    };

}

#endif //NGEN_BMI_C_FORMULATION_H
