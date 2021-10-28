#ifndef NGEN_BMI_FORTRAN_FORMULATION_HPP
#define NGEN_BMI_FORTRAN_FORMULATION_HPP

#ifdef NGEN_BMI_FORTRAN_ACTIVE

#include "Bmi_Module_Formulation.hpp"
#include "Bmi_Fortran_Adapter.hpp"
#include <ForcingProvider.hpp>

// TODO: consider merging this somewhere with the C value in that formulation header
#define BMI_FORTRAN_DEFAULT_REGISTRATION_FUNC "register_bmi"

class Bmi_Fortran_Formulation_Test;

using namespace models::bmi;

namespace realization {

    class Bmi_Fortran_Formulation : public Bmi_Module_Formulation<Bmi_Fortran_Adapter> {

    public:

        Bmi_Fortran_Formulation(std::string id, std::unique_ptr<ForcingProvider> forcing, utils::StreamHandler output_stream);

        std::string get_formulation_type() override;

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
         * Implementations will throw `invalid_argument` exceptions if data for the provided time step parameter is not
         * accessible.  Note that, for this type, only the last processed time step is accessible, because formulations
         * do not save results from previous time steps.  This also has the consequence of there being no valid set of
         * arguments before a least one call to @ref get_response has been made.
         *
         * @param timestep The time step for which data is desired.
         * @return A delimited string with all the output variable values for the given time step.
         */
        string get_output_line_for_timestep(int timestep, std::string delimiter) override;

        /**
         * Get the model response for a time step.
         *
         * Get the model response for the provided time step, executing the backing model formulation one or more times as
         * needed.
         *
         * Function assumes the backing model has been fully initialized an that any additional input values have been applied.
         *
         * The function throws an error if the index of a previously processed time step is supplied, except if it is the last
         * processed time step.  In that case, the appropriate value is returned as described below, but without executing any
         * model update.
         *
         * Assuming updating to the implied time is valid for the model, the function executes one or more model updates to
         * process future time steps for the necessary indexes.  Multiple time steps updates occur when the given future time
         * step index is not the next time step index to be processed.  Regardless, all processed time steps have the size
         * supplied in `t_delta`.
         *
         * However, it is possible to provide `t_index` and `t_delta` values that would result in the aggregate updates taking
         * the model's time beyond its `end_time` value.  In such cases, if the formulation config indicates this model is
         * not allow to exceed its set `end_time`, the function does not update the model and throws an error.
         *
         * The function will return the value of the primary output variable (see `get_bmi_main_output_var()`) for the given
         * time step after the model has been updated to that point. The type returned will always be a `double`, with other
         * numeric types being cast if necessary.
         *
         * The BMI spec requires for variable values to be passed to/from models via as arrays.  This function essentially
         * treats the variable array reference as if it were just a raw pointer and returns the `0`-th array value.
         *
         * @param t_index The index of the time step for which to run model calculations.
         * @param d_delta_s The duration, in seconds, of the time step for which to run model calculations.
         * @return The total discharge of the model for the given time step.
         */
        double get_response(::time_step_t index, ::time_step_t t_delta) override;

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
        std::shared_ptr<Bmi_Fortran_Adapter> construct_model(const geojson::PropertyMap& properties) override;

        time_t convert_model_time(const double &model_time) override {
            return (time_t) (get_bmi_model()->convert_model_time_to_seconds(model_time));
        }

        double get_var_value_as_double(const string &var_name) override;

        double get_var_value_as_double(const int &index, const string &var_name) override;

        friend class Bmi_Multi_Formulation;

        // Unit test access
        friend class ::Bmi_Formulation_Test;
        friend class ::Bmi_Fortran_Formulation_Test;

    private:
        /**
         * Index value (0-based) of the time step that will be processed by the next update of the model.
         *
         * A formulation time step for BMI types can be thought of as the execution of a call to any of the functions of
         * the underlying BMI model that advance the model (either `update` or `update_until`). This member stores the
         * ordinal index of the next time step to be executed.  Except in the initial formulation state, this will be
         * one greater than the index of the last executed time step.
         *
         * E.g., on initialization, before any calls to @ref get_response, this value will be ``0``.  After a call to
         * @ref get_response (assuming ``0`` as the passed ``t_index`` argument), time step ``0`` will be processed, and
         * this member would be incremented by 1, thus making it ``1``.
         *
         * The member serves as an implicit marker of how many time steps have been processed so far.  Knowing this is
         * required to maintain valid behavior in certain things, such as @ref get_response (we may want to process
         * multiple time steps forward to a particular index other than the next, but it would not be valid to receive
         * a ``t_index`` earlier than the last processed time step) and @ref get_output_line_for_timestep (because
         * formulations do not save results from previous time steps, only the results from the last processed time step
         * can be used to generate output).
         */
        int next_time_step_index = 0;
    };

}

#endif // NGEN_BMI_FORTRAN_ACTIVE

#endif //NGEN_BMI_FORTRAN_FORMULATION_HPP
