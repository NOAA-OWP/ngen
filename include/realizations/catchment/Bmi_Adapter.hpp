#ifndef NGEN_BMI_ADAPTER_HPP
#define NGEN_BMI_ADAPTER_HPP

#include <string>
#include <vector>
#include "bmi.hpp"
#include "FileChecker.h"
#include "JSONProperty.hpp"
#include "StreamHandler.hpp"

using namespace std;

namespace models {
    namespace bmi {
        /**
         * Abstract adapter interface for C++ classes to interact with the essential aspects of external models that
         * implement the BMI spec but that are written in some other programming language.
         */
        template <class T>
        class Bmi_Adapter : public ::bmi::Bmi {
        public:
            Bmi_Adapter(string model_name, string bmi_init_config, string forcing_file_path, bool allow_exceed_end,
                        bool has_fixed_time_step, const geojson::JSONProperty &other_input_vars,
                        utils::StreamHandler output, bool call_bmi_initialize = true)
                : model_name(move(model_name)),
                  bmi_init_config(move(bmi_init_config)),
                  bmi_model_uses_forcing_file(!forcing_file_path.empty()),
                  forcing_file_path(move(forcing_file_path)),
                  bmi_model_has_fixed_time_step(has_fixed_time_step),
                  allow_model_exceed_end_time(allow_exceed_end),
                  output(move(output)),
                  bmi_model_time_convert_factor(1.0)
            {
                if (call_bmi_initialize)
                    Initialize();
            }

            /**
             * Determine backing model's time units and set the reference parameter to an appropriate conversion factor.
             *
             * A backing BMI model may use arbitrary units for time, but it will expose what those units are via the
             * BMI ``GetTimeUnits`` function.  This function retrieves (and interprets) its model's units and
             * sets the given reference parameter to an appropriate factor for converting its internal time values to
             * equivalent representations within the model, and vice versa.
             *
             * @param time_convert_factor A reference to set to the determined conversion factor.
             */
            virtual void acquire_time_conversion_factor(double &time_convert_factor) {
                string time_units = GetTimeUnits();
                if (time_units == "s" || time_units == "sec" || time_units == "second" || time_units == "seconds")
                    time_convert_factor = 1.0;
                else if (time_units == "m" || time_units == "min" || time_units == "minute" ||
                         time_units == "minutes")
                    time_convert_factor = 60.0;
                else if (time_units == "h" || time_units == "hr" || time_units == "hour" || time_units == "hours")
                    time_convert_factor = 3600.0;
                else if (time_units == "d" || time_units == "day" || time_units == "days")
                    time_convert_factor = 86400.0;
                else
                    throw runtime_error(
                            "Invalid model time step units ('" + time_units + "') in " + model_name + ".");
            }

            /**
             * Initialize the wrapped BMI model functionality using the value from the `bmi_init_config` member variable
             * and the API's ``Initialize`` function.
             *
             * If no attempt to initialize the model has yet been made (i.e., ``model_initialized`` is ``false`` when
             * this function is called), then an initialization is attempted for the model. This is handled by a nested
             * call to the ``construct_and_init_backing_model`` function. If initialization fails, an exception will be
             * raised, with it's message saved as part of this object's state.  However, regardless of the success of
             * initialization, ``model_initialized`` is set to ``true`` after the attempt.
             *
             * Additionally, if a successful model initialization is performed, ``bmi_model_time_convert_factor``
             * is immediately thereafter set by passing it by reference in a call to ``acquire_time_conversion_factor``.
             *
             * If an attempt to initialize the model has already been made (i.e., ``model_initialized`` is ``true``),
             * this function will either simply return or will throw a runtime_error, with its message including the
             * message of the exception from the earlier attempt.
             *
             * @throws runtime_error  If called again after earlier call that resulted in an exception, or if BMI
             *                             config file could not be read.
             * @throws models::external::State_Exception   If `initialize()` in nested model does not return successful.
             */
            void Initialize() {
                // If there was previous init attempt but w/ failure exception, throw runtime error and include previous
                // message
                if (model_initialized && !init_exception_msg.empty()) {
                    throw runtime_error(
                            "Previous " + model_name + " init attempt had exception: \n\t" + init_exception_msg);
                }
                    // If there was previous init attempt w/ (implicitly) no exception on previous attempt, just return
                else if (model_initialized) {
                    return;
                }
                else if (!utils::FileChecker::file_is_readable(bmi_init_config)) {
                    init_exception_msg = "Cannot initialize " + model_name + " using unreadable file '"
                            + bmi_init_config + "'";
                    throw runtime_error(init_exception_msg);
                }
                else {
                    try {
                        // TODO: make this same name as used with other testing (adjust name in docstring above also)
                        construct_and_init_backing_model();
                        // Make sure this is set to 'true' after this function call finishes
                        model_initialized = true;
                        acquire_time_conversion_factor(bmi_model_time_convert_factor);
                    }
                        // Record the exception message before re-throwing to handle subsequent function calls properly
                    catch (exception& e) {
                        // Make sure this is set to 'true' after this function call finishes
                        model_initialized = true;
                        throw e;
                    }
                }
            }

            /**
             * Initialize the wrapped BMI model object using the given config file and the object's ``Initialize``
             * function.
             *
             * If the given file is not the same as what is in `bmi_init_config`` and the model object has not already
             * been initialized, this function will produce a warning message about the difference, then subsequently
             * update `bmi_init_config`` to the given file.  It will then proceed with initialization.
             *
             * However, if the given file is not the same as what is in `bmi_init_config``, but the model has already
             * been initialized, a runtime_error exception is thrown.
             *
             * This otherwise operates using the logic of ``Initialize()``.
             *
             * @param config_file
             * @see Initialize()
             * @throws runtime_error If already initialized but using a different file than the passed argument.
             */
            void Initialize(string config_file) override {
                if (config_file != bmi_init_config && model_initialized) {
                    throw runtime_error(
                            "Model init previously attempted; cannot change config from " + bmi_init_config + " to "
                            + config_file);
                }

                if (config_file != bmi_init_config && !model_initialized) {
                    output.put("Warning: initialization call changes model config from " + bmi_init_config + " to "
                                + config_file);
                    bmi_init_config = config_file;
                }

                Initialize();
            }

        protected:
            /** Whether model ``Update`` calls are allowed and handled in some way by the backing model. */
            bool allow_model_exceed_end_time = false;
            /** Path (as a string) to the BMI config file for initializing the backing model (empty if none). */
            string bmi_init_config;
            /** Pointer to backing BMI model instance. */
            shared_ptr<T> bmi_model = nullptr;
            /** Whether this particular model has a time step size that cannot be changed internally or externally. */
            bool bmi_model_has_fixed_time_step = true;
            /** Conversion factor for converting values for model time in model's unit type to equivalent in seconds. */
            double bmi_model_time_convert_factor;
            /** Pointer to stored time step size value of backing model, if it is fixed and has been retrieved. */
            shared_ptr<double> bmi_model_time_step_size = nullptr;
            /** Whether this particular model implementation directly reads input data from the forcing file. */
            bool bmi_model_uses_forcing_file;
            string forcing_file_path;
            /** Message from an exception (if encountered) on the first attempt to initialize the backing model. */
            string init_exception_msg;
            /** Pointer to collection of input variable names for backing model, used by ``GetInputVarNames()``. */
            shared_ptr<vector<string>> input_var_names;
            /** Whether the backing model has been initialized yet, which is always initially ``false``. */
            bool model_initialized = false;
            string model_name;
            utils::StreamHandler output;
            /** Pointer to collection of output variable names for backing model, used by ``GetOutputVarNames()``. */
            shared_ptr<vector<string>> output_var_names;

            /**
             * Construct the backing BMI model object, then call its BMI-native ``Initialize()`` function.
             *
             * Implementations should return immediately without taking any further action if ``model_initialized`` is
             * already ``true``.
             *
             * The call to the BMI native ``Initialize(string)`` should pass the value stored in ``bmi_init_config``.
             */
            virtual void construct_and_init_backing_model() = 0;

        };
    }
}
#endif //NGEN_BMI_ADAPTER_HPP
