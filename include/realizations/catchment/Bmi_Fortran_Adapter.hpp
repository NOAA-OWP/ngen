#ifndef NGEN_BMI_FORTRAN_ADAPTER_HPP
#define NGEN_BMI_FORTRAN_ADAPTER_HPP

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

            void *GetValuePtr(std::string name) override {
                throw std::runtime_error(model_name + " cannot currently get pointers for Fortran-based BMI modules.");
            }

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

#endif //NGEN_BMI_FORTRAN_ADAPTER_HPP
