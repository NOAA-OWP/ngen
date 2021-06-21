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


        void convert_vector_to_numpy_array(const std::vector<double> &flow_vector);


    private:

        /** A binding to the Python numpy package/module. */
        py::module_ np;

        /** A binding to the t-route module. */
        py::module_ t_route_module;
    };

}


#endif //ACTIVATE_PYTHON

#endif //NGEN_ROUTING_PY_ADAPTER_H
