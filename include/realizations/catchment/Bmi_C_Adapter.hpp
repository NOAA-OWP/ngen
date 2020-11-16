#ifndef NGEN_BMI_C_ADAPTER_H
#define NGEN_BMI_C_ADAPTER_H

#include <memory>
#include "bmi.h"
#include "JSONProperty.hpp"
#include "StreamHandler.hpp"

namespace models {
    namespace bmi {

        /**
         * An adapter class to serve as a C++ interface to the essential aspects of external models written in the C
         * language that implement the BMI.
         */
        class Bmi_C_Adapter {

            // TODO: how to figure out what exactly needs to be called (i.e., how to get at the particular C function
            //  for the particular model?)

            // TODO: implement the essential BMI methods and translate to C calls

        public:

            explicit Bmi_C_Adapter(utils::StreamHandler output);

            Bmi_C_Adapter(std::string bmi_init_config, utils::StreamHandler output);

            Bmi_C_Adapter(const geojson::JSONProperty& other_input_vars, utils::StreamHandler output);

            Bmi_C_Adapter(const std::string& bmi_init_config, const geojson::JSONProperty& other_input_vars,
                          utils::StreamHandler output);

            /**
             * The number of input variables the model can use from other models implementing a BMI.
             *
             * @return The number of input variables the model can use from other models implementing a BMI.
             */
            int get_input_item_count();

            /**
             * Gets a collection of names for the variables the model can use from other models implementing a BMI.
             *
             * @return A vector of names for the variables the model can use from other models implementing a BMI.
             */
            std::vector<std::string> get_input_var_names();

            /**
             * Initialize the wrapped BMI model functionality using the value from the `bmi_init_config` member variable
             * and the API's ``Initialize`` function.
             *
             * If no attempt to initialize the model has yet been made (i.e., ``model_initialized`` is ``false`` when
             * this function is called), then ``model_initialized`` is set to ``true`` and initialization is attempted
             * for the model. If initialization fails, an exception will be raised, with it's message saved as part of
             * this object's state.
             *
             * If an attempt to initialize the model has already been made (i.e., ``model_initialized`` is ``true``),
             * this function will either simply return or will throw a runtime_error, with its message including the
             * message of the exception from the earlier attempt.
             */
            void Initialize();

            /**
             * Initialize the wrapped BMI model object using the given config file and the object's ``Initialize``
             * function.
             *
             * If the given file is not the same as what is in `bmi_init_config`` and the model has not already
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
            void Initialize(const std::string& config_file);

        private:

            /** Path (as a string) to the BMI config file for initializing the backing model (empty if none). */
            std::string bmi_init_config;
            /** Pointer to C BMI model struct object. */
            std::shared_ptr<Bmi> bmi_model;
            /** Message from an exception (if encountered) on the first attempt to initialize the backing model. */
            std::string init_exception_msg;
            /** Pointer to collection of input variable names for backing model, used by ``get_input_var_names()``. */
            std::shared_ptr<std::vector<std::string>> input_var_names;
            /** Whether the backing model has been initialized yet, which is always initially ``false``. */
            bool model_initialized = false;
            utils::StreamHandler output;
        };

    }
}

#endif //NGEN_BMI_C_ADAPTER_H
