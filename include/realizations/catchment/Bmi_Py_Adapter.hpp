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
#include "python/InterpreterUtil.hpp"

// Forward declaration to provide access to protected items in testing
class Bmi_Py_Adapter_Test;

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

            Bmi_Py_Adapter(const string &type_name, std::string bmi_init_config, const string &bmi_python_type,
                           bool allow_exceed_end, bool has_fixed_time_step, utils::StreamHandler output);

            Bmi_Py_Adapter(const string &type_name, std::string bmi_init_config, const string &bmi_python_type,
                           std::string forcing_file_path, bool allow_exceed_end, bool has_fixed_time_step,
                           utils::StreamHandler output);

            /**
             * Copy the given BMI variable's values from the backing numpy array to a C++ array.
             *
             * @tparam T The appropriate C++ type for the values.
             * @param name The name of the BMI variable in question.
             * @param dest A pointer to an already-allocated array in which to copy the values of the desired BMI
             *             variable, which is assume to be of the necessary size.
             */
            template <typename T>
            void copy_to_array(const string& name, T *dest)
            {
                py::array_t<T> backing_array = bmi_model->attr("get_value_ptr")(name);
                auto uncheck_proxy = backing_array.template unchecked<1>();
                for (ssize_t i = 0; i < backing_array.size(); ++i) {
                    dest[i] = uncheck_proxy(i);
                }
            }

            /**
             * Copy the given BMI variable's values from the backing numpy array to a C++ vector.
             *
             * @tparam T The appropriate C++ type for the values.
             * @param name The name of the BMI variable in question.
             * @return A vector containing the values of the desired BMI variable.
             */
            template <typename T>
            std::vector<T> copy_to_vector(const string& name)
            {
                py::array_t<T> backing_array = bmi_model->attr("get_value_ptr")(name);
                std::vector<T> dest(backing_array.size());
                auto uncheck_proxy = backing_array.template unchecked<1>();
                for (ssize_t i = 0; i < backing_array.size(); ++i) {
                    dest[i] = uncheck_proxy(i);
                }
                return dest;
            }

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

            /**
             * Get the name string for the C++ type analogous to the described type in the Python backing model.
             *
             * Note that the size of an individual item is also required, as this can vary in certain situations in
             * Python.
             *
             * @param py_type_name The string name of the analog type in Python.
             * @param item_size The particular size in bytes for items of the involved analogous types.
             * @return The name string for the C++ type analogous to the described type in the Python backing model.
             */
            const std::string get_analogous_cxx_type(const std::string &py_type_name, const size_t item_size) override {
                /*
                 * Note that an implementation using a "switch" statement would be problematic.  It could be done by
                 * rewriting to separate the integer and non-integer type, then having cases based on size.  However,
                 * this might cause trouble on certain systems, since (depending on the particular sizes of types) that
                 * could produce duplicate "case" values.
                 */
                //TODO: include other numpy type strings, https://numpy.org/doc/stable/user/basics.types.html
                if (py_type_name == "int" && item_size == sizeof(short)) {
                    return "short";
                } else if (py_type_name == "int" && item_size == sizeof(int)) {
                    return "int";
                } else if (py_type_name == "int" && item_size == sizeof(long)) {
                    return "long";
                } else if ( (py_type_name == "int" || py_type_name == "int64") && item_size == sizeof(long long)) {
                    return "long long";
                } else if (py_type_name == "longlong" && item_size == sizeof(long long)) {
                    return "long long"; //numpy type
                } else if (py_type_name == "float" && item_size == sizeof(float)) {
                    return "float";
                } else if ((py_type_name == "float" || py_type_name == "float64" || py_type_name == "np.float64" ||
                            py_type_name == "numpy.float64") && item_size == sizeof(double)) {
                    return "double";
                } else if (py_type_name == "float" && item_size == sizeof(long double)) {
                    return "long double";
                } else {
                    throw runtime_error(
                            "(Bmi_Py_Adapter) Failed determining analogous C++ type for Python model '" + py_type_name +
                            "' type with size " + std::to_string(item_size) + " bytes.");
                }
            }


            /**
             * Get the analogous built-in Python type for the C++ type with the provided name.
             *
             * @param cxx_type_name The string name of the analog type in C++.
             * @return The name of the appropriate built-in Python type.
             */
            inline std::string get_analog_python_builtin(const std::string &cxx_type_name) {
                if (cxx_type_name == "short" || cxx_type_name == "int" || cxx_type_name == "long" ||
                    cxx_type_name == "long long") {
                    return "int";
                } else if (cxx_type_name == "float" || cxx_type_name == "double" || cxx_type_name == "long double") {
                    return "double";
                } else {
                    throw runtime_error("(Bmi_Py_Adapter) Failed determining analogous built-in Python type for C++ '" +
                                        cxx_type_name + "' type");
                }
            }

            /**
             * Get the analogous Python type appropriate for use in numpy arrays for the described  C++ type.
             *
             * @param cxx_type_name The string name of the analog type in C++.
             * @param item_size The particular size in bytes for items of the involved analogous types.
             * @return The name of the appropriate Python type.
             */
            inline std::string get_analog_python_dtype(const std::string &cxx_type_name, const size_t item_size) {
                // TODO: figure out how to correctly get this
                std::string numpy_module_name = "numpy";

                if (cxx_type_name == "short" || cxx_type_name == "int" || cxx_type_name == "long" ||
                    cxx_type_name == "long long")
                {
                    switch (item_size) {
                        case 1: return numpy_module_name + ".int8";
                        case 2: return numpy_module_name + ".int16";
                        case 4: return numpy_module_name + ".int32";
                        case 8: return numpy_module_name + ".int64";
                        default: break;
                    }
                }

                if (cxx_type_name == "float" || cxx_type_name == "double" || cxx_type_name == "long double") {
                    switch (item_size) {
                        case 2: return numpy_module_name + ".float16";
                        case 4: return numpy_module_name + ".float32";
                        case 8: return numpy_module_name + ".float64";
                        default: break;
                    }
                }

                throw runtime_error("(Bmi_Py_Adapter) Failed determining analogous Python dtype for C++ '" +
                                    cxx_type_name + "' type with size " + std::to_string(item_size) + " bytes.");
            }

            /**
             * Get the analogous Python type (or ``dtype``) for the described C++ type.
             *
             * Get the appropriate name of the analogous Python type for the described C++ type.  If set to do so, use
             * numpy-specific types appropriate for an array ``dtype`` over standard, built-in Python types.
             *
             * @param cxx_type_name The string name of the analog type in C++.
             * @param item_size The particular size in bytes for items of the involved analogous types.
             * @param is_dtype Whether to prioritize numpy-specific types
             * @return The name of the appropriate Python type.
             */
            inline std::string get_analog_python_type(const std::string &cxx_type_name, const size_t item_size,
                                                      const bool is_dtype)
            {
                return is_dtype ? get_analog_python_dtype(cxx_type_name, item_size) : get_analog_python_builtin(
                        cxx_type_name);
            }

            /**
             * Get the analogous Python type for the described C++ type.
             *
             * Get the appropriate name of the analogous Python type for the described C++ type.  This variant assumes
             * that a standard built-in type is sufficient, and numpy-specific types are not needed (as they might be
             * for certain situations, like when dealing with BMI variable numpy array type).
             *
             * @param cxx_type_name The string name of the analog type in C++.
             * @param item_size The particular size in bytes for items of the involved analogous types.
             * @return The name of the appropriate Python type.
             */
            inline std::string get_analog_python_type(const std::string &cxx_type_name, const size_t item_size) {
                return get_analog_python_type(cxx_type_name, item_size, false);
            }

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
                else if (val_type == "float" || val_type == "float16" || val_type == "float32" || val_type == "float64") {
                    if (val_item_size == sizeof(float))
                        get_via_numpy_array<float>(name, dest, inds, count, val_item_size, is_all_indices);
                    else if (val_item_size == sizeof(double))
                        get_via_numpy_array<double>(name, dest, inds, count, val_item_size, is_all_indices);
                    else if (val_item_size == sizeof(long double))
                        get_via_numpy_array<long double>(name, dest, inds, count, val_item_size, is_all_indices);
                }
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
                            = np.attr("zeros")(item_count, "dtype"_a = "int32", "order"_a = "C");
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
             * Whether the backing model has been initialized yet.
             *
             * @return Whether the backing model has been initialized yet.
             */
            inline bool is_model_initialized() {
                return model_initialized;
            }

            void Update() override;

            void UpdateUntil(double time) override;

            void SetValue(std::string name, void *src) override {
                int itemSize = GetVarItemsize(name);
                std::string py_type = GetVarType(name);
                std::string cxx_type = get_analogous_cxx_type(py_type, (size_t) itemSize);

                if (cxx_type == "short") {
                    set_value<short>(name, (short *) src);
                } else if (cxx_type == "int") {
                    set_value<int>(name, (int *) src);
                } else if (cxx_type == "long") {
                    set_value<long>(name, (long *) src);
                } else if (cxx_type == "long long") {
                    set_value<long long>(name, (long long *) src);
                } else if (cxx_type == "float") {
                    set_value<float>(name, (float *) src);
                } else if (cxx_type == "double") {
                    set_value<double>(name, (double *) src);
                } else if (cxx_type == "long double") {
                    set_value<long double>(name, (long double *) src);
                } else {
                    throw std::runtime_error("Bmi_Py_Adapter cannot set values for variable '" + name +
                                             "' that has unrecognized C++ type '" + cxx_type + "'");
                }
            }

            /**
             * Set the values of the given BMI variable for the model, sourcing new data from the provided vector.
             *
             * @tparam T The type of the variable source values.
             * @param name The name of the involved BMI model variable.
             * @param src The source vector of new values to use to set the values in the backing BMI model, which must
             *            be of the same length as the model's variable array.
             */
            template <typename T>
            void set_value(const std::string &name, std::vector<T> src) {
                int nbytes = GetVarNbytes(name);
                int itemSize = GetVarItemsize(name);
                int length = nbytes / itemSize;

                if (length != src.size()) {
                    throw std::runtime_error(
                            "Bmi_Py_Adapter mismatch of lengths setting variable array (" + std::to_string(length) +
                            " expected but " + std::to_string(src.size()) + " received)");
                }

                py::array_t<T> model_var_array = bmi_model->attr("get_value_ptr")(name);
                auto mutable_unchecked_proxy = model_var_array.template mutable_unchecked<1>();
                for (size_t i = 0; i < length; ++i) {
                    mutable_unchecked_proxy(i) = src[i];
                }
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
                                      const string &np_type)
            {
                py::array_t<int> index_array(py::buffer_info(inds, count));
                py::array_t<T> src_array(py::buffer_info((T*)cxx_array, count));
                bmi_model->attr("set_value_at_indices")(name, index_array, src_array);
            }

        protected:
            std::string model_name = "BMI Python model";

            /**
             * Construct the backing BMI model object, then call its BMI-native ``Initialize()`` function.
             *
             * Returns immediately without taking any further action if ``model_initialized`` is already ``true``.
             *
             * Performs a call to the BMI native ``Initialize(string)`` and passes the value stored in
             * ``bmi_init_config``.
             */
            void construct_and_init_backing_model() override {
                construct_and_init_backing_model_for_py_adapter();
            }

        private:

            /** Fully qualified Python type name for backing module. */
            string bmi_type_py_full_name;
            /** A binding to the Python numpy package/module. */
            py::object np;
            /** A pointer to a string with the parent package name of the Python type referenced by ``py_bmi_type_ref``. */
            shared_ptr<string> bmi_type_py_module_name;
            /** A pointer to a string with the simple name of the Python type referenced by ``py_bmi_type_ref``. */
            shared_ptr<string> bmi_type_py_class_name;

            /**
             * Construct the backing BMI model object, then call its BMI-native ``Initialize()`` function.
             *
             * Private wrapper function, to allow use from constructors.
             *
             * Returns immediately without taking any further action if ``model_initialized`` is already ``true``.
             *
             * Performs a call to the BMI native ``Initialize(string)`` and passes the value stored in
             * ``bmi_init_config``.
             */
            inline void construct_and_init_backing_model_for_py_adapter() {
                if (model_initialized)
                    return;
                try {
                    separate_package_and_simple_name();
                    vector<string> moduleComponents = {*bmi_type_py_module_name, *bmi_type_py_class_name};
                    // This is a class object for the BMI module Python class
                    py::object bmi_py_class = utils::ngenPy::InterpreterUtil::getPyModule(moduleComponents);
                    // This is the actual backing model object
                    bmi_model = make_shared<py::object>(bmi_py_class());
                    bmi_model->attr("initialize")(bmi_init_config);
                }
                catch (std::runtime_error& e){ //Catch specific exception types so the type/message don't get erased
                    throw e;
                }
                // Record the exception message before re-throwing to handle subsequent function calls properly
                // TODO: handle exceptions in better detail, without losing type information
                catch (exception& e) {
                    init_exception_msg = string(e.what());
                    // Make sure this is non-empty to be consistent with the above logic
                    if (init_exception_msg.empty()) {
                        init_exception_msg = "Unknown Python model initialization exception.";
                    }
                    throw e;
                }
            }

            /**
             * Parse the full name of the BMI model type to its module/package name and (simple) class name.
             *
             * Parse the full name into separate components, expecting ``.`` as the delimiter.  Expect the last
             * substring to be the class name.  Rejoin the earlier substrings to construct the module or package name.
             *
             * @throw std::runtime_error Thrown if initial type name not in the format ``<module_name>.<class_name>``.
             */
            inline void separate_package_and_simple_name() {
                if (!model_initialized) {
                    vector<string> split_name;
                    string delimiter = ".";
                    string name_string = bmi_type_py_full_name;

                    size_t pos = 0;
                    string token;
                    while ((pos = name_string.find(delimiter)) != string::npos) {
                        token = name_string.substr(0, pos);
                        split_name.emplace_back(token);
                        name_string.erase(0, pos + delimiter.length());
                    }
                    if (split_name.empty()) {
                        throw std::runtime_error("Cannot interpret BMI Python model type '" + bmi_type_py_full_name
                                                 + "'; expected format is <python_module>.<python_class>");
                    }
                    // What's left should be the class name
                    bmi_type_py_class_name = make_shared<string>(name_string);
                    //split_name.pop_back();
                    // And then the split name should contain the module
                    // TODO: going to need to look at this again in the future; right now, assuming the format
                    //  <python_module>.<python_class> works fine as long as a model class is always in a top-level
                    //  module, but the current logic is going to interpret any complex parent module name as a single
                    //  top-level namespace package; e.g., ngen.namespacepackage.model works if ngen.namespacepackage is
                    //  a namespace package, but there would be problems with something like ngenpkg.innermodule1.model.
                    bmi_type_py_module_name = make_shared<string>(boost::algorithm::join(split_name, delimiter));
                }
            }

            /**
             * Set the values of the given BMI variable based on a provided C++ array of values.
             *
             * @tparam T The type of source values, assumed to be appropriate for the involved variable.
             * @param name The name of the involved BMI model variable.
             * @param src An array of source values to apply to the BMI variable, assumed to be of the same size as the
             *            BMI model's current array for the involved variable.
             */
            template <typename T>
            void set_value(const std::string &name, T *src) {
                // Because all BMI arrays are flattened, we can just use the size/length in the buffer info
                int length = GetVarNbytes(name) / GetVarItemsize(name);
                py::array_t<T> src_array(py::buffer_info(src, length));
                bmi_model->attr("set_value")(name, src_array);
            }

            // For unit testing
            friend class ::Bmi_Py_Adapter_Test;
        };

    }
}

#endif //ACTIVATE_PYTHON

#endif //NGEN_BMI_PY_ADAPTER_H
