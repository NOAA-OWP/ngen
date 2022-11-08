#ifndef NGEN_BMI_FORTRAN_ADAPTER_HPP
#define NGEN_BMI_FORTRAN_ADAPTER_HPP

#ifdef NGEN_BMI_FORTRAN_ACTIVE

#include "AbstractCLibBmiAdapter.hpp"
#include "Bmi_Fortran_Common.h"
#include "bmi.h"

// Forward declaration to provide access to protected items in testing
class Bmi_Fortran_Adapter_Test;

namespace models {
    namespace bmi {
        
        typedef struct Bmi_Fortran_Handle_Wrapper {
            void ** handle;
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
                catch( models::external::State_Exception& e)
                {
                    model_initialized = true;
                    throw e;
                }
                catch( ::external::ExternalIntegrationException& e)
                {
                    model_initialized = true;
                    throw e;
                }
                catch (std::runtime_error &e) {
                    //Catch `runtime_error`s that may possible be thrown from acquire_time_conversion_factor
                    // Make sure this is set to 'true' after this function call finishes
                    model_initialized = true;
                    throw e;
                }
                catch (exception &e) {
                    //This will catch any other exception, but the it will be cast to this base type.
                    //This means it looses it any specific type/message information.  So if construct_and_init_backing_model_for_fortran
                    //throws an exception besides runtime_error, catch that type explicitly.
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
                if (get_var_nbytes(&bmi_model->handle, name.c_str(), &nbytes) != BMI_SUCCESS)
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
             * Get the name string for the C++ type analogous to the described type in the Fortran backing model.
             *
             * The supported languages for BMI modules support different types than C++ in general, but it is necessary
             * to map "external" types to the appropriate C++ type for certain interactions; e.g., setting a variable
             * value.  This function encapsulates the translation specific to Fortran.
             *
             * Note that the size of an individual item is also required, as this can vary in certain situations
             * depending on the implementation language of a backing model.
             *
             * @param external_type_name The string name of some Fortran type.
             * @param item_size The particular size in bytes for items of the involved analogous types.
             * @return The name string for the C++ type analogous to the described type in the Fortran backing model.
             * @throw std::runtime_error If an unrecognized external type name parameter is provided.
             */
            const std::string
            get_analogous_cxx_type(const std::string &external_type_name, const size_t item_size) override {
                if (external_type_name == "int" || external_type_name == "integer") {
                    return "int";
                }
                else if (external_type_name == "float" || external_type_name == "real") {
                    return "float";
                }
                else if (external_type_name == "double" || external_type_name == "double precision") {
                    return "double";
                }
                else {
                    throw std::runtime_error(
                            "Bmi_Fortran_Adapter received unrecognized Fortran type name '" + external_type_name +
                            "' for which the analogous C++ type could not be determined");
                }
            }

            /**
             * Whether the backing model has been initialized yet.
             *
             * @return Whether the backing model has been initialized yet.
             */
            bool is_model_initialized();

            void SetValue(std::string name, void *src) override;

            /**
             * Set the given BMI input variable, sourcing values from the given vector.
             *
             * Note that because of how BMI works, only the size of the individual items must agree, not necessarily
             * their types.  However, non-matching types will likely produce strange results.
             *
             * @tparam T The type for the vector, which must be of the same size as the type for the variable.
             * @param name The name of the BMI variable for which to set values.
             * @param src The vector of source values, whose length must match the backing BMI variable array.
             */
            template<class T>
            void SetValue(std::string name, std::vector<T> src) {
                int item_size, total_bytes;
                try {
                    item_size = GetVarItemsize(name);
                    total_bytes = GetVarNbytes(name);
                }
                catch (std::runtime_error &e) {
                    throw std::runtime_error("Cannot set " + name + " variable of " + model_name +
                                             "; unable to test item and array sizes are equal (does variable " + name +
                                             " exist for model " + model_name + "?)");
                }
                if (item_size != sizeof(src[0])) {
                    throw std::runtime_error("Cannot set " + name + " variable of " + model_name +
                                             " with values of different item size");
                }
                if (src.size() != total_bytes / item_size) {
                    throw std::runtime_error(
                            "Cannot set " + name + " variable of " + model_name + " from vector of size " +
                            std::to_string(src.size) + " (expected size " + std::to_string(total_bytes / item_size) +
                            ")");
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

            /**
             * Load and execute the "registration" function for the backing BMI module.
             *
             * Integrated BMI module libraries are expected to provide an additional ``register_bmi`` function. This
             * initializes the Fortran BMI object and passes the opaque @see Bmi_Fortran_Handle_Wrapper::handle from
             * this instance's @see bmi_model struct. The handle is then set to point to the Fortran BMI object, and it
             * can then be passed to the free proxy functions from the internal/common ngen Fortran integration library.
             */
            inline void execModuleRegistration() {
                if (get_dyn_lib_handle() == nullptr) {
                    dynamic_library_load();
                }
                void *symbol;
                // TODO: verify this doesn't need to just return void, rather than void*
                void *(*dynamic_register_bmi)(void *model);

                try {
                    // Acquire the BMI struct func pointer registration function
                    symbol = dynamic_load_symbol(get_bmi_registration_function());
                    // TODO: as with the TODO above, verify this doesn't need to just return void, rather than void*
                    dynamic_register_bmi = (void *(*)(void *)) symbol;
                    // Call registration function, which for Fortran sets up the Fortran BMI object and sets the passed
                    // opaque handle param to point to the Fortran BMI object.
                    dynamic_register_bmi(&bmi_model->handle);
                }
                catch (const ::external::ExternalIntegrationException &e) {
                    // "Override" the default message in this case
                    this->init_exception_msg =
                            "Cannot init " + this->model_name + " without valid library registration function: " +
                            this->init_exception_msg;
                    throw ::external::ExternalIntegrationException(this->init_exception_msg);
                }
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
                bmi_model = std::make_shared<Bmi_Fortran_Handle_Wrapper>(Bmi_Fortran_Handle_Wrapper());
                dynamic_library_load();
                execModuleRegistration();
                int init_result = initialize(&bmi_model->handle, bmi_init_config.c_str());
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
                if (get_input_item_count(&bmi_model->handle, &item_count) != BMI_SUCCESS) {
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
                if (get_output_item_count(&bmi_model->handle, &item_count) != BMI_SUCCESS) {
                    throw std::runtime_error(model_name + " failed to get model output item count.");
                }
                return item_count;
            }

            inline std::string inner_get_var_type(const std::string &name) {
                char type_c_str[BMI_MAX_TYPE_NAME];
                if (get_var_type(&bmi_model->handle, name.c_str(), type_c_str) != BMI_SUCCESS) {
                    throw std::runtime_error(model_name + " failed to get variable type for " + name + ".");
                }
                return {type_c_str};
            }

            /**
             * Internal implementation of logic used for @see GetValue.
             *
             * The Fortran implementation has separate getters/setters for different variable types, which is mirrored
             * in the corresponding inner implementations here.
             *
             * "Inner" functions such as this should not contain nested function calls to any other member functions for
             * the type.
             *
             * Essentially, function exists as inner implementation.  This allows it to be inlined, which may lead to
             * optimization in certain situations.
             *
             * @param name The name of the variable for which to get values.
             * @param dest A float pointer in which to return the values.
             */
            inline void inner_get_value(const std::string& name, void *dest) {
                std::string varType = inner_get_var_type(name);
                //Can use the C type or the fortran type, e.g. int or integer
                if (varType == "int" || varType == "integer") {
                    inner_get_value_int(name, (int *)dest);
                }
                else if (varType == "float" || varType == "real") {
                    inner_get_value_float(name, (float *)dest);
                }
                else if (varType == "double" || varType == "double precision") {
                    inner_get_value_double(name, (double *)dest);
                }
                else {
                    throw ::external::ExternalIntegrationException(
                            "Can't get model " + model_name + " variable " + name + " of type '" + varType + ".");
                }
            }

            /**
             * Internal implementation of logic used for @see GetValue for ints.
             *
             * The Fortran implementation has separate getters/setters for different variable types, which is mirrored
             * in the corresponding inner implementations here.
             *
             * "Inner" functions such as this should not contain nested function calls to any other member functions for
             * the type.
             *
             * Essentially, function exists as inner implementation.  This allows it to be inlined, which may lead to
             * optimization in certain situations.
             *
             * @param name The name of the variable for which to get values.
             * @param dest An int pointer in which to return the values.
             */
            inline void inner_get_value_int(const std::string& name, int *dest) {
                if (get_value_int(&bmi_model->handle, name.c_str(), dest) != BMI_SUCCESS) {
                    throw std::runtime_error(model_name + " failed to get values for variable " + name + ".");
                }
            }

            /**
             * Internal implementation of logic used for @see GetValue for floats.
             *
             * The Fortran implementation has separate getters/setters for different variable types, which is mirrored
             * in the corresponding inner implementations here.
             *
             * "Inner" functions such as this should not contain nested function calls to any other member functions for
             * the type.
             *
             * Essentially, function exists as inner implementation.  This allows it to be inlined, which may lead to
             * optimization in certain situations.
             *
             * @param name The name of the variable for which to get values.
             * @param dest A float pointer in which to return the values.
             */
            inline void inner_get_value_float(const std::string& name, float *dest) {
                if (get_value_float(&bmi_model->handle, name.c_str(), dest) != BMI_SUCCESS) {
                    throw std::runtime_error(model_name + " failed to get values for variable " + name + ".");
                }
            }

            /**
             * Internal implementation of logic used for @see GetValue for doubles.
             *
             * The Fortran implementation has separate getters/setters for different variable types, which is mirrored
             * in the corresponding inner implementations here.
             *
             * "Inner" functions such as this should not contain nested function calls to any other member functions for
             * the type.
             *
             * Essentially, function exists as inner implementation.  This allows it to be inlined, which may lead to
             * optimization in certain situations.
             *
             * @param name The name of the variable for which to get values.
             * @param dest A double pointer in which to return the values.
             */
            inline void inner_get_value_double(const std::string& name, double *dest) {
                if (get_value_double(&bmi_model->handle, name.c_str(), dest) != BMI_SUCCESS) {
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

            /**
             * Internal implementation of logic used for @see SetValue.
             *
             * The Fortran implementation has separate getters/setters for different variable types, which is mirrored
             * in the corresponding inner implementations here.
             *
             * "Inner" functions such as this should not contain nested function calls to any other member functions for
             * the type.
             *
             * Essentially, function exists as inner implementation.  This allows it to be inlined, which may lead to
             * optimization in certain situations.
             *
             * @param name The name of the variable for which to get values.
             * @param dest A pointer that should be passed to the analogous BMI setter of the Fortran module.
             */
            inline void inner_set_value(const std::string& name, void *src) {
                std::string varType = inner_get_var_type(name);
                //Can use the C type or the fortran type, e.g. int or integer
                if (varType == "int" || varType == "integer") {
                    inner_set_value_int(name, (int *)src);
                }
                else if (varType == "float" || varType == "real") {
                    inner_set_value_float(name, (float *)src);
                }
                else if (varType == "double" || varType == "double precision") {
                    inner_set_value_double(name, (double *)src);
                }
                else {
                    throw ::external::ExternalIntegrationException(
                            "Can't get model " + model_name + " variable " + name + " of type '" + varType + ".");
                }
            }

            /**
             * Internal implementation of logic used for @see SetValue for ints.
             *
             * The Fortran implementation has separate getters/setters for different variable types, which is mirrored
             * in the corresponding inner implementations here.
             *
             * "Inner" functions such as this should not contain nested function calls to any other member functions for
             * the type.
             *
             * Essentially, function exists as inner implementation.  This allows it to be inlined, which may lead to
             * optimization in certain situations.
             *
             * @param name The name of the variable for which to get values.
             * @param dest An int pointer that should be passed to the analogous BMI setter of the Fortran module.
             */
            inline void inner_set_value_int(const std::string& name, int *src) {
                if (set_value_int(&bmi_model->handle, name.c_str(), src) != BMI_SUCCESS) {
                    throw models::external::State_Exception("Failed to set values of " + name + " int variable for " + model_name);
                }
            }

            /**
             * Internal implementation of logic used for @see SetValue for floats.
             *
             * The Fortran implementation has separate getters/setters for different variable types, which is mirrored
             * in the corresponding inner implementations here.
             *
             * "Inner" functions such as this should not contain nested function calls to any other member functions for
             * the type.
             *
             * Essentially, function exists as inner implementation.  This allows it to be inlined, which may lead to
             * optimization in certain situations.
             *
             * @param name The name of the variable for which to get values.
             * @param dest A float pointer that should be passed to the analogous BMI setter of the Fortran module.
             */
            inline void inner_set_value_float(const std::string& name, float *src) {
                if (set_value_float(&bmi_model->handle, name.c_str(), src) != BMI_SUCCESS) {
                    throw models::external::State_Exception("Failed to set values of " + name + " float variable for " + model_name);
                }
            }

            /**
             * Internal implementation of logic used for @see SetValue for doubles.
             *
             * The Fortran implementation has separate getters/setters for different variable types, which is mirrored
             * in the corresponding inner implementations here.
             *
             * "Inner" functions such as this should not contain nested function calls to any other member functions for
             * the type.
             *
             * Essentially, function exists as inner implementation.  This allows it to be inlined, which may lead to
             * optimization in certain situations.
             *
             * @param name The name of the variable for which to get values.
             * @param dest A double pointer that should be passed to the analogous BMI setter of the Fortran module.
             */
            inline void inner_set_value_double(const std::string& name, double *src) {
                if (set_value_double(&bmi_model->handle, name.c_str(), src) != BMI_SUCCESS) {
                    throw models::external::State_Exception("Failed to set values of " + name + " double variable for " + model_name);
                }
            }

            friend class ::Bmi_Fortran_Adapter_Test;
        };
    }
}

#endif // NGEN_BMI_FORTRAN_ACTIVE

#endif //NGEN_BMI_FORTRAN_ADAPTER_HPP
