#ifndef NGEN_BMI_C_ADAPTER_H
#define NGEN_BMI_C_ADAPTER_H

#include <memory>
#include <string>
#include "bmi.h"
#include "JSONProperty.hpp"
#include "StreamHandler.hpp"

// Declaring registration method from BMI model library
extern "C" {
    Bmi* register_bmi(Bmi *model);
};

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

            explicit Bmi_C_Adapter(std::string forcing_file_path, bool model_uses_forcing_file, bool allow_exceed_end,
                                   utils::StreamHandler output);

            Bmi_C_Adapter(std::string bmi_init_config, std::string forcing_file_path, bool model_uses_forcing_file,
                          bool allow_exceed_end, utils::StreamHandler output);

            Bmi_C_Adapter(std::string forcing_file_path, bool model_uses_forcing_file, bool allow_exceed_end,
                          const geojson::JSONProperty& other_input_vars, utils::StreamHandler output);

            Bmi_C_Adapter(const std::string& bmi_init_config, std::string forcing_file_path,
                          bool model_uses_forcing_file, bool allow_exceed_end,
                          const geojson::JSONProperty& other_input_vars, utils::StreamHandler output);

            // Copy constructor
            Bmi_C_Adapter(Bmi_C_Adapter &adapter);

            // Move constructor
            Bmi_C_Adapter(Bmi_C_Adapter &&adapter) noexcept;

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
             * Get the backing model "next" time step size.
             *
             * Get the time step size used in the backing model for the "next", in the units expressed by
             * `GetTimeUnits()`.
             *
             * This type does not make any assumptions about how the backing model handles time steps, including whether
             * the time step size is fixed.  This creates some complexities.  In particular, this function makes a
             * best-effort for determining the time step size.  It attempts to do this first by analyzing the data in
             * the forcing file at `forcing_file_path`, for whatever the next time step is.  This may not be possible,
             * in which case it falls back to the value returned from the analogous BMI method from the backing model.
             *
             * @return The time step size of the model.
             */
            double GetTimeStep();

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
            std::string GetVarType(std::string name);

            /**
             * Get the units for a variable.
             *
             * @param name
             * @return
             */
            std::string GetVarUnits(std::string name);

            /**
             * Get the last processed time step.
             *
             * Get the 0-based index of the last time step that the model processed via it's `Update()` function.
             *
             * The
             *
             * @return
             */
            int get_last_processed_time_step();

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

            /**
             * Whether the backing model has been initialized yet.
             *
             * @return Whether the backing model has been initialized yet.
             */
            bool is_model_initialized();

            void SetValue(std::string name, void *src);

            template <typename T>
            void SetValue(std::string name, std::vector<T> src, size_t src_item_size);

            void SetValueAtIndices(std::string name, int *inds, int count, void *src);

            /**
             *
             * @tparam T
             * @param name
             * @param inds Collection of indexes within the model for which this variable should have a value set.
             * @param src Values that should be set for this variable at the corresponding index in `inds`.
             */
             /*
            template <class T>
            void SetValueAtIndices(const std::string& name, std::vector<int> inds, std::vector<T> src, size_t src_item_size);
              */

             void Update();

        protected:
            // TODO: look at setting this in some other way
            std::string model_name = "BMI C model";

        private:
            /** Whether model ``Update`` calls are allowed and handled in some way by the backing model. */
            bool allow_model_exceed_end_time = false;
            /** Path (as a string) to the BMI config file for initializing the backing model (empty if none). */
            std::string bmi_init_config;
            /** Pointer to C BMI model struct object. */
            std::shared_ptr<Bmi> bmi_model;
            /** Whether this particular model implementation directly reads input data from the forcing file. */
            bool bmi_model_uses_forcing_file;
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

        };

    }
}

#endif //NGEN_BMI_C_ADAPTER_H
