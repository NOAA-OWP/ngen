#ifndef NGEN_BMI_ADAPTER_HPP
#define NGEN_BMI_ADAPTER_HPP

#include <string>
#include <vector>

#include "bmi.hpp"

#include "core/mediator/UnitsHelper.hpp"
#include "utilities/StreamHandler.hpp"


namespace models {
    namespace bmi {
        /**
         * Abstract adapter interface for C++ classes to interact with the essential aspects of external models that
         * implement the BMI spec but that are written in some other programming language.
         */
        class Bmi_Adapter : public ::bmi::Bmi {
        public:

            Bmi_Adapter(std::string model_name, std::string bmi_init_config, bool allow_exceed_end,
                        bool has_fixed_time_step);

            Bmi_Adapter(Bmi_Adapter const&) = delete;
            Bmi_Adapter(Bmi_Adapter &&) = delete;

            virtual ~Bmi_Adapter() = 0;

            /**
             * Whether the backing model has been initialized yet.
             *
             * @return Whether the backing model has been initialized yet.
             */
            virtual bool is_model_initialized() = 0;

            /**
             * Determine backing model's time units and return an appropriate conversion factor.
             *
             * A backing BMI model may use arbitrary units for time, but it will expose what those units are via the
             * BMI ``GetTimeUnits`` function. This function retrieves (and interprets) its model's units and
             * return an appropriate factor for converting its internal time values to equivalent representations
             * within the model, and vice versa. This function complies with the BMI get_time_units standard
             * 
             * @throws runtime_error  If the delegated BMI functions to query time throw an exception, the
             *                        exception is caught and wrapped in a runtime_error
             */
            double get_time_convert_factor();

            /**
             * Convert model time value to value in seconds.
             *
             * Performs necessary lookup and conversion of some given model time value - as from `GetCurrentTime()` or
             * `GetStartTime()` - and returns the equivalent value when represented in seconds.
             *
             * @param model_time_val Arbitrary model time value in units provided by `GetTimeUnits()`
             * @return Equivalent model time value to value in seconds.
             */
            double convert_model_time_to_seconds(const double& model_time_val);

            /**
             * Convert a given number of seconds to equivalent in model time units.
             *
             * @param seconds_val
             * @return
             */
            double convert_seconds_to_model_time(const double& seconds_val);

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
            void Initialize();

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
            void Initialize(std::string config_file) override;

            /**
             * Get whether this instance has been initialized properly.
             *
             * @return Whether this instance has been initialized properly.
             */
            bool isInitialized();

            /**
             * @brief Get the model name.
             * 
             * @return The name of the model connected to the adapter.
             * 
             */
            std::string get_model_name();

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
            /** Message from an exception (if encountered) on the first attempt to initialize the backing model. */
            std::string init_exception_msg;
            /** Pointer to collection of input variable names for backing model, used by ``GetInputVarNames()``. */
            std::shared_ptr<std::vector<std::string>> input_var_names;
            /** Whether the backing model has been initialized yet, which is always initially ``false``. */
            bool model_initialized = false;
            std::string model_name;
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
