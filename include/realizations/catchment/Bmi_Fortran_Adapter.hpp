#ifndef NGEN_BMI_FORTRAN_ADAPTER_HPP
#define NGEN_BMI_FORTRAN_ADAPTER_HPP

#include "Bmi_C_Adapter.hpp"

// Forward declaration to provide access to protected items in testing
class Bmi_Fortran_Adapter_Test;

namespace models {
    namespace bmi {
        /**
         * An adapter class to serve as a C++ interface to the essential aspects of external models written in the
         * Fortran language that implement the BMI.
         */
        class Bmi_Fortran_Adapter : public Bmi_C_Adapter {

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
                                utils::StreamHandler output) : Bmi_C_Adapter(type_name,
                                                                             library_file_path,
                                                                             bmi_init_config,
                                                                             forcing_file_path,
                                                                             allow_exceed_end,
                                                                             has_fixed_time_step,
                                                                             registration_func,
                                                                             output,
                                                                             false)
            {
                try {
                    construct_and_init_backing_model_for_fortran();
                    // Make sure this is set to 'true' after this function call finishes
                    model_initialized = true;
                    acquire_time_conversion_factor(bmi_model_time_convert_factor);
                }
                // Record the exception message before re-throwing to handle subsequent function calls properly
                catch (exception& e) {
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
                bmi_model = std::make_shared<C_Bmi>(C_Bmi());
                // TODO: make sure there are tests in place to make sure it is caught if this usage isn't safe, now or in future
                dynamic_library_load();
                // Set the individual struct function pointers to the Fortran C-binding proxy functions
                load_proxy_function_pointers();
                int init_result = bmi_model->initialize(bmi_model.get(), bmi_init_config.c_str());
                if (init_result != BMI_SUCCESS) {
                    init_exception_msg = "Failure when attempting to initialize " + model_name;
                    throw models::external::State_Exception(init_exception_msg);
                }
            }

            /**
             * Load function pointers for ISO C binding proxy function via the dynamic symbol lookups.
             *
             * Function assumes symbol for function is equivalent to the "standard" BMI function name.
             */
            inline void load_proxy_function_pointers() {
                if (model_initialized)
                    return;
                if (bmi_model == nullptr) {
                    init_exception_msg = "Can't load Fortran proxy function symbols before init of BMI struct pointer.";
                    throw ::external::ExternalIntegrationException(init_exception_msg);
                }

                bmi_model->initialize = (int (*)(struct ::Bmi *, const char *)) dynamic_load_symbol("initialize");
                bmi_model->update = (int (*)(struct ::Bmi *)) dynamic_load_symbol("update");
                bmi_model->update_until = (int (*)(struct ::Bmi *, double)) dynamic_load_symbol("update_until");
                bmi_model->finalize = (int (*)(struct ::Bmi *)) dynamic_load_symbol("finalize");

                bmi_model->get_component_name = (int (*)(struct ::Bmi *, char *)) dynamic_load_symbol(
                        "get_component_name");
                bmi_model->get_input_item_count = (int (*)(struct ::Bmi *, int *)) dynamic_load_symbol(
                        "get_input_item_count");
                bmi_model->get_output_item_count = (int (*)(struct ::Bmi *, int *)) dynamic_load_symbol(
                        "get_output_item_count");
                bmi_model->get_input_var_names = (int (*)(struct ::Bmi *, char **)) dynamic_load_symbol(
                        "get_input_var_names");
                bmi_model->get_output_var_names = (int (*)(struct ::Bmi *, char **)) dynamic_load_symbol(
                        "get_output_var_names");

                bmi_model->get_var_grid = (int (*)(struct ::Bmi *, const char *, int *)) dynamic_load_symbol(
                        "get_var_grid");
                bmi_model->get_var_type = (int (*)(struct ::Bmi *, const char *, char *)) dynamic_load_symbol(
                        "get_var_type");
                bmi_model->get_var_units = (int (*)(struct ::Bmi *, const char *, char *)) dynamic_load_symbol(
                        "get_var_units");
                bmi_model->get_var_itemsize = (int (*)(struct ::Bmi *, const char *, int *)) dynamic_load_symbol(
                        "get_var_itemsize");
                bmi_model->get_var_nbytes = (int (*)(struct ::Bmi *, const char *, int *)) dynamic_load_symbol(
                        "get_var_nbytes");
                bmi_model->get_var_location = (int (*)(struct ::Bmi *, const char *, char *)) dynamic_load_symbol(
                        "get_var_location");

                bmi_model->get_current_time = (int (*)(struct ::Bmi *, double *)) dynamic_load_symbol(
                        "get_current_time");
                bmi_model->get_start_time = (int (*)(struct ::Bmi *, double *)) dynamic_load_symbol("get_start_time");
                bmi_model->get_end_time = (int (*)(struct ::Bmi *, double *)) dynamic_load_symbol("get_end_time");
                bmi_model->get_time_units = (int (*)(struct ::Bmi *, char *)) dynamic_load_symbol("get_time_units");
                bmi_model->get_time_step = (int (*)(struct ::Bmi *, double *)) dynamic_load_symbol("get_time_step");

                bmi_model->get_value = (int (*)(struct ::Bmi *, const char *, void *)) dynamic_load_symbol("get_value");
                bmi_model->get_value_ptr = (int (*)(struct ::Bmi *, const char *, void **)) dynamic_load_symbol(
                        "get_value_ptr");
                bmi_model->get_value_at_indices = (int (*)(struct ::Bmi *, const char *, void *, int *,
                                                           int)) dynamic_load_symbol("get_value_at_indices");

                bmi_model->set_value = (int (*)(struct ::Bmi *, const char *, void *)) dynamic_load_symbol("set_value");
                bmi_model->set_value_at_indices = (int (*)(struct ::Bmi *, const char *, int *, int,
                                                           void *)) dynamic_load_symbol("set_value_at_indices");

                bmi_model->get_grid_size = (int (*)(struct ::Bmi *, int, int *)) dynamic_load_symbol("get_grid_size");
                bmi_model->get_grid_rank = (int (*)(struct ::Bmi *, int, int *)) dynamic_load_symbol("get_grid_rank");
                bmi_model->get_grid_type = (int (*)(struct ::Bmi *, int, char *)) dynamic_load_symbol("get_grid_type");

                bmi_model->get_grid_shape = (int (*)(struct ::Bmi *, int, int *)) dynamic_load_symbol("get_grid_shape");
                bmi_model->get_grid_spacing = (int (*)(struct ::Bmi *, int, double *)) dynamic_load_symbol(
                        "get_grid_spacing");
                bmi_model->get_grid_origin = (int (*)(struct ::Bmi *, int, double *)) dynamic_load_symbol(
                        "get_grid_origin");

                bmi_model->get_grid_x = (int (*)(struct ::Bmi *, int, double *)) dynamic_load_symbol("get_grid_x");
                bmi_model->get_grid_y = (int (*)(struct ::Bmi *, int, double *)) dynamic_load_symbol("get_grid_y");
                bmi_model->get_grid_z = (int (*)(struct ::Bmi *, int, double *)) dynamic_load_symbol("get_grid_z");

                bmi_model->get_grid_node_count = (int (*)(struct ::Bmi *, int, int *)) dynamic_load_symbol(
                        "get_grid_node_count");
                bmi_model->get_grid_edge_count = (int (*)(struct ::Bmi *, int, int *)) dynamic_load_symbol(
                        "get_grid_edge_count");
                bmi_model->get_grid_face_count = (int (*)(struct ::Bmi *, int, int *)) dynamic_load_symbol(
                        "get_grid_face_count");
                bmi_model->get_grid_edge_nodes = (int (*)(struct ::Bmi *, int, int *)) dynamic_load_symbol(
                        "get_grid_edge_nodes");
                bmi_model->get_grid_face_edges = (int (*)(struct ::Bmi *, int, int *)) dynamic_load_symbol(
                        "get_grid_face_edges");
                bmi_model->get_grid_face_nodes = (int (*)(struct ::Bmi *, int, int *)) dynamic_load_symbol(
                        "get_grid_face_nodes");
                bmi_model->get_grid_nodes_per_face = (int (*)(struct ::Bmi *, int, int *)) dynamic_load_symbol(
                        "get_grid_nodes_per_face");
            }

        };

    }
}

#endif //NGEN_BMI_FORTRAN_ADAPTER_HPP
