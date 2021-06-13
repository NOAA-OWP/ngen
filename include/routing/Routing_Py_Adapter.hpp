#ifndef NGEN_ROUTING_PY_ADAPTER_H
#define NGEN_ROUTING_PY_ADAPTER_H

#ifdef ACTIVATE_PYTHON

#include <exception>
#include <memory>
#include <string>
#include "pybind11/pybind11.h"
#include "pybind11/pytypes.h"
#include "pybind11/numpy.h"
#include <pybind11/stl.h>
//#include "JSONProperty.hpp"
//#include "StreamHandler.hpp"

namespace py = pybind11;

//using namespace std;


namespace routing_py_adapter {

    class Routing_Py_Adapter {

    public:

        Routing_Py_Adapter(std::string t_route_connection_path, std::string input_path, 
                           const std::vector<std::string> &ids,
                           const std::vector<double> &flow_vector);


        void convert_vector_to_numpy_array(const std::vector<double> &flow_vector);
        //void convert_vector_to_numpy_array(double flow_vector);


        /**
         * Get the values at some set of indices for the given variable, using intermediate wrapped numpy array.
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

/*
        template <typename T>
        py::array_t<T> get_via_numpy_array(const std::vector<double> &flow_vector, void *dest, const int *indices, int item_count,
                                           size_t item_size, bool is_all_indices)
        {
            //string val_type = GetVarType(name); //Maybe just stick to doubles for vector var type
            //GetVarType currently requires calling a bmi method

            //py::array_t<T, py::array::c_style> dest_array
            //    = np.attr("zeros")(item_count, "dtype"_a = "double", "order"_a = "C");

            // allocate py::array (to pass the result of the C++ function to Python)
            //auto result = py::array_t<double>(array.size());
            //auto result_buffer = result.request();
            //int *result_ptr    = (int *) result_buffer.ptr;

            // copy std::vector -> py::array
            //std::memcpy(result_ptr,result_vec.data(),result_vec.size()*sizeof(int));


            if (is_all_indices) {
                //bmi_model->attr("get_value")(name, dest_array);

                // copy std::vector -> py::array
                //std::memcpy(dest_array,result_vec.data(),result_vec.size()*sizeof(int));
                int aaa=1;


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
                //bmi_model->attr("get_value_at_indices")(name, dest_array, indices_np_array);
/*
            }

            auto direct_access = dest_array.template unchecked<1>();
            for (py::size_t i = 0; i < (py::size_t) item_count; ++i)
                ((T *) dest)[i] = direct_access(i);

            return dest_array;
        }
*/
    private:

        /** A binding to the Python numpy package/module. */
        py::module_ np;



    };


}




#endif //ACTIVATE_PYTHON

#endif //NGEN_ROUTING_PY_ADAPTER_H
