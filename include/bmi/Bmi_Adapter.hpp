#ifndef NGEN_BMI_ADAPTER_HPP
#define NGEN_BMI_ADAPTER_HPP

#include <string>
#include <vector>
#include "bmi.hpp"
#include "FileChecker.h"
#include "JSONProperty.hpp"
#include "State_Exception.hpp"
#include "StreamHandler.hpp"
#include <UnitsHelper.hpp>

namespace models {
    namespace bmi {
        /**
         * Abstract adapter interface for C++ classes to interact with the essential aspects of external models that
         * implement the BMI spec but that are written in some other programming language.
         */
        class Bmi_Adapter : public ::bmi::Bmi {
        public:

            Bmi_Adapter(std::string model_name, std::string bmi_init_config, std::string forcing_file_path, bool allow_exceed_end,
                        bool has_fixed_time_step, utils::StreamHandler output)
                : model_name(std::move(model_name)),
                  bmi_init_config(std::move(bmi_init_config)),
                  bmi_model_uses_forcing_file(!forcing_file_path.empty()),
                  forcing_file_path(std::move(forcing_file_path)),
                  bmi_model_has_fixed_time_step(has_fixed_time_step),
                  allow_model_exceed_end_time(allow_exceed_end),
                  output(std::move(output)),
                  bmi_model_time_convert_factor(1.0)
            {
                // This replicates a lot of Initialize, but it's necessary to be able to do it separately to support
                // "initializing" on construction, given using Initialize requires use of virtual functions
                if (!utils::FileChecker::file_is_readable(this->bmi_init_config)) {
                    init_exception_msg = "Cannot create and initialize " + this->model_name + " using unreadable file '"
                            + this->bmi_init_config + "'";
                    throw std::runtime_error(init_exception_msg);
                }
            }

            Bmi_Adapter(Bmi_Adapter const&) = default;
            Bmi_Adapter(Bmi_Adapter &&) = default;

            /**
             * Determine backing model's time units and return an appropriate conversion factor.
             *
             * A backing BMI model may use arbitrary units for time, but it will expose what those units are via the
             * BMI ``GetTimeUnits`` function. This function retrieves (and interprets) its model's units and
             * return an appropriate factor for converting its internal time values to equivalent representations
             * within the model, and vice versa. This function complies with the BMI get_time_units standard
             */
            double get_time_convert_factor() {
                double value = 1.0;
                std::string input_units = GetTimeUnits();
                std::string output_units = "s";
                return UnitsHelper::get_converted_value(input_units, value, output_units);
            }

            /**
             * Convert model time value to value in seconds.
             *
             * Performs necessary lookup and conversion of some given model time value - as from `GetCurrentTime()` or
             * `GetStartTime()` - and returns the equivalent value when represented in seconds.
             *
             * @param model_time_val Arbitrary model time value in units provided by `GetTimeUnits()`
             * @return Equivalent model time value to value in seconds.
             */
            double convert_model_time_to_seconds(const double& model_time_val) {
                return model_time_val * bmi_model_time_convert_factor;
            }

            /**
             * Convert a given number of seconds to equivalent in model time units.
             *
             * @param seconds_val
             * @return
             */
            double convert_seconds_to_model_time(const double& seconds_val) {
                return seconds_val / bmi_model_time_convert_factor;
            }

            /**
             * Get the name string for the C++ type analogous to the described type in the backing model.
             *
             * The supported languages for BMI modules support different types than C++ in general, but it is necessary
             * to map "external" types to the appropriate C++ type for certain interactions; e.g., setting a variable
             * value.  This function encapsulates the translation specific to the particular language.
             *
             * Note that the size of an individual item is also required, as this can vary in certain situations
             * depending on the implementation language of a backing model.
             *
             * Implementations should all throw the same type of exception (currently, @ref std::runtime_error) if/when
             * an unrecognized external type name parameter is provided.
             *
             * @param external_type_name The string name of some type in backing model's implementation language.
             * @param item_size The particular size in bytes for items of the involved analogous types.
             * @return The name string for the C++ type analogous to the described type in the backing model.
             * @throw std::runtime_error If an unrecognized external type name parameter is provided.
             */
            virtual const std::string get_analogous_cxx_type(const std::string &external_type_name,
                                                             const size_t item_size) = 0;

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
                    throw std::runtime_error(
                            "Previous " + model_name + " init attempt had exception: \n\t" + init_exception_msg);
                }
                    // If there was previous init attempt w/ (implicitly) no exception on previous attempt, just return
                else if (model_initialized) {
                    return;
                }
                else if (!utils::FileChecker::file_is_readable(bmi_init_config)) {
                    init_exception_msg = "Cannot initialize " + model_name + " using unreadable file '"
                            + bmi_init_config + "'";
                    throw std::runtime_error(init_exception_msg);
                }
                else {
                    try {
                        // TODO: make this same name as used with other testing (adjust name in docstring above also)
                        construct_and_init_backing_model();
                        // Make sure this is set to 'true' after this function call finishes
                        model_initialized = true;
                        bmi_model_time_convert_factor = get_time_convert_factor();
                    }
                        // Record the exception message before re-throwing to handle subsequent function calls properly
                    catch (std::exception& e) {
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
             * @throws models::external::State_Exception   If `initialize()` in nested model is not successful.
             * @throws runtime_error If already initialized but using a different file than the passed argument.
             */
            void Initialize(std::string config_file) override {
                if (config_file != bmi_init_config && model_initialized) {
                    throw std::runtime_error(
                            "Model init previously attempted; cannot change config from " + bmi_init_config + " to "
                            + config_file);
                }

                if (config_file != bmi_init_config && !model_initialized) {
                    output.put("Warning: initialization call changes model config from " + bmi_init_config + " to "
                                + config_file);
                    bmi_init_config = config_file;
                }
                try {
                    Initialize();
                }
                catch (models::external::State_Exception &e) {
                    throw e;
                }
                catch (std::exception &e) {
                    throw std::runtime_error(e.what());
                }
            }

            /**
             * Get whether this instance has been initialized properly.
             *
             * @return Whether this instance has been initialized properly.
             */
            inline bool isInitialized() {
                return model_initialized;
            }

            /**
             * @brief Get the model name.
             * 
             * @return The name of the model connected to the adapter.
             * 
             */
            inline std::string get_model_name(){
                return model_name;
            }

        protected:
            /** Whether model ``Update`` calls are allowed and handled in some way by the backing model. */
            bool allow_model_exceed_end_time = false;
            /** Path (as a string) to the BMI config file for initializing the backing model (empty if none). */
            std::string bmi_init_config;
            /** Whether this particular model has a time step size that cannot be changed internally or externally. */
            bool bmi_model_has_fixed_time_step = true;
            /** Conversion factor for converting values for model time in model's unit type to equivalent in seconds. */
            double bmi_model_time_convert_factor;
            /** Pointer to stored time step size value of backing model, if it is fixed and has been retrieved. */
            std::shared_ptr<double> bmi_model_time_step_size = nullptr;
            /** Whether this particular model implementation directly reads input data from the forcing file. */
            bool bmi_model_uses_forcing_file;
            std::string forcing_file_path;
            /** Message from an exception (if encountered) on the first attempt to initialize the backing model. */
            std::string init_exception_msg;
            /** Pointer to collection of input variable names for backing model, used by ``GetInputVarNames()``. */
            std::shared_ptr<std::vector<std::string>> input_var_names;
            /** Whether the backing model has been initialized yet, which is always initially ``false``. */
            bool model_initialized = false;
            std::string model_name;
            utils::StreamHandler output;
            /** Pointer to collection of output variable names for backing model, used by ``GetOutputVarNames()``. */
            std::shared_ptr<std::vector<std::string>> output_var_names;

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
