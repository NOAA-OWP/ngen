#ifndef NGEN_BMI_C_ADAPTER_H
#define NGEN_BMI_C_ADAPTER_H

#include <memory>
#include <string>
#include <dlfcn.h>
#include "bmi.h"
#include "Bmi_Adapter.hpp"
#include "State_Exception.hpp"
#include "ExternalIntegrationException.hpp"
#include "JSONProperty.hpp"
#include "StreamHandler.hpp"

// Forward declaration to provide access to protected items in testing
class Bmi_C_Adapter_Test;

using C_Bmi = ::Bmi;

namespace models {
    namespace bmi {

        /**
         * An adapter class to serve as a C++ interface to the essential aspects of external models written in the C
         * language that implement the BMI.
         */
        class Bmi_C_Adapter : public Bmi_Adapter<C_Bmi> {

        public:

            /**
             * Public constructor without path to BMI initialization config file.
             *
             * @param type_name The name of the backing BMI module/model type.
             * @param library_file_path The string path to the shared library file for external module.
             * @param forcing_file_path The string path for the forcing file the module should use, empty if it does not
             *                          use one directly.
             * @param allow_exceed_end Whether the backing model is allowed to execute beyond its advertised end_time.
             * @param has_fixed_time_step Whether the model has a fixed time step size.
             * @param registration_func The name for the @see bmi_registration_function.
             * @param output The output stream handler.
             */
            explicit Bmi_C_Adapter(const string &type_name, std::string library_file_path, std::string forcing_file_path,
                                   bool allow_exceed_end, bool has_fixed_time_step,
                                   const std::string& registration_func, utils::StreamHandler output);

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
            Bmi_C_Adapter(const string &type_name, std::string library_file_path, std::string bmi_init_config,
                          std::string forcing_file_path, bool allow_exceed_end, bool has_fixed_time_step,
                          std::string registration_func, utils::StreamHandler output);

        protected:

            /**
             * Protected constructor that allows control over whether initialization steps are done during construction.
             *
             * Constructor that has parameter to control whether initialization steps - i.e., steps that would be
             * performed in the BMI @see Initialize(std::string) function if not already done - are done when
             * constructing the object.
             *
             * In general, it is assumed that an object of this type will be initialized on construction.  However, a
             * subtype may wish to utilize the constructor while deferring initialization (usually so it can perform
             * that internally as a later step).
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
             * @param do_initialization Whether initialization should be performed during construction or deferred.
             */
            Bmi_C_Adapter(const string &type_name, std::string library_file_path, std::string bmi_init_config,
                          std::string forcing_file_path, bool allow_exceed_end, bool has_fixed_time_step,
                          std::string registration_func, utils::StreamHandler output, bool do_initialization);

        public:

            // Copy constructor
            // TODO: since the dynamically loaded lib and model struct can't easily be copied (without risking breaking
            //  once original object closes the handle for its dynamically loaded lib) it make more sense to remove the
            //  copy constructor.
            // TODO: However, it may make sense to bring it back once it's possible to serialize/deserialize the model.
            //Bmi_C_Adapter(Bmi_C_Adapter &adapter);

            // Move constructor
            Bmi_C_Adapter(Bmi_C_Adapter &&adapter) noexcept;

            /**
             * Class destructor.
             *
             * Note that this calls the `Finalize()` function for cleaning up this object and its backing BMI model.
             */
            virtual ~Bmi_C_Adapter();

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

            string GetComponentName() override;

            /**
             * Get the backing model's current time.
             *
             * Get the backing model's current time, relative to the start time, in the units expressed by
             * `GetTimeUnits()`.
             *
             * @return The backing model's current time.
             */
            double GetCurrentTime() override;

            double GetEndTime() override;

            /**
             * The number of input variables the model can use.
             *
             * The number of input variables the model can use from other models implementing a BMI.
             *
             * @return The number of input variables the model can use from other models implementing a BMI.
             */
            int GetInputItemCount() override;

            /**
             * Get input variable names for the backing model.
             *
             * Gets a collection of names for the variables the model can use from other models implementing a BMI.
             *
             * @return A vector of names for the variables the model can use from other models implementing a BMI.
             */
            std::vector<std::string> GetInputVarNames() override;

            int GetOutputItemCount() override;

            std::vector<std::string> GetOutputVarNames() override;

            /**
             * Get the backing model's starting time.
             *
             * Get the backing model's starting time, in the units expressed by `GetTimeUnits()`.
             *
             * The BMI standard start time is typically defined to be `0.0`.
             *
             * @return
             */
            double GetStartTime() override;

            /**
             * Get the time step used in the model.
             *
             * Get the time step size used in the backing model, expressed as a `double` type value.
             *
             * This function defers to the behavior of the analogous function `get_time_step` of the backing model.  As
             * such, the the model-specific documentation for this function should be consulted for utilized models.
             *
             * @return The time step size of the model.
             */
            inline double GetTimeStep() override {
                return *get_bmi_model_time_step_size_ptr();
            }

            /**
             * Get the units of time for the backing model.
             *
             * Get the units of time for the backing model, as a standard convention string representing the unit
             * obtained directly from the model.
             *
             * @return A conventional string representing the time units for the model.
             */
            std::string GetTimeUnits() override;

            /**
             * Get value(s) for a variable.
             *
             * Implementation of virtual function in BMI interface.
             *
             * @param name
             * @param dest
             * @see get_value
             */
            void GetValue(std::string name, void *dest) override {
                get_value(name, dest);
            }

            /**
             * Get a copy of values of a given variable.
             *
             * @tparam T
             * @param name
             * @see get_value
             */
            template <typename T>
            std::vector<T> GetValue(const std::string& name) {
                std::string type = GetVarType(name);
                int total_mem = GetVarNbytes(name);
                int item_size = GetVarItemsize(name);
                int num_items = total_mem/item_size;

                void* dest = malloc(total_mem);

                get_value(name, dest);

                std::vector<T> retrieved_results(num_items);
                T* d_results_ptr;
                d_results_ptr = (T*) dest;

                for (int i = 0; i < num_items; i++)
                    retrieved_results[i] = d_results_ptr[i];

                /*
                std::vector<T> retrieved_results;

                if (type == "double") {
                    retrieved_results = std::vector<double>(num_items);
                    double d_results[num_items];
                    double* d_results_ptr = d_results;
                    d_results_ptr = (double*) dest;
                    for (int i = 0; i < num_items; i++)
                        retrieved_results[i] = d_results_ptr[i];
                }
                */

                free(dest);
                return retrieved_results;
            }

            void GetValueAtIndices(std::string name, void *dest, int *inds, int count) override;

            void GetGridShape(const int grid, int *shape) override;

            void GetGridSpacing(const int grid, double *spacing) override;

            void GetGridOrigin(const int grid, double *origin) override;

            void GetGridX(const int grid, double *x) override;

            void GetGridY(const int grid, double *y) override;

            void GetGridZ(const int grid, double *z) override;

            int GetGridNodeCount(const int grid) override;

            int GetGridEdgeCount(const int grid) override;

            int GetGridFaceCount(const int grid) override;

            void GetGridEdgeNodes(const int grid, int *edge_nodes) override;

            void GetGridFaceEdges(const int grid, int *face_edges) override;

            void GetGridFaceNodes(const int grid, int *face_nodes) override;

            void GetGridNodesPerFace(const int grid, int *nodes_per_face) override;

            void *GetValuePtr(std::string name) override {
                void *dest;
                if (bmi_model->get_value_ptr(bmi_model.get(), name.c_str(), &dest) != BMI_SUCCESS) {
                    throw std::runtime_error(model_name + " failed to get pointer for BMI variable " + name + ".");
                }
                return dest;
            }

            /**
             * Get a reference to the value(s) of a given variable.
             *
             * This function gets and returns the reference (i.e., pointer) to the backing model's variable with the
             * given name.  Unlike what is returned by `GetValue()`, this will point to the current value(s) of the
             * variable, even if the model's state has changed.
             *
             * Because of how C-based BMI models are implemented, raw pointers are returned.  The pointers are "owned"
             * by the model implementation, though, so there should be no risk of memory leaks (i.e., strictly as a
             * result of obtaining a reference via this function.)
             *
             * @tparam T   The type of the variable to which a reference will be returned.
             * @param name The name of the variable to which a reference will be returned.
             * @return A reference to the value(s) of a given variable
             */
            template<class T>
            T *GetValuePtr(const std::string &name) {
                int nbytes;
                if (bmi_model->get_var_nbytes(bmi_model.get(), name.c_str(), &nbytes) != BMI_SUCCESS)
                    throw std::runtime_error(model_name + " failed to get pointer for BMI variable " + name + ".");
                void *dest = GetValuePtr(name);
                T *ptr = (T *) dest;
                return ptr;
            }

            /**
             * Get the size (in bytes) of one item of a variable.
             *
             * @param name
             * @return
             */
            int GetVarItemsize(std::string name) override;

            /**
             * Get the total size (in bytes) of a variable.
             *
             * This function provides the total amount of memory used to store a variable; i.e., the number of items
             * multiplied by the size of each item.
             *
             * @param name
             * @return
             */
            int GetVarNbytes(std::string name) override;

            /**
             * Get the data type of a variable.
             *
             * @param name
             * @return
             */
            int GetVarGrid(std::string name) override;

            /**
             * Get the grid (id) of a variable.
             *
             * @param name
             * @return
             */
            std::string GetVarType(std::string name) override;

            /**
             * Get the units for a variable.
             *
             * @param name
             * @return
             */
            std::string GetVarUnits(std::string name) override;

            /**
             * Get location for a variable.
             *
             * @param name
             * @return
             */
            std::string GetVarLocation(std::string name) override;

            /**
             * Get grid type for a grid-id.
             *
             * @param name
             * @return
             */
            std::string GetGridType(int grid_id) override;

            /**
             * Get grid rank for a grid-id.
             *
             * @param name
             * @return
             */
            int GetGridRank(int grid_id) override;

            /**
             * Get grid size for a grid-id.
             *
             * @param name
             * @return
             */
            int GetGridSize(int grid_id) override;

            /**
             * Whether the backing model has been initialized yet.
             *
             * @return Whether the backing model has been initialized yet.
             */
            bool is_model_initialized();

            void SetValue(std::string name, void *src) override;

            template<class T>
            void SetValue(std::string name, std::vector<T> src) {
                size_t item_size;
                try {
                    item_size = (size_t) GetVarItemsize(name);
                }
                catch (std::runtime_error &e) {
                    throw std::runtime_error("Cannot set " + name + " variable of " + model_name +
                                             "; unable to test item sizes are equal (does variable exist for model?)");
                }
                if (item_size != sizeof(src[0])) {
                    throw std::runtime_error("Cannot set " + name + " variable of " + model_name +
                                             " with values of different item size");
                }
                SetValue(std::move(name), static_cast<void *>(src.data()));
            }

            void SetValueAtIndices(std::string name, int *inds, int count, void *src) override;

            /**
             *
             * @tparam T
             * @param name
             * @param inds Collection of indexes within the model for which this variable should have a value set.
             * @param src Values that should be set for this variable at the corresponding index in `inds`.
             */

            template<class T>
            void SetValueAtIndices(const std::string &name, std::vector<int> inds, std::vector<T> src) {
                if (inds.size() != src.size()) {
                    throw std::runtime_error("Cannot set specified indexes for " + name + " variable of " + model_name +
                                             " when index collection is different size than collection of values.");
                }
                if (inds.empty()) {
                    return;
                }
                size_t item_size;
                try {
                    item_size = (size_t) GetVarItemsize(name);
                }
                catch (std::runtime_error &e) {
                    throw std::runtime_error("Cannot set specified indexes for " + name + " variable of " + model_name +
                                             "; unable to test item sizes are equal (does variable exist for model?)");
                }
                if (item_size != sizeof(src[0])) {
                    throw std::runtime_error("Cannot set specified indexes for " + name + " variable of " + model_name +
                                             " with values of different item size");
                }
                SetValueAtIndices(name, inds.data(), inds.size(), static_cast<void *>(src.data()));
            }

            /**
             * Have the backing model update to next time step.
             *
             * Have the backing BMI model perform an update to the next time step according to its own internal time
             * keeping.
             */
            void Update() override;

            /**
             * Have the backing BMI model update to the specified time.
             *
             * Update the backing BMI model to some desired model time, specified either explicitly or implicitly as a
             * non-integral multiple of time steps.  Note that the former is supported, but not required, by the BMI
             * specification.  The same is true for negative argument values.
             *
             * This function does not attempt to determine whether the particular backing model will consider the
             * provided parameter valid.
             *
             * @param time Time to update model to, either as literal model time or non-integral multiple of time steps.
             */
            void UpdateUntil(double time) override;

        protected:
            // TODO: look at setting this in some other way
            std::string model_name = "BMI C model";

            /**
             * Construct the backing BMI model object, then call its BMI-native ``Initialize()`` function.
             *
             * Implementations should return immediately without taking any further action if ``model_initialized`` is
             * already ``true``.
             *
             * The call to the BMI native ``Initialize(string)`` should pass the value stored in ``bmi_init_config``.
             * @see construct_and_init_backing_model_for_type
             */
            void construct_and_init_backing_model() override {
                construct_and_init_backing_model_for_type();
            }

            /**
             * Dynamically load the required shared library and the backing BMI model itself.
             *
             * Dynamically load the external shared library for this object's backing model.  Then load this object's
             * "instance" of the model itself.  For C BMI models (as well as other for which the adapter is derived from
             * this type), this is actually a struct with function pointer, rather than a class with member functions.
             *
             * Libraries should provide an additional ``register_bmi`` function.  For C modules, this essentially works
             * as a constructor (or factory) for the model struct, accepting a pointer to a BMI struct and then setting
             * the appropriate function pointer values.
             *
             * A handle to the dynamically loaded library (as a ``void*``) is maintained in within a private member
             * variable.  A warning will output if this function is called with the handle already set (i.e., with the
             * library already loaded), and then the function will returns without taking any other action.
             *
             * @throws ``std::runtime_error`` If configured BMI library file is not readable.
             * @throws ``::external::ExternalIntegrationException`` If symbol for registration function could not be
             *                                                      found for the shared library handle.
             */
            inline void dynamic_library_load() {
                if (dyn_lib_handle != nullptr) {
                    output.put("WARNING: ignoring attempt to reload dynamic shared library '" + bmi_lib_file +
                               "' for BMI model " + model_name);
                    return;
                }
                if (!utils::FileChecker::file_is_readable(bmi_lib_file)) {
                    init_exception_msg =
                            "Cannot init " + model_name + "; unreadable dynamic shared lib file '" + bmi_lib_file + "'";
                    throw std::runtime_error(init_exception_msg);
                }
                if (bmi_registration_function.empty()) {
                    init_exception_msg =
                            "Cannot init " + model_name + "; empty name given for library's registration function.";
                    throw std::runtime_error(init_exception_msg);
                }

                // TODO: add support for either the configured-by-name mapping or just using the standard names
                void *symbol;
                C_Bmi *(*dynamic_register_bmi)(C_Bmi *model);

                // Load up the necessary library dynamically
                dyn_lib_handle = dlopen(bmi_lib_file.c_str(), RTLD_NOW | RTLD_LOCAL);

                try {
                    // Acquire the BMI struct func pointer registration function
                    symbol = dynamic_load_symbol(bmi_registration_function);
                    dynamic_register_bmi = (C_Bmi *(*)(C_Bmi *)) symbol;
                    // Call registration function, which (for C libs) sets up object's pointed-to member BMI struct
                    // (Note that this probably is not the case for the Fortran subclass, though the registration
                    // function is still expected and utilized).
                    dynamic_register_bmi(bmi_model.get());
                }
                catch (const ::external::ExternalIntegrationException &e) {
                    // "Override" the default message in this case
                    init_exception_msg =
                            "Cannot init " + model_name + " without valid library registration function: " +
                            init_exception_msg;
                    throw ::external::ExternalIntegrationException(init_exception_msg);
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
                    throw std::runtime_error("Cannot load symbol " + symbol_name + "without handle to shared library");
                }
                // Call first to ensure any previous error is cleared before trying to load the symbol
                dlerror();
                void *symbol = dlsym(dyn_lib_handle, symbol_name.c_str());
                // Now call again to see if there was an error (if there was, this will not be null)
                char *err_message = dlerror();
                if (symbol == nullptr && (err_message != nullptr || !is_null_valid)) {
                    init_exception_msg = "Cannot load shared lib symbol '" + symbol_name + "' for model " + model_name;
                    if (err_message != nullptr) {
                        init_exception_msg += " (" + std::string(err_message) + ")";
                    }
                    throw ::external::ExternalIntegrationException(init_exception_msg);
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

            /**
             * A non-virtual equivalent for the virtual @see Finalize.
             *
             * Primarily, this exists to contain the functionality appropriate for @see Finalize in a function that is
             * non-virtual, and can therefore be called by a destructor.
             */
            void finalizeForSubtype();

            /**
             * Get model time step size pointer, using lazy loading when fixed.
             *
             * Get a pointer to the value of the backing model's time step size.  If the model is configured to have
             * fixed time step size, return the pointer from `bmi_model_time_step_size`, potentially after it is lazily
             * loaded.  If the model's time step is not fixed, retrieve it directly from the model in real time, and do
             * not set `bmi_model_time_step_size`.
             *
             * @return A pointer to the value of the backing model's time step size.
             * @see retrieve_bmi_model_time_step_size
             */
            inline std::shared_ptr<double> get_bmi_model_time_step_size_ptr() {
                if (bmi_model_time_step_size == nullptr) {
                    if (!bmi_model_has_fixed_time_step)
                        return retrieve_bmi_model_time_step_size();
                    else
                        bmi_model_time_step_size = retrieve_bmi_model_time_step_size();
                }
                return bmi_model_time_step_size;
            }

            /**
             * Internal implementation of logic used for @see GetValue.
             *
             * Essentially, function exists as inner implementation.  This allows it to be inlined, which may lead to
             * optimization in certain situations.
             *
             * @param name The name of the variable for which to get values.
             * @param dest A function pointer in which to return the values.
             */
            inline void get_value(const std::string& name, void *dest) {
                if (bmi_model->get_value(bmi_model.get(), name.c_str(), dest) != BMI_SUCCESS) {
                    throw std::runtime_error(model_name + " failed to get values for variable " + name + ".");
                }
            }

            /**
             * Retrieve time step size directly from model.
             *
             * Retrieve the time step size from the backing model and return in a shared pointer.
             *
             * @return Shared pointer to just-retrieved time step size
             */
            inline std::shared_ptr<double> retrieve_bmi_model_time_step_size() {
                double ts;
                int result = bmi_model->get_time_step(bmi_model.get(), &ts);
                if (result != BMI_SUCCESS) {
                    throw std::runtime_error(model_name + " failed to read time step from model.");
                }
                return std::make_shared<double>(ts);
            }

        private:
            /** Path to the BMI shared library file, for dynamic linking. */
            std::string bmi_lib_file;
            /** Name of the function that registers BMI struct's function pointers to the right module functions. */
            const std::string bmi_registration_function;
            /** Handle for dynamically loaded library file. */
            void *dyn_lib_handle = nullptr;

            /**
             * Construct the backing BMI model object, then call its BMI-native ``Initialize()`` function.
             *
             * The essentially provides the functionality for @see construct_and_init_backing_model without being a
             * virtual function.
             *
             * Implementations should return immediately without taking any further action if ``model_initialized`` is
             * already ``true``.
             *
             * The call to the BMI native ``Initialize(string)`` should pass the value stored in ``bmi_init_config``.
             */
            inline void construct_and_init_backing_model_for_type() {
                if (model_initialized)
                    return;
                bmi_model = std::make_shared<C_Bmi>(C_Bmi());
                dynamic_library_load();
                int init_result = bmi_model->initialize(bmi_model.get(), bmi_init_config.c_str());
                if (init_result != BMI_SUCCESS) {
                    init_exception_msg = "Failure when attempting to initialize " + model_name;
                    throw models::external::State_Exception(init_exception_msg);
                }
            }

            /**
             * Helper method for getting either input or output variable names.
             *
             * @param is_input_variable Whether input variable names should be retrieved (as opposed to output).
             * @return
             */
            std::shared_ptr<std::vector<std::string>> get_variable_names(bool is_input_variable);

            // For unit testing
            friend class ::Bmi_C_Adapter_Test;

        };

    }
}

#endif //NGEN_BMI_C_ADAPTER_H
