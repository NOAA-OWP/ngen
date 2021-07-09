#ifndef NGEN_BMI_PY_ADAPTER_H
#define NGEN_BMI_PY_ADAPTER_H

#ifdef ACTIVATE_PYTHON

#include <cstring>
#include <exception>
#include <memory>
#include <string>
#include "pybind11/pybind11.h"
#include "pybind11/pytypes.h"
#include "pybind11/numpy.h"
#include "JSONProperty.hpp"
#include "StreamHandler.hpp"
#include "boost/algorithm/string.hpp"
#include "Bmi_Adapter.hpp"

namespace py = pybind11;

using namespace pybind11::literals; // to bring in the `_a` literal for pybind11 keyword args functionality
using namespace std;

namespace models {
    namespace bmi {

        /**
         * An adapter class to serve as a C++ interface to the aspects of external models written in the Python
         * language that implement the BMI.
         */
        class Bmi_Py_Adapter : public Bmi_Adapter<py::object> {

        public:

            Bmi_Py_Adapter(const string &type_name, std::string bmi_init_config, bool allow_exceed_end,
                           bool has_fixed_time_step, const geojson::JSONProperty& other_input_vars,
                           utils::StreamHandler output);

            Bmi_Py_Adapter(const string &type_name, std::string  bmi_init_config, std::string forcing_file_path,
                           bool allow_exceed_end, bool has_fixed_time_step,
                           const geojson::JSONProperty& other_input_vars, utils::StreamHandler output);

            void Finalize() override {
                bmi_model->attr("finalize")();
            }

            string GetComponentName() override;

            double GetCurrentTime() override;

            double GetEndTime() override;

            int GetInputItemCount() override;

            vector<std::string> GetInputVarNames() override;

            int GetOutputItemCount() override;

            vector<std::string> GetOutputVarNames() override;

            int GetGridEdgeCount(const int grid) override {
                return py::int_(bmi_model->attr("get_grid_edge_count")(grid));
            }

            void GetGridEdgeNodes(const int grid, int *edge_nodes) override {
                // As per BMI docs:
                //  "For each edge, connectivity is given as node at edge tail, followed by node at edge head."
                //  "The total length of the array is 2 * get_grid_edge_count."
                get_and_copy_grid_array<int>("get_grid_edge_nodes", grid, edge_nodes, 2*GetGridEdgeCount(grid), "int");
            }

            int GetGridFaceCount(const int grid) override {
                return py::int_(bmi_model->attr("get_grid_face_count")(grid));
            }

            void GetGridFaceEdges(const int grid, int *face_edges) override {
                // As per BMI docs:
                //  "The length of the array returned is the sum of the values of get_grid_nodes_per_face."
                int nodes_per_face_sum = get_sum_of_grid_nodes_per_face(grid);
                get_and_copy_grid_array<int>("get_grid_face_edges", grid, face_edges, nodes_per_face_sum, "int");
            }

            void GetGridFaceNodes(const int grid, int *face_nodes) override {
                int nodes_per_face_sum = get_sum_of_grid_nodes_per_face(grid);
                get_and_copy_grid_array<int>("get_grid_face_nodes", grid, face_nodes, nodes_per_face_sum, "int");
            }

            int GetGridNodeCount(const int grid) override {
                return py::int_(bmi_model->attr("get_grid_node_count")(grid));
            }

            void GetGridNodesPerFace(const int grid, int *nodes_per_face) override {
                get_and_copy_grid_array<int>(
                        "get_grid_nodes_per_face", grid, nodes_per_face, GetGridFaceCount(grid), "int");
            }

            void GetGridOrigin(const int grid, double *origin) override {
                get_and_copy_grid_array<double>("get_grid_origin", grid, origin, GetGridRank(grid), "double");
            }

            int GetGridRank(const int grid) override {
                return py::int_(bmi_model->attr("get_grid_rank")(grid));
            }

            void GetGridShape(const int grid, int *shape) override {
                get_and_copy_grid_array<int>("get_grid_shape", grid, shape, GetGridRank(grid), "int");
            }

            int GetGridSize(const int grid) override {
                return py::int_(bmi_model->attr("get_grid_size")(grid));
            }

            void GetGridSpacing(const int grid, double *spacing) override {
                get_and_copy_grid_array<double>("get_grid_spacing", grid, spacing, GetGridRank(grid), "float");
            }

            string GetGridType(const int grid) override {
                return py::str(bmi_model->attr("get_grid_type")(grid));
            }

            void GetGridX(const int grid, double *x) override {
                // From the BMI docs:
                //   "The length of the resulting one-dimensional array depends on the grid type. (It will have either
                //   get_grid_rank or get_grid_size elements.) See Model grids for more information."
                // Leaving this not implemented for now, since it is probably not yet needed, to make sure a proper
                // solution to determining the array length is used.
                throw runtime_error("GetGridX not yet implemented for Python BMI adapter");
            }

            void GetGridY(const int grid, double *y) override {
                // From the BMI docs:
                //   "The length of the resulting one-dimensional array depends on the grid type. (It will have either
                //   get_grid_rank or get_grid_size elements.) See Model grids for more information."
                // Leaving this not implemented for now, since it is probably not yet needed, to make sure a proper
                // solution to determining the array length is used.
                throw runtime_error("GetGridY not yet implemented for Python BMI adapter");
            }

            void GetGridZ(const int grid, double *z) override {
                // From the BMI docs:
                //   "The length of the resulting one-dimensional array depends on the grid type. (It will have either
                //   get_grid_rank or get_grid_size elements.) See Model grids for more information."
                // Leaving this not implemented for now, since it is probably not yet needed, to make sure a proper
                // solution to determining the array length is used.
                throw runtime_error("GetGridZ not yet implemented for Python BMI adapter");
            }

            double GetStartTime() override;

            string GetTimeUnits() override;

            double GetTimeStep() override;

            string GetVarType(std::string name) override;

            string GetVarUnits(std::string name) override;

            int GetVarGrid(std::string name) override;

            int GetVarItemsize(std::string name) override;

            int GetVarNbytes(std::string name) override;

            string GetVarLocation(std::string name) override;

            void GetValue(std::string name, void *dest) override;

            void GetValueAtIndices(std::string name, void *dest, int *inds, int count) override;

            void *GetValuePtr(std::string name) override;

            template <typename T>
            void get_and_copy_grid_array(const char* grid_func_name, const int grid, T* dest, int dest_length,
                                         const char* np_dtype)
            {
                py::array_t<T> np_array = np.attr("zeros")(dest_length, "dtype"_a = np_dtype);
                bmi_model->attr(grid_func_name)(grid, np_array);
                auto np_array_direct = np_array.template unchecked<1>();
                for (int i = 0; i < dest_length; ++i)
                    dest[i] = np_array_direct(i);
            }

            /**
             * Get the name of the parent package for the Python BMI model type.
             *
             * @return The name of the parent package for the Python BMI model type.
             */
            std::string get_bmi_type_package() const;

            /**
             * Get the simple name of the Python BMI model type.
             *
             * @return The simple name of the Python BMI model type.
             */
            std::string get_bmi_type_simple_name() const;

            /**
             * Convenience function to get sum of values of ``GetGridNodesPerFace``.
             *
             * The sum of the values returned by the BMI ``GetGridNodesPerFace`` is used to determine the length of the
             * required array for several other BMI grid functions.  As such, this function simplifies the process for
             * obtaining said sum.
             *
             * @param grid The model grid identifier.
             * @return
             */
            int get_sum_of_grid_nodes_per_face(const int grid) {
                int grid_node_face_count = GetGridFaceCount(grid);
                int grid_nodes_per_face[grid_node_face_count];
                GetGridNodesPerFace(grid, grid_nodes_per_face);
                int nodes_per_face_sum = 0;
                for (int i = 0; i < grid_node_face_count; ++i)
                    nodes_per_face_sum += grid_nodes_per_face[i];
                return nodes_per_face_sum;
            }

            /**
             * Get the value for a variable at specified indices, potentially optimizing for all-indices case.
             *
             * This function consolidates logic needed to efficiently implement @ref GetValue and @ref GetValueAtIndices
             * using a single execution path.  It first directly implements the logic for determining the correct native
             * type for the BMI variable of the given name.  This type is then used as a template parameter for a nested
             * call to @ref get_via_numpy_array, and this function then optimizes getting the value from the Python
             * model using the most appropriate BMI getter function for the situation (i.e., all or specific indices).
             *
             * If a supported native type for use with @ref get_via_numpy_array cannot be inferred from the values
             * returned by @ref GetVarType and @ref GetVarItemsize, then a ``runtime_error`` is thrown.
             *
             * @param name The name of the desired variable.
             * @param dest Destination array pointer to which to copy item values.
             * @param inds Pointer to array holding desired indices of items to obtain.
             * @param count The number of item values to be copied.
             * @param is_all_indices Whether all items for variable are to be copied, which would permit optimization.
             * @throws runtime_error Thrown if @ref GetVarType and @ref GetVarItemsize functions return a combination for
             *                       which there is not support for mapping to a native type in the framework.
             */
            void get_value_at_indices(const string& name, void *dest, int *inds, int count, bool is_all_indices) {
                string val_type = GetVarType(name);
                size_t val_item_size = (size_t)GetVarItemsize(name);
                vector<string> in_v = GetInputVarNames();

                // The available types and how they are handled here should match what is in SetValueAtIndices
                if (val_type == "int" && val_item_size == sizeof(short))
                    get_via_numpy_array<short>(name, dest, inds, count, val_item_size, is_all_indices);
                else if (val_type == "int" && val_item_size == sizeof(int))
                    get_via_numpy_array<int>(name, dest, inds, count, val_item_size, is_all_indices);
                else if (val_type == "int" && val_item_size == sizeof(long))
                    get_via_numpy_array<long>(name, dest, inds, count, val_item_size, is_all_indices);
                else if (val_type == "int" && val_item_size == sizeof(long long))
                    get_via_numpy_array<long long>(name, dest, inds, count, val_item_size, is_all_indices);
                else if (val_type == "float" && val_item_size == sizeof(float))
                    get_via_numpy_array<float>(name, dest, inds, count, val_item_size, is_all_indices);
                else if (val_type == "float" && val_item_size == sizeof(double))
                    get_via_numpy_array<double>(name, dest, inds, count, val_item_size, is_all_indices);
                else if (val_type == "float" && val_item_size == sizeof(long double))
                    get_via_numpy_array<long double>(name, dest, inds, count, val_item_size, is_all_indices);
                else
                    throw runtime_error(
                            "(Bmi_Py_Adapter) Failed attempt to GET values of BMI variable '" + name + "' from '" +
                            model_name + "' model:  model advertises unsupported combination of type (" + val_type +
                            ") and size (" + std::to_string(val_item_size) + ").");
            }

            /**
             * Get the values at some set of indices for the given variable, using intermediate wrapped numpy array.
             *
             * The function performs checking to optimize whether a nested call to ``get_value`` or
             * ``get_value_at_indices`` is required.  @ref GetValue and @ref GetValueAtIndices.  This enables
             * centralization of that while maintaining the ability to optimize for the case when all indices are to
             * be retrieved, and thus permitting calling the Python ``get_value`` instead of ``get_value_at_indices``.
             *
             * The acquired wrapped numpy arrays will be set to use ``py::array::c_style`` ordering.
             *
             * The ``indices`` parameter is ignored in cases when ``is_all_indices`` is ``true``.
             *
             * @tparam T The C++ type for the obtained values.
             * @param name The name of the variable for which to obtain values.
             * @param dest A pre-allocated destination pointer in which to copy retrieved values.
             * @param indices The specific indices of on the model side for which variable values are to be retrieved.
             * @param item_count The number of individual values to retrieve.
             * @param item_size The size in bytes for a single item of the type in question.
             * @param is_all_indices Whether all indices for the variable are being requested.
             * @return
             */
            template <typename T>
            py::array_t<T> get_via_numpy_array(const string& name, void *dest, const int *indices, int item_count,
                                               size_t item_size, bool is_all_indices)
            {
                string val_type = GetVarType(name);
                py::array_t<T, py::array::c_style> dest_array
                    = np.attr("zeros")(item_count, "dtype"_a = val_type, "order"_a = "C");
                if (is_all_indices) {
                    bmi_model->attr("get_value")(name, dest_array);
                }
                else {
                    py::array_t<int, py::array::c_style> indices_np_array
                            = np.attr("zeros")(item_count, "dtype"_a = "int", "order"_a = "C");
                    auto indices_mut_direct = indices_np_array.mutable_unchecked<1>();
                    for (py::size_t i = 0; i < (py::size_t) item_count; ++i)
                        indices_mut_direct(i) = indices[i];
                    /* Leaving this here for now in case mutable_unchecked method doesn't behave as expected in testing
                    py::buffer_info buffer_info = indices_np_array.request();
                    int *indices_np_arr_ptr = (int*)buffer_info.ptr;
                    for (int i = 0; i < item_count; ++i)
                        indices_np_arr_ptr[i] = indices[i];
                    */
                    bmi_model->attr("get_value_at_indices")(name, dest_array, indices_np_array);
                }

                auto direct_access = dest_array.template unchecked<1>();
                for (py::size_t i = 0; i < (py::size_t) item_count; ++i)
                    ((T *) dest)[i] = direct_access(i);

                return dest_array;
            }

            /**
             * Parse variable value(s) from within "other_vars" property of formulation config, to a numpy array suitable for
             * passing to the BMI model via the ``set_value`` function.
             *
             * @param other_value_json JSON node containing variable/parameter value(s) needing to be passed to a BMI model.
             * @return A bound Python numpy array to containing values to pass to a BMI model object via ``set_value``.
             */
            static py::array parse_other_var_val_for_setter(const geojson::JSONProperty& other_value_json);

            void Update() override;

            void UpdateUntil(double time) override;

            void SetValue(std::string name, void *src) override {
                void* dest = GetValuePtr(name);
                vector<string> in_v = GetInputVarNames();
                memcpy(dest, src, GetVarNbytes(name));
            }

            /**
             * Set values for a model's BMI variable at specified indices.
             *
             * This function is required to fulfill the @ref ::bmi::Bmi interface.  It essentially gets the advertised
             * type and size of the variable in question via @ref GetVarType and @ref GetVarItemsize to infer the native
             * type for this variable (i.e., the actual type for the values pointed to by ``src``).  It then uses this
             * as the type param in a nested called to the template-based @ref set_value_at_indices.  If such a type
             * param cannot be determined, a ``runtime_error`` is thrown.
             *
             * @param name The name of the involved BMI variable.
             * @param inds A C++ integer array of indices to update, corresponding to each value in ``src``.
             * @param count Number of elements in the ``inds`` and ``src`` arrays.
             * @param src A C++ array containing the new values to be set in the BMI variable.
             * @throws runtime_error Thrown if @ref GetVarType and @ref GetVarItemsize functions return a combination
             *                       for which there is not support for mapping to a native type in the framework.
             * @see set_value_at_indices
             */
            void SetValueAtIndices(std::string name, int *inds, int count, void *src) override;

            /**
             * Set values for a model's BMI variable at specified indices using the Python ``set_value_at_indices``.
             *
             * Perform a call to the Python model's ``set_value_at_indices`` function to set the given variable's values
             * at the appropriate indexes.  Since this Python function requires Numpy arrays as arguments, create
             * wrapped Numpy arrays analogous to ``inds`` and ``cxx_array``, copying into each analog the values of the
             * C++ arrays.
             *
             * The parameters serve very much the same purpose as the standard BMI ``SetValueAtIndices``.  The
             * additional ``np_type`` parameter allows the required wrapped Numpy array to be created using the correct
             * dtype for the variable in question.
             *
             * @tparam T The C++ type to use for the wrapper template holding the Numpy array of values to be set.
             * @param name The name of the involved BMI variable.
             * @param inds A C++ integer array of indices to update, corresponding to each value in ``cxx_array``.
             * @param count Number of elements in the ``inds`` and ``cxx_array`` arrays.
             * @param cxx_array A C++ array of unknown type containing the new values to be set in the BMI variable.
             * @param np_type The name of the Python dtype to use for the wrapped Numpy array of update values.
             */
            template <typename T>
            void set_value_at_indices(const string &name, const int *inds, int count, void* cxx_array,
                                      const char* np_type)
            {
                py::array_t<int> index_np_array = np.attr("zeros")(count, "dtype"_a = "int");
                py::array_t<T> src_np_array = np.attr("zeros")(count, "dtype"_a = np_type);
                // These get direct access (mutable) to the arrays, since we don't need to worry about dimension checks
                // as we just created the arrays
                auto index_mut_direct = index_np_array.mutable_unchecked<1>();
                auto src_mut_direct = index_np_array.mutable_unchecked<1>();
                for (py::size_t i = 0; i < (py::size_t) count; ++i) {
                    index_mut_direct(i) = inds[i];
                    src_mut_direct(i) = ((T *)cxx_array)[i];
                }

                /* The other method should work better, but leaving this for now in case.
                py::buffer_info indx_buffer_info = index_np_array.request();
                py::buffer_info src_buffer_info = src_np_array.request();
                for (int i = 0; i < count; ++i) {
                    ((int *) indx_buffer_info.ptr)[i] = inds[i];
                    ((T *) src_buffer_info.ptr)[i] = ((T *) cxx_array)[i];
                }
                */

                bmi_model->attr("set_value_at_indices")(name, index_np_array, src_np_array);
            }

        protected:
            std::string model_name = "BMI Python model";

            /**
             * Construct the backing BMI model object, then call its BMI-native ``Initialize()`` function.
             *
             * Implementations should return immediately without taking any further action if ``model_initialized`` is
             * already ``true``.
             *
             * The call to the BMI native ``Initialize(string)`` should pass the value stored in ``bmi_init_config``.
             */
            void construct_and_init_backing_model() override {
                if (model_initialized)
                    return;
                try {
                    separate_package_and_simple_name();
                    // This is a class object for the BMI module Python class
                    py::object bmi_py_class = py::module_::import(py_bmi_type_package_name->c_str()).attr(
                            py_bmi_type_simple_name->c_str());
                    // This is the actual backing model object
                    bmi_model = make_shared<py::object>(bmi_py_class());
                    bmi_model->attr("Initialize")(bmi_init_config);
                }
                    // Record the exception message before re-throwing to handle subsequent function calls properly
                catch (exception& e) {
                    init_exception_msg = string(e.what());
                    // Make sure this is non-empty to be consistent with the above logic
                    if (init_exception_msg.empty()) {
                        init_exception_msg = "Unknown Python model initialization exception.";
                    }
                    throw e;
                }
            }


        private:

            /** Fully qualified Python type name for backing module. */
            string bmi_py_type_name;
            /** A binding to the Python numpy package/module. */
            py::module_ np;
            /** A pointer to a string with the parent package name of the Python type referenced by ``py_bmi_type_ref``. */
            shared_ptr<string> py_bmi_type_package_name;
            /** A pointer to a string with the simple name of the Python type referenced by ``py_bmi_type_ref``. */
            shared_ptr<string> py_bmi_type_simple_name;

            inline void separate_package_and_simple_name() {
                if (!model_initialized) {
                    vector<string> split_name;
                    string delimiter = ".";

                    size_t pos = 0;
                    string token;
                    while ((pos = bmi_py_type_name.find(delimiter)) != string::npos) {
                        token = bmi_py_type_name.substr(0, pos);
                        split_name.emplace_back(token);
                        bmi_py_type_name.erase(0, pos + delimiter.length());
                    }

                    py_bmi_type_simple_name = make_shared<string>(split_name.back());
                    split_name.pop_back();
                    py_bmi_type_package_name = make_shared<string>(boost::algorithm::join(split_name, delimiter));
                }
            }
        };

    }
}

#endif //ACTIVATE_PYTHON

#endif //NGEN_BMI_PY_ADAPTER_H
