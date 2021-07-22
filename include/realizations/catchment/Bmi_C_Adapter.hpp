#ifndef NGEN_BMI_C_ADAPTER_H
#define NGEN_BMI_C_ADAPTER_H

#include <memory>
#include <string>
#include "bmi.h"
#include "State_Exception.hpp"
#include "JSONProperty.hpp"
#include "StreamHandler.hpp"

// Forward declaration to provide access to protected items in testing
class Bmi_C_Adapter_Test;

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

            explicit Bmi_C_Adapter(std::string library_file_path, std::string forcing_file_path,
                                   bool model_uses_forcing_file, bool allow_exceed_end, bool has_fixed_time_step,
                                   const std::string& registration_func, utils::StreamHandler output);

            Bmi_C_Adapter(std::string library_file_path, std::string bmi_init_config,
                          std::string forcing_file_path, bool model_uses_forcing_file, bool allow_exceed_end,
                          bool has_fixed_time_step, std::string  registration_func, utils::StreamHandler output);

            Bmi_C_Adapter(std::string library_file_path, std::string forcing_file_path, bool model_uses_forcing_file,
                          bool allow_exceed_end, bool has_fixed_time_step, const std::string& registration_func,
                          const geojson::JSONProperty &other_input_vars, utils::StreamHandler output);

            Bmi_C_Adapter(std::string library_file_path, const std::string &bmi_init_config,
                          std::string forcing_file_path, bool model_uses_forcing_file, bool allow_exceed_end,
                          bool has_fixed_time_step, const std::string& registration_func,
                          const geojson::JSONProperty &other_input_vars, utils::StreamHandler output);

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
            void Finalize();

            /**
             * Get the backing model's current time.
             *
             * Get the backing model's current time, relative to the start time, in the units expressed by
             * `GetTimeUnits()`.
             *
             * @return The backing model's current time.
             */
            double GetCurrentTime();

            double GetEndTime();

            /**
             * The number of input variables the model can use.
             *
             * The number of input variables the model can use from other models implementing a BMI.
             *
             * @return The number of input variables the model can use from other models implementing a BMI.
             */
            int GetInputItemCount();

            /**
             * Get input variable names for the backing model.
             *
             * Gets a collection of names for the variables the model can use from other models implementing a BMI.
             *
             * @return A vector of names for the variables the model can use from other models implementing a BMI.
             */
            std::vector<std::string> GetInputVarNames();

            int GetOutputItemCount();

            std::vector<std::string> GetOutputVarNames();

            /**
             * Get the backing model's starting time.
             *
             * Get the backing model's starting time, in the units expressed by `GetTimeUnits()`.
             *
             * The BMI standard start time is typically defined to be `0.0`.
             *
             * @return
             */
            double GetStartTime();

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
            inline double GetTimeStep() {
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
            std::string GetTimeUnits();

             /**
              * Get a copy of values of a given variable.
              *
              * @tparam T
              * @param name
              */
             template <typename T>
             std::vector<T> GetValue(const std::string& name) {
                 std::string type = GetVarType(name);
                 int total_mem = GetVarNbytes(name);
                 int item_size = GetVarItemsize(name);
                 int num_items = total_mem/item_size;

                 void* dest = malloc(total_mem);

                 int result = bmi_model->get_value(bmi_model.get(), name.c_str(), dest);
                 if (result != BMI_SUCCESS) {
                     free(dest);
                     throw std::runtime_error(model_name + " failed to get values for variable " + name + ".");
                 }

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
             template <class T>
             T* GetValuePtr(const std::string& name) {
                 int nbytes;
                 int result = bmi_model->get_var_nbytes(bmi_model.get(), name.c_str(), &nbytes);
                 if (result != BMI_SUCCESS)
                     throw std::runtime_error(model_name + " failed to get pointer for BMI variable " + name + ".");
                 void* dest;
                 result = bmi_model->get_value_ptr(bmi_model.get(), name.c_str(), &dest);
                 if (result != BMI_SUCCESS) {
                     throw std::runtime_error(model_name + " failed to get pointer for BMI variable " + name + ".");
                 }
                 T* ptr = (T*) dest;
                 return ptr;
             }

            /**
             * Get the size (in bytes) of one item of a variable.
             *
             * @param name
             * @return
             */
            int GetVarItemsize(std::string name);

            /**
             * Get the total size (in bytes) of a variable.
             *
             * This function provides the total amount of memory used to store a variable; i.e., the number of items
             * multiplied by the size of each item.
             *
             * @param name
             * @return
             */
            int GetVarNbytes(std::string name);

            /**
             * Get the data type of a variable.
             *
             * @param name
             * @return
             */
            int GetVarGrid(std::string name);

            /**
             * Get the grid (id) of a variable.
             *
             * @param name
             * @return
             */
            std::string GetVarType(std::string name);

            /**
             * Get the units for a variable.
             *
             * @param name
             * @return
             */
            std::string GetVarUnits(std::string name);

            /**
             * Get location for a variable.
             *
             * @param name
             * @return
             */
            std::string GetVarLocation(std::string name);

            /**
             * Get grid type for a grid-id.
             *
             * @param name
             * @return
             */
            std::string GetGridType(int grid_id);

            /**
             * Get grid rank for a grid-id.
             *
             * @param name
             * @return
             */
            int GetGridRank(int grid_id);

            /**
             * Get grid size for a grid-id.
             *
             * @param name
             * @return
             */
            int GetGridSize(int grid_id);
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
             *
             * @throws std::runtime_error  If called again after earlier call that resulted in an exception, or if BMI
             *                             config file could not be read.
             * @throws models::external::State_Exception   If `initialize()` in nested model does not return successful.
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
             * @throws std::runtime_error  If called again after earlier call that resulted in an exception, or if
             *                             called again after a previously successful call with a different
             *                             `config_file`, or if BMI config file could not be read.
             * @throws models::external::State_Exception   If `initialize()` in nested model does not return successful.
             */
            void Initialize(const std::string& config_file);

            /**
             * Whether the backing model has been initialized yet.
             *
             * @return Whether the backing model has been initialized yet.
             */
            bool is_model_initialized();

            void SetValue(std::string name, void *src);

            template <class T>
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
                SetValue(std::move(name), static_cast<void*>(src.data()));
            }

            void SetValueAtIndices(std::string name, int *inds, int count, void *src);

            /**
             *
             * @tparam T
             * @param name
             * @param inds Collection of indexes within the model for which this variable should have a value set.
             * @param src Values that should be set for this variable at the corresponding index in `inds`.
             */

            template <class T>
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
            void Update();

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
            void UpdateUntil(double time);

        protected:
            // TODO: look at setting this in some other way
            std::string model_name = "BMI C model";

            /**
             * Dynamically load the required C library and the backing BMI model itself.
             *
             * Dynamically load the external C library for this object's backing model.  Then load this object's "instance" of the
             * model itself.  For C BMI models, this is actually a struct with function pointer (rather than a class with member
             * functions).
             *
             * Libraries should provide an additional ``register_bmi`` function that essentially works as a constructor (or factory)
             * for the model struct, accepts a pointer to a BMI struct and then setting the appropriate function pointer values.
             *
             * A handle to the dynamically loaded library (as a ``void*``) is maintained in within a private member variable.  A
             * warning will output if this function is called with the handle already set (i.e., with the library already loaded),
             * and then the function will returns without taking any other action.
             *
             * @throws ``std::runtime_error`` Thrown if the configured BMI C library file is not readable.
             */
            inline void dynamic_library_load();

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
            /** Whether model ``Update`` calls are allowed and handled in some way by the backing model. */
            bool allow_model_exceed_end_time = false;
            /** Path (as a string) to the BMI config file for initializing the backing model (empty if none). */
            std::string bmi_init_config;
            /** Path to the BMI shared library file, for dynamic linking. */
            std::string bmi_lib_file;
            /** Pointer to C BMI model struct object. */
            std::shared_ptr<Bmi> bmi_model;
            /** Whether this particular model has a time step size that cannot be changed internally or externally. */
            bool bmi_model_has_fixed_time_step = true;
            /** Conversion factor for converting values for model time in model's unit type to equivalent in seconds. */
            double bmi_model_time_convert_factor;
            /** Pointer to stored time step size value of backing model, if it is fixed and has been retrieved. */
            std::shared_ptr<double> bmi_model_time_step_size = nullptr;
            /** Whether this particular model implementation directly reads input data from the forcing file. */
            bool bmi_model_uses_forcing_file;
            /** Name of the function that registers BMI struct's function pointers to the right module functions. */
            const std::string bmi_registration_function;
            /** Handle for dynamically loaded library file. */
            void* dyn_lib_handle = nullptr;
            std::string forcing_file_path;
            /** Message from an exception (if encountered) on the first attempt to initialize the backing model. */
            std::string init_exception_msg;
            /** Pointer to collection of input variable names for backing model, used by ``GetInputVarNames()``. */
            std::shared_ptr<std::vector<std::string>> input_var_names;
            /** Whether the backing model has been initialized yet, which is always initially ``false``. */
            bool model_initialized = false;
            utils::StreamHandler output;
            /** Pointer to collection of output variable names for backing model, used by ``GetOutputVarNames()``. */
            std::shared_ptr<std::vector<std::string>> output_var_names;

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
