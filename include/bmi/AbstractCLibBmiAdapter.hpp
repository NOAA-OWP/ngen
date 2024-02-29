#ifndef NGEN_ABSTRACTCLIBBMIADAPTER_HPP
#define NGEN_ABSTRACTCLIBBMIADAPTER_HPP

#include <dlfcn.h>

#include "Bmi_Adapter.hpp"

namespace models {
    namespace bmi {

        class AbstractCLibBmiAdapter : public Bmi_Adapter {

        public:
            /**
             * Main public constructor.
             *
             * @param type_name The name of the backing BMI module/model type.
             * @param library_file_path The string path to the shared library file for external module.
             * @param bmi_init_config The string path to the BMI initialization config file for the module.
             * @param forcing_file_path The string path for the forcing file the module should use, empty if it does not
             *                          use one directly.
             * @param allow_exceed_end Whether the backing model is allowed to execute beyond its advertised end_time.
             * @param has_fixed_time_step Whether the model has a fixed time step size.
             * @param registration_func The name for the @see bmi_registration_function.
             * @param output The output stream handler.
             */
            AbstractCLibBmiAdapter(const std::string &type_name, std::string library_file_path, std::string bmi_init_config,
                                   std::string forcing_file_path, bool allow_exceed_end, bool has_fixed_time_step,
                                   std::string registration_func, utils::StreamHandler output);

            AbstractCLibBmiAdapter(AbstractCLibBmiAdapter &&adapter) noexcept;

            /**
             * Class destructor.
             *
             * Note that this performs the logic in the `Finalize()` function for cleaning up this object and its
             * backing BMI model.
             */
            ~AbstractCLibBmiAdapter() noexcept override;

            /**
             * Perform tear-down task for this object and its backing model.
             *
             * The function will simply return if either the pointer to the backing model is `nullptr` (e.g., after use
             * in a move constructor) or if the model has not been initialized.  Otherwise, it will execute its internal
             * tear-down logic, including a nested call to `finalize()` for the backing model.
             *
             * Note that because of how model initialization state is determined, regardless of whether the call to the
             * model's `finalize()` function is successful (i.e., according to the function's return code value), the
             * model will subsequently be consider not initialized.  This essentially means that if backing model
             * tear-down fails, it cannot be retried.
             *
             * @throws models::external::State_Exception Thrown if nested model `finalize()` call is not successful.
             */
            void Finalize() override;

        protected:

            /**
             * Dynamically load and obtain this instance's handle to the shared library.
             */
            void dynamic_library_load();

            /**
             * Load and return the address of the given symbol from the loaded dynamic model shared library.
             *
             * The initial primary purpose for this is to obtain a function pointer for accessing the registration
             * function for a dynamically loaded library, as done in @see dynamic_library_load.  Other function pointers
             * (or pointers to other valid symbols) can also be returned.
             *
             * It is possible for a symbol pointer to actually be null in some cases.  However, this may not be
             * valid in the context in which a call to this function was made.  To control whether this is treated as
             * a valid case, and thus whether null should be returned (instead of an exception thrown), there is the
             * ``is_null_valid`` parameter.  When ``true``, a null symbol will be returned by the function.
             *
             * Typically, a call to @see dynamic_library_load must happen (though not necessarily have completed) before
             * a call to this function to ensure @see dyn_lib_handle is set.  If it is not set, an exception is thrown.
             *
             * @param symbol_name The name of the symbol to load.
             * @param is_null_valid Whether a null address for the symbol is valid, as opposed to implying there was
             *                      simply a failure finding it.
             * @return A void pointer to the address of the desired symbol in memory.
             * @throws ``std::runtime_error`` If there is not a valid handle already to the shared library.
             * @throws ``::external::ExternalIntegrationException`` If symbol could not be found for the shared library.
             */
            void *dynamic_load_symbol(const std::string &symbol_name, bool is_null_valid);

            /**
             * Convenience for @see dynamic_load_symbol(std::string, bool) where the symbol address must be found.
             *
             * This serves as a convenience overload for the function of the same name, in cases when a sought symbol
             * must be found and the return of a null address is not valid.
             *
             * @param symbol_name The name of the symbol to load.
             * @return A void pointer to the address of the desired symbol in memory.
             * @throws ``std::runtime_error`` If there is not a valid handle already to the shared library.
             * @throws ``::external::ExternalIntegrationException`` If symbol could not be found for the shared library.
             * @see dynamic_load_symbol(std::string, bool)
             */
            void *dynamic_load_symbol(const std::string &symbol_name);

            const std::string &get_bmi_registration_function();

            const void *get_dyn_lib_handle();

        private:

            /** Path to the BMI shared library file, for dynamic linking. */
            std::string bmi_lib_file;
            /** Name of the function that registers BMI struct's function pointers to the right module functions. */
            std::string bmi_registration_function;
            /** Handle for dynamically loaded library file. */
            void *dyn_lib_handle = nullptr;

            /**
             * A non-virtual equivalent for the virtual @see Finalize.
             *
             * This function should be kept private.  If its logic needs to be invoked externally, that should be done
             * via the destructor or via the public interface @see Finalize function.
             *
             * Primarily, this exists to contain the functionality appropriate for @see Finalize in a function that is
             * non-virtual, and can therefore be called by a destructor.
             */
            void finalizeForLibAbstraction();
        };

    }
}

#endif //NGEN_ABSTRACTCLIBBMIADAPTER_HPP
