#ifndef NGEN_BMI_FORTRAN_ADAPTER_HPP
#define NGEN_BMI_FORTRAN_ADAPTER_HPP

#include <NGenConfig.h>

#if NGEN_WITH_BMI_FORTRAN

#include "AbstractCLibBmiAdapter.hpp"
#include "Bmi_Fortran_Common.h"
#include "bmi.h"
#include "State_Exception.hpp"
#include "utilities/ExternalIntegrationException.hpp"
#include "utilities/bmi_utilities.hpp"

#include <stdexcept>

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
        class Bmi_Fortran_Adapter final : public AbstractCLibBmiAdapter {

        public:

            explicit Bmi_Fortran_Adapter(const std::string &type_name, std::string library_file_path,
                                         bool has_fixed_time_step,
                                         const std::string &registration_func)
                    : Bmi_Fortran_Adapter(type_name, library_file_path, "",
                                          has_fixed_time_step,
                                          registration_func) {}

            Bmi_Fortran_Adapter(const std::string &type_name, std::string library_file_path, std::string bmi_init_config,
                                bool has_fixed_time_step,
                                std::string registration_func) : AbstractCLibBmiAdapter(type_name,
                                                                                      library_file_path,
                                                                                      bmi_init_config,
                                                                                      has_fixed_time_step,
                                                                                      registration_func
                                                                                      ) {
                try {
                    construct_and_init_backing_model_for_fortran();
                    // Make sure this is set to 'true' after this function call finishes
                    model_initialized = true;
                    bmi_model_time_convert_factor = get_time_convert_factor();
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
                catch (std::exception &e) {
                    //This will catch any other exception, but the it will be cast to this base type.
                    //This means it looses it any specific type/message information.  So if construct_and_init_backing_model_for_fortran
                    //throws an exception besides runtime_error, catch that type explicitly.
                    model_initialized = true;
                    throw e;
                }
            }

            Bmi_Fortran_Adapter(Bmi_Fortran_Adapter const&) = delete;
            Bmi_Fortran_Adapter(Bmi_Fortran_Adapter&&) = delete;

            std::string GetComponentName() override;

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
             * The Fortran module's getter is chosen by the variable's width as well as its type name, since the
             * latter alone does not determine the former in Fortran (@see get_primitive_type).
             *
             * @param name
             * @param dest
             * @see get_value
             */
            void GetValue(std::string name, void *dest) override;

            /**
             * Get the values of a variable at the given indices.
             *
             * The ISO C shim has no access by index, so this fetches the whole variable through @ref GetValue
             * and copies the requested items out of it.
             *
             * @param name The name of the variable.
             * @param dest Where to write the @p count items, each of the variable's item size.
             * @param inds The indices of the items to get.
             * @param count The number of indices in @p inds.
             * @throw std::out_of_range If an index is outside the variable.
             */
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
             * Resolve a Fortran type name to a primitive type, by way of the item size.
             *
             * A Fortran type name does not fix a width.  Compiler flags such as gfortran's ``-fdefault-real-8`` and
             * ``-fdefault-integer-8`` change what ``real`` and ``integer`` are, and a ``kind`` parameter changes it
             * per variable, so a module may well report ``real`` for 8-byte items.  Taken at its word, that name
             * would be addressed with a 4-byte stride.
             *
             * So the name settles only whether the variable is integral or floating point, and the item size
             * settles which type of that kind.  The C spellings modules commonly report are treated the same way.
             *
             * The exception is a ``double precision`` or ``double`` narrower than 8 bytes, which is not taken for a
             * ``f32``.  gfortran's ``-freal-8-real-4`` can make one, but it is far likelier a module misreporting
             * its type or its size, as test_bmi_fortran once did for its grid origins.  Resolved as named, to
             * ``f64``, it is refused by the item size check with a message describing the mismatch, rather than by
             * an accessor the module does not serve for that name.
             *
             * Every width with a primitive type is mapped, whether or not the ISO C shim currently has an accessor
             * for it; which of them can actually be read or written is the business of @ref GetValue and
             * @ref SetValue.  A 16-byte real is ``f128`` only where ``long double`` is IEEE binary128: on x86 that
             * size is also gfortran's 80-bit ``real(kind=10)`` with padding, so the size cannot tell the two apart.
             *
             * A module can resolve that ambiguity itself by reporting ``long double`` for a variable of kind
             * ``c_long_double``.  That name is resolved as named, to ``long_double``, the platform's C++ ``long
             * double``, whatever else shares its size.
             *
             * Any other name, or a size with no type of the named kind, is resolved as named, for the item size
             * check to refuse.
             *
             * @param external_type_name The string name of some type, as reported by the Fortran module.
             * @param item_size The size in bytes of one item, as reported by the Fortran module.
             * @return The primitive type, which is @p item_size bytes wide where the name has one that wide.
             * @throw models::bmi::unknown_bmi_type If the name is not one models::bmi::parse_bmi_type knows.
             */
            models::bmi::primitive_type
            get_primitive_type(const std::string &external_type_name, const size_t item_size) override {
                using models::bmi::primitive_type;
                const bool is_integer = external_type_name == "integer" || external_type_name == "int";
                const bool is_single = external_type_name == "real" || external_type_name == "float";
                const bool is_double = external_type_name == "double precision" || external_type_name == "double";

                if (is_integer) {
                    switch (item_size) {
                        case 1: return primitive_type::i8;
                        case 2: return primitive_type::i16;
                        case 4: return primitive_type::i32;
                        case 8: return primitive_type::i64;
                    }
                }
                if (is_single || is_double) {
                    if (is_single && item_size == 4) {return primitive_type::f32;}
                    if (item_size == 8) {return primitive_type::f64;}
                    if (item_size == 16 && models::bmi::long_double_is_float128) {return primitive_type::f128;}
                }
                return models::bmi::parse_bmi_type(external_type_name);
            }

            /**
             * Whether the backing model has been initialized yet.
             *
             * @return Whether the backing model has been initialized yet.
             */
            bool is_model_initialized() override;

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
                            std::to_string(src.size()) + " (expected size " + std::to_string(total_bytes / item_size) +
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
            void construct_and_init_backing_model_for_fortran();

            /**
             * Internal implementation of logic used for @see GetInputItemCount.
             *
             * "Inner" functions such as this should not contain nested function calls to any non-inner member functions
             * for the type.
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
             * "Inner" functions such as this should not contain nested function calls to any non-inner member functions
             * for the type.
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
             * Internal implementation of logic used for @see GetValue for ints.
             *
             * The Fortran implementation has separate getters/setters for different variable types, which is mirrored
             * in the corresponding inner implementations here, as overloads on the pointer type.  @see GetValue
             * calls whichever overload takes the variable's resolved type, so supporting a further width from the
             * ISO C shim needs only a further overload.
             *
             * "Inner" functions such as this should not contain nested function calls to any non-inner member functions
             * for the type.
             *
             * @param name The name of the variable for which to get values.
             * @param dest An int pointer in which to return the values.
             */
            inline void inner_get_value(const std::string& name, int *dest) {
                if (get_value_int(&bmi_model->handle, name.c_str(), dest) != BMI_SUCCESS) {
                    throw std::runtime_error(model_name + " failed to get values for variable " + name + ".");
                }
            }

            /**
             * Internal implementation of logic used for @see GetValue for floats.
             *
             * The Fortran implementation has separate getters/setters for different variable types, which is mirrored
             * in the corresponding inner implementations here, as overloads on the pointer type.  @see GetValue
             * calls whichever overload takes the variable's resolved type, so supporting a further width from the
             * ISO C shim needs only a further overload.
             *
             * "Inner" functions such as this should not contain nested function calls to any non-inner member functions
             * for the type.
             *
             * @param name The name of the variable for which to get values.
             * @param dest A float pointer in which to return the values.
             */
            inline void inner_get_value(const std::string& name, float *dest) {
                if (get_value_float(&bmi_model->handle, name.c_str(), dest) != BMI_SUCCESS) {
                    throw std::runtime_error(model_name + " failed to get values for variable " + name + ".");
                }
            }

            /**
             * Internal implementation of logic used for @see GetValue for doubles.
             *
             * The Fortran implementation has separate getters/setters for different variable types, which is mirrored
             * in the corresponding inner implementations here, as overloads on the pointer type.  @see GetValue
             * calls whichever overload takes the variable's resolved type, so supporting a further width from the
             * ISO C shim needs only a further overload.
             *
             * "Inner" functions such as this should not contain nested function calls to any non-inner member functions
             * for the type.
             *
             * @param name The name of the variable for which to get values.
             * @param dest A double pointer in which to return the values.
             */
            inline void inner_get_value(const std::string& name, double *dest) {
                if (get_value_double(&bmi_model->handle, name.c_str(), dest) != BMI_SUCCESS) {
                    throw std::runtime_error(model_name + " failed to get values for variable " + name + ".");
                }
            }

            /**
             * Build a vector of the input or output variable names string, and return a pointer to it.
             *
             * "Inner" functions such as this should not contain nested function calls to any non-inner member functions
             * for the type.
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
                std::vector<char*> names_array(variableCount);
                // ... but allocate the space for the individual C strings (i.e., the char * elements)
                for (int i = 0; i < variableCount; i++) {
                    names_array[i] = static_cast<char *>(malloc(sizeof(char) * BMI_MAX_VAR_NAME));
                }

                // With the necessary char** in hand, get the names from the model
                int names_result;
                if (is_input_variables) {
                    names_result = get_input_var_names(bmi_model.get(), names_array.data());
                }
                else {
                    names_result = get_output_var_names(bmi_model.get(), names_array.data());
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
             * Internal implementation of logic used for @see SetValue for ints.
             *
             * The Fortran implementation has separate getters/setters for different variable types, which is mirrored
             * in the corresponding inner implementations here, as overloads on the pointer type.  @see SetValue
             * calls whichever overload takes the variable's resolved type, so supporting a further width from the
             * ISO C shim needs only a further overload.
             *
             * "Inner" functions such as this should not contain nested function calls to any non-inner member functions
             * for the type.
             *
             * @param name The name of the variable for which to get values.
             * @param src An int pointer that should be passed to the analogous BMI setter of the Fortran module.
             */
            inline void inner_set_value(const std::string& name, int *src) {
                if (set_value_int(&bmi_model->handle, name.c_str(), src) != BMI_SUCCESS) {
                    throw models::external::State_Exception("Failed to set values of " + name + " int variable for " + model_name);
                }
            }

            /**
             * Internal implementation of logic used for @see SetValue for floats.
             *
             * The Fortran implementation has separate getters/setters for different variable types, which is mirrored
             * in the corresponding inner implementations here, as overloads on the pointer type.  @see SetValue
             * calls whichever overload takes the variable's resolved type, so supporting a further width from the
             * ISO C shim needs only a further overload.
             *
             * "Inner" functions such as this should not contain nested function calls to any non-inner member functions
             * for the type.
             *
             * @param name The name of the variable for which to get values.
             * @param src A float pointer that should be passed to the analogous BMI setter of the Fortran module.
             */
            inline void inner_set_value(const std::string& name, float *src) {
                if (set_value_float(&bmi_model->handle, name.c_str(), src) != BMI_SUCCESS) {
                    throw models::external::State_Exception("Failed to set values of " + name + " float variable for " + model_name);
                }
            }

            /**
             * Internal implementation of logic used for @see SetValue for doubles.
             *
             * The Fortran implementation has separate getters/setters for different variable types, which is mirrored
             * in the corresponding inner implementations here, as overloads on the pointer type.  @see SetValue
             * calls whichever overload takes the variable's resolved type, so supporting a further width from the
             * ISO C shim needs only a further overload.
             *
             * "Inner" functions such as this should not contain nested function calls to any non-inner member functions
             * for the type.
             *
             * @param name The name of the variable for which to get values.
             * @param src A double pointer that should be passed to the analogous BMI setter of the Fortran module.
             */
            inline void inner_set_value(const std::string& name, double *src) {
                if (set_value_double(&bmi_model->handle, name.c_str(), src) != BMI_SUCCESS) {
                    throw models::external::State_Exception("Failed to set values of " + name + " double variable for " + model_name);
                }
            }

            /**
             * Get the primitive type of a variable's values, checked against the item size the module reports.
             *
             * @param name The name of the variable.
             * @return The variable's primitive type, which is as wide as its items.
             * @throw ::external::ExternalIntegrationException If no C++ type corresponds to the variable's type name.
             * @throw std::runtime_error If the resolved type is not as wide as the module says the items are.
             * @see models::bmi::resolve_var_type
             */
            models::bmi::primitive_type get_value_primitive_type(const std::string &name);

            /**
             * Get a variable's values into @p dest, as items of @tparam T, through the ISO C shim's getter for T.
             *
             * The variable's type has to be resolved already, to @p type, of which T is the C++ type.
             *
             * @param name The name of the variable.
             * @param dest Where to write the variable's values.
             * @param type The variable's primitive type, for the message if the shim has no getter for it.
             * @throw ::external::ExternalIntegrationException If the ISO C shim has no getter for T.
             */
            template <typename T>
            void get_value_as(const std::string &name, T *dest, models::bmi::primitive_type type);

            friend class ::Bmi_Fortran_Adapter_Test;

        private:
            /** Pointer to backing BMI model instance. */
            std::unique_ptr<Bmi_Fortran_Handle_Wrapper> bmi_model = nullptr;

        };
    }
}

#endif // NGEN_WITH_BMI_FORTRAN

#endif //NGEN_BMI_FORTRAN_ADAPTER_HPP
