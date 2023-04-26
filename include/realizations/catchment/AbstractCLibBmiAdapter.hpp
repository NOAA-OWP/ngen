#ifndef NGEN_ABSTRACTCLIBBMIADAPTER_HPP
#define NGEN_ABSTRACTCLIBBMIADAPTER_HPP

#include <dlfcn.h>
#include "Bmi_Adapter.hpp"
#include "ExternalIntegrationException.hpp"
#include "State_Exception.hpp"

namespace models {
    namespace bmi {

        template <class C>
        class AbstractCLibBmiAdapter : public Bmi_Adapter<C> {

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
            AbstractCLibBmiAdapter(const string &type_name, std::string library_file_path, std::string bmi_init_config,
                                   std::string forcing_file_path, bool allow_exceed_end, bool has_fixed_time_step,
                                   std::string registration_func, utils::StreamHandler output)
                    : Bmi_Adapter<C>(type_name, std::move(bmi_init_config), std::move(forcing_file_path),
                                     allow_exceed_end, has_fixed_time_step, output),
                      bmi_lib_file(std::move(library_file_path)),
                      bmi_registration_function(std::move(registration_func)) { }

            AbstractCLibBmiAdapter(AbstractCLibBmiAdapter &&adapter) noexcept :
                    Bmi_Adapter<C>(std::move(adapter)),
                    bmi_lib_file(std::move(adapter.bmi_lib_file)),
                    bmi_registration_function(adapter.bmi_registration_function),
                    dyn_lib_handle(adapter.dyn_lib_handle)
            {
                // Have to make sure to do this after "moving" so the original does not close the dynamically loaded library handle
                adapter.dyn_lib_handle = nullptr;
            }

            /**
             * Class destructor.
             *
             * Note that this performs the logic in the `Finalize()` function for cleaning up this object and its
             * backing BMI model.
             */
            virtual ~AbstractCLibBmiAdapter() {
                finalizeForLibAbstraction();
            }

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
            void Finalize() override {
                finalizeForLibAbstraction();
            }

        protected:

            /**
             * Dynamically load and obtain this instance's handle to the shared library.
             */
            inline void dynamic_library_load() {
                if (bmi_registration_function.empty()) {
                    this->init_exception_msg =
                            "Can't init " + this->model_name + "; empty name given for library's registration function.";
                    throw std::runtime_error(this->init_exception_msg);
                }
                if (dyn_lib_handle != nullptr) {
                    this->output.put("WARNING: ignoring attempt to reload dynamic shared library '" + bmi_lib_file +
                    "' for " + this->model_name);
                    return;
                }
                if (!utils::FileChecker::file_is_readable(bmi_lib_file)) {
                    //Try alternative extension...
                    size_t idx = bmi_lib_file.rfind(".");
                    std::string alt_bmi_lib_file;  
                    if(bmi_lib_file.length() == 0){
                        this->init_exception_msg =
                                "Can't init " + this->model_name + "; library file path is empty";
                        throw std::runtime_error(this->init_exception_msg);
                    }                
                    if(bmi_lib_file.substr(idx) == ".so"){
                        alt_bmi_lib_file = bmi_lib_file.substr(0,idx) + ".dylib";
                    } else if(bmi_lib_file.substr(idx) == ".dylib"){
                        alt_bmi_lib_file = bmi_lib_file.substr(0,idx) + ".so";
                    } else {
                        // Try appending instead of replacing...
                        #ifdef __APPLE__
                        alt_bmi_lib_file = bmi_lib_file + ".dylib";
                        #else
                        #ifdef __GNUC__
                        alt_bmi_lib_file = bmi_lib_file + ".so";
                        #endif // __GNUC__
                        #endif // __APPLE__
                    }
                    //TODO: Try looking in e.g. /usr/lib, /usr/local/lib, $LD_LIBRARY_PATH... try pre-pending "lib"...
                    if (utils::FileChecker::file_is_readable(alt_bmi_lib_file)) {
                        bmi_lib_file = alt_bmi_lib_file;
                    } else {
                        this->init_exception_msg =
                                "Can't init " + this->model_name + "; unreadable shared library file '" + bmi_lib_file + "'";
                        throw std::runtime_error(this->init_exception_msg);
                    }

                }

                // Call first to ensure any previous error is cleared before trying to load the symbol
                dlerror();
                // Load up the necessary library dynamically
                dyn_lib_handle = dlopen(bmi_lib_file.c_str(), RTLD_NOW | RTLD_LOCAL);
                // Now call again to see if there was an error (if there was, this will not be null)
                char *err_message = dlerror();
                if (dyn_lib_handle == nullptr && err_message != nullptr) {
                    this->init_exception_msg = "Cannot load shared lib '" + bmi_lib_file + "' for model " + this->model_name;
                    if (err_message != nullptr) {
                        this->init_exception_msg += " (" + std::string(err_message) + ")";
                    }
                    throw ::external::ExternalIntegrationException(this->init_exception_msg);
                }
            }

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
            inline void *dynamic_load_symbol(const std::string &symbol_name, bool is_null_valid) {
                if (dyn_lib_handle == nullptr) {
                    throw std::runtime_error("Cannot load symbol " + symbol_name + " without handle to shared library");
                }
                // Call first to ensure any previous error is cleared before trying to load the symbol
                dlerror();
                void *symbol = dlsym(dyn_lib_handle, symbol_name.c_str());
                // Now call again to see if there was an error (if there was, this will not be null)
                char *err_message = dlerror();
                if (symbol == nullptr && (err_message != nullptr || !is_null_valid)) {
                    this->init_exception_msg = "Cannot load shared lib symbol '" + symbol_name + "' for model " + this->model_name;
                    if (err_message != nullptr) {
                        this->init_exception_msg += " (" + std::string(err_message) + ")";
                    }
                    throw ::external::ExternalIntegrationException(this->init_exception_msg);
                }
                return symbol;
            }

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
            inline void *dynamic_load_symbol(const std::string &symbol_name) {
                return dynamic_load_symbol(symbol_name, false);
            }

            inline const std::string &get_bmi_registration_function() {
                return bmi_registration_function;
            }

            inline const void *get_dyn_lib_handle() {
                return dyn_lib_handle;
            }

        private:

            /** Path to the BMI shared library file, for dynamic linking. */
            std::string bmi_lib_file;
            /** Name of the function that registers BMI struct's function pointers to the right module functions. */
            const std::string bmi_registration_function;
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
            void finalizeForLibAbstraction() {
                //  close the dynamically loaded library
                if (dyn_lib_handle != nullptr) {
                    dlclose(dyn_lib_handle);
                }
            }
        };

    }
}

#endif //NGEN_ABSTRACTCLIBBMIADAPTER_HPP
