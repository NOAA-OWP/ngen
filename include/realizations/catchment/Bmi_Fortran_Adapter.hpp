#ifndef NGEN_BMI_FORTRAN_ADAPTER_HPP
#define NGEN_BMI_FORTRAN_ADAPTER_HPP

#ifdef ACTIVATE_FORTRAN

#include "AbstractCLibBmiAdapter.hpp"
#include "Bmi_Fortran_Common.h"
#include "bmi.h"

// Forward declaration to provide access to protected items in testing
class Bmi_Fortran_Adapter_Test;

namespace models {
    namespace bmi {
        
        typedef struct Bmi_Fortran_Handle_Wrapper {
            void *handle;
        } Bmi_Fortran_Handle_Wrapper;

        /**
         * An adapter class to serve as a C++ interface to the essential aspects of external models written in the
         * Fortran language that implement the BMI.
         */
        class Bmi_Fortran_Adapter : public AbstractCLibBmiAdapter<Bmi_Fortran_Handle_Wrapper> {

        public:

            explicit Bmi_Fortran_Adapter(const string &type_name, std::string library_file_path,
                                         std::string forcing_file_path,
                                         bool allow_exceed_end, bool has_fixed_time_step,
                                         const std::string &registration_func, utils::StreamHandler output)
                    : Bmi_Fortran_Adapter(type_name, library_file_path, "", forcing_file_path, allow_exceed_end,
                                          has_fixed_time_step,
                                          registration_func, output) {}

            Bmi_Fortran_Adapter(const string &type_name, std::string library_file_path, std::string bmi_init_config,
                                std::string forcing_file_path, bool allow_exceed_end, bool has_fixed_time_step,
                                std::string registration_func,
                                utils::StreamHandler output) : AbstractCLibBmiAdapter(type_name,
                                                                                      library_file_path,
                                                                                      bmi_init_config,
                                                                                      forcing_file_path,
                                                                                      allow_exceed_end,
                                                                                      has_fixed_time_step,
                                                                                      registration_func,
                                                                                      output) {
                try {
                    construct_and_init_backing_model_for_fortran();
                    // Make sure this is set to 'true' after this function call finishes
                    model_initialized = true;
                    acquire_time_conversion_factor(bmi_model_time_convert_factor);
                }
                    // Record the exception message before re-throwing to handle subsequent function calls properly
                catch (exception &e) {
                    // Make sure this is set to 'true' after this function call finishes
                    model_initialized = true;
                    throw e;
                }
            }

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
             * The number of BMI input variables the model makes available.
             *
             * @return The number of BMI input variables the model makes available.
             * @see inner_get_input_item_count
             */
            int GetInputItemCount() override {
                return inner_get_input_item_count();
            }

            /**
             * Get input variable names for the backing model.
             *
             * @return A vector of BMI input variable names for the backing model.
             */
            std::vector<std::string> GetInputVarNames() override;

            /**
             * The number of BMI output variables the model makes available.
             *
             * @return The number of BMI output variables the model makes available.
             * @see inner_get_output_item_count
             */
            int GetOutputItemCount() override {
                return inner_get_output_item_count();
            }

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
            double GetTimeStep() override;

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
                inner_get_value(name, dest);
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

                inner_get_value(name, dest);

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
                throw std::runtime_error(model_name + " cannot currently get pointers for Fortran-based BMI modules.");
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
                if (get_var_nbytes(bmi_model->handle, name.c_str(), &nbytes) != BMI_SUCCESS)
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
                construct_and_init_backing_model_for_fortran();
            }

        private:

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
            inline void construct_and_init_backing_model_for_fortran() {
                if (model_initialized)
                    return;
                dynamic_library_load();
                void *handle = execModuleRegistration();
                bmi_model->handle = handle;
                int init_result = initialize(bmi_model->handle, bmi_init_config.c_str());
                if (init_result != BMI_SUCCESS) {
                    init_exception_msg = "Failure when attempting to initialize " + model_name;
                    throw models::external::State_Exception(init_exception_msg);
                }
            }

            /**
             * Internal implementation of logic used for @see GetInputItemCount.
             *
             * "Inner" functions such as this should not contain nested function calls to any other member functions for
             * the type.
             *
             * @return The count of input BMI variables.
             * @see GetInputItemCount
             */
            inline int inner_get_input_item_count() {
                int item_count;
                if (get_input_item_count(bmi_model->handle, &item_count) != BMI_SUCCESS) {
                    throw std::runtime_error(model_name + " failed to get model input item count.");
                }
                return item_count;
            }

            /**
             * Internal implementation of logic used for @see GetOutputItemCount.
             *
             * "Inner" functions such as this should not contain nested function calls to any other member functions for
             * the type.
             *
             * @return The count of output BMI variables.
             * @see GetOutputItemCount
             */
            inline int inner_get_output_item_count() {
                int item_count;
                if (get_output_item_count(bmi_model->handle, &item_count) != BMI_SUCCESS) {
                    throw std::runtime_error(model_name + " failed to get model output item count.");
                }
                return item_count;
            }

            /**
             * Internal implementation of logic used for @see GetValue.
             *
             * "Inner" functions such as this should not contain nested function calls to any other member functions for
             * the type.
             *
             * Essentially, function exists as inner implementation.  This allows it to be inlined, which may lead to
             * optimization in certain situations.
             *
             * @param name The name of the variable for which to get values.
             * @param dest A function pointer in which to return the values.
             */
            inline void inner_get_value(const std::string& name, void *dest) {
                if (get_value(bmi_model->handle, name.c_str(), dest) != BMI_SUCCESS) {
                    throw std::runtime_error(model_name + " failed to get values for variable " + name + ".");
                }
            }

            /**
             * Build a vector of the input or output variable names string, and return a pointer to it.
             *
             * "Inner" functions such as this should not contain nested function calls to any other member functions for
             * the type.
             *
             * This should be used for @see GetInputVarNames and @see GetOutputVarNames.
             *
             * @param is_input_variables Whether input variable names should be retrieved.
             * @return A shared pointer to a vector of input or output variable name strings.
             * @see GetInputVarNames
             * @see GetOutputVarNames
             */
            std::shared_ptr<std::vector<std::string>> inner_get_variable_names(bool is_input_variables) {
                // Writing this def once here at beginning: fewer lines, but this may or may not be used.
                std::string varType = (is_input_variables) ? "input" : "output";

                // Obtain this via inner functions, which should use the model directly and not other member functions
                int variableCount;
                try {
                    variableCount = (is_input_variables) ? inner_get_input_item_count() : inner_get_output_item_count();
                }
                catch (const std::exception &e) {
                    throw std::runtime_error(model_name + " failed to count of " + varType + " variable names array.");
                }

                // With variable count now obtained, create the vector
                std::shared_ptr<std::vector<std::string>> var_names = std::make_shared<std::vector<std::string>>(
                        std::vector<std::string>(variableCount));

                // Must get the names from the model as an array of C strings
                // The array can be on the stack ...
                char* names_array[variableCount];
                // ... but allocate the space for the individual C strings (i.e., the char * elements)
                for (int i = 0; i < variableCount; i++) {
                    names_array[i] = static_cast<char *>(malloc(sizeof(char) * BMI_MAX_VAR_NAME));
                }

                // With the necessary char** in hand, get the names from the model
                int names_result;
                if (is_input_variables) {
                    names_result = get_input_var_names(bmi_model.get(), names_array);
                }
                else {
                    names_result = get_output_var_names(bmi_model.get(), names_array);
                }
                if (names_result != BMI_SUCCESS) {
                    throw std::runtime_error(model_name + " failed to get array of " + varType + " variables names.");
                }

                // Then convert from array of C strings to vector of strings, freeing the allocated space as we go
                for (int i = 0; i < variableCount; ++i) {
                    (*var_names)[i] = std::string(names_array[i]);
                    free(names_array[i]);
                }

                return var_names;
            }
        };
    }
}

#endif // ACTIVATE_FORTRAN

#endif //NGEN_BMI_FORTRAN_ADAPTER_HPP
