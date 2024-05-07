#ifndef NGEN_BMI_PY_FORMULATION_H
#define NGEN_BMI_PY_FORMULATION_H

#include <NGenConfig.h>

#if NGEN_WITH_PYTHON

#include <memory>
#include <string>
#include "Bmi_Module_Formulation.hpp"
#include "Bmi_Py_Adapter.hpp"
#include "GenericDataProvider.hpp"
#include "pybind11/pybind11.h"
#include "pybind11/pytypes.h"
#include "pybind11/numpy.h"

// Forward declaration to provide access to protected items in testing
class Bmi_Py_Formulation_Test;

namespace realization {

    class Bmi_Py_Formulation : public Bmi_Module_Formulation {

    public:

        Bmi_Py_Formulation(std::string id, std::shared_ptr<data_access::GenericDataProvider> forcing, utils::StreamHandler output_stream);

        const std::vector<std::string> get_bmi_input_variables() override;

        const std::vector<std::string> get_bmi_output_variables() override;

        std::string get_formulation_type() override;

        /**
         * Get the model response for a time step.
         *
         * Get the model response for the provided time step, executing the backing model formulation one or more times
         * as needed.
         *
         * Function assumes the backing model has been fully initialized an that any additional input values have been
         * applied.
         *
         * The function throws an error if the index of a previously processed time step is supplied, except if it is
         * the last processed time step.  In that case, the appropriate value is returned as described below, but
         * without executing any model update.
         *
         * Assuming updating to the implied time is valid for the model, the function executes one or more model updates
         * to process future time steps for the necessary indexes.  Multiple time steps updates occur when the given
         * future time step index is not the next time step index to be processed.  Regardless, all processed time steps
         * have the size supplied in `t_delta`.
         *
         * However, it is possible to provide `t_index` and `t_delta` values that would result in the aggregate updates
         * taking the model's time beyond its `end_time` value.  In such cases, if the formulation config indicates this
         * model is not allow to exceed its set `end_time`, the function does not update the model and throws an error.
         *
         * The function will return the value of the primary output variable (see `get_bmi_main_output_var()`) for the
         * given time step after the model has been updated to that point. The type returned will always be a `double`,
         * with other numeric types being cast if necessary.
         *
         * The BMI spec requires for variable values to be passed to/from models via as arrays.  This function
         * essentially  treats the variable array reference as if it were just a raw pointer and returns the `0`-th
         * array value.
         *
         * @param t_index The index of the time step for which to run model calculations.
         * @param d_delta_s The duration, in seconds, of the time step for which to run model calculations.
         * @return The total discharge of the model for the given time step.
         */
        double get_response(time_step_t t_index, time_step_t t_delta) override;

        bool is_bmi_input_variable(const std::string &var_name) override;

        bool is_bmi_output_variable(const std::string &var_name) override;

    protected:

        std::shared_ptr<models::bmi::Bmi_Adapter> construct_model(const geojson::PropertyMap &properties) override;

        time_t convert_model_time(const double &model_time) override;

        double get_var_value_as_double(const std::string &var_name) override;

        double get_var_value_as_double(const int &index, const std::string &var_name) override;

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
        bool is_model_initialized() override;

        friend class Bmi_Multi_Formulation;

        // Unit test access
        friend class ::Bmi_Formulation_Test;
        friend class ::Bmi_Py_Formulation_Test;
        friend class ::Bmi_Multi_Formulation_Test;
    };

}

#endif //NGEN_WITH_PYTHON

#endif //NGEN_BMI_PY_FORMULATION_H
