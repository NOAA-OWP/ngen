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
        /**
         * Parameterized constructor Routing_Py_Adapter with the flow_vector
         *
         * @param t_route_connection_path
         * @param input_path
         * @param catchment_subset_ids
         * @param number_of_timesteps
         * @param delta_time
         * @param flow_vector
         */
        Routing_Py_Adapter(std::string t_route_connection_path, std::string input_path, 
                           const std::vector<std::string> &catchment_subset_ids,
                           int number_of_timesteps, int delta_time,
                           const std::vector<double> &flow_vector);


        /**
         * Parameterized constructor Routing_Py_Adapter without the flow_vector
         *
         * @param t_route_connection_path
         * @param input_path
         * @param catchment_subset_ids
         * @param number_of_timesteps
         * @param delta_time
         */
        Routing_Py_Adapter(std::string t_route_connection_path, std::string input_path, 
                           const std::vector<std::string> &catchment_subset_ids, 
                           int number_of_timesteps, int delta_time);

        template <typename T>
        void convert_vector_to_numpy_array(const std::vector<T> &flow_vector)
        {
            /**
            * NOTE: Currently not using this function because current plan is to have routing read Nexus
            * values from Nexus output files instead of passing in-memory values to t-route. If
            * the ability to pass in-memory values to t-route is needed, then the following TODOs are
            * needed.
            * TODO: Retrieve and convert map of Nexus IDs with Nexus flow vectors to Python dictionary
            *       of NumPy arrays
            * TODO: Retrieve single time index vector for all Nexus flow values and convert to NumPy array
            */

            //Allocate py::array (to pass the result of the C++ function to Python)
            auto result = py::array_t<T>(flow_vector.size());
            auto result_buffer = result.request();
            T *result_ptr = (T *) result_buffer.ptr;

            //Copy std::vector -> py::array
            std::memcpy(result_ptr, flow_vector.data(), flow_vector.size()*sizeof(T));

            //Create object for receive_flow_values subroutine
            py::object receive_flow_values = t_route_module.attr("receive_flow_values");

            //Call receive_flow_values subroutine
            receive_flow_values(result);

        }


    private:

        /** A binding to the Python numpy package/module. */
        py::module_ np;

        /** A binding to the t-route module. */
        py::module_ t_route_module;
    };

}


#endif //ACTIVATE_PYTHON

#endif //NGEN_ROUTING_PY_ADAPTER_H
