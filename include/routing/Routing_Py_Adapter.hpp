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
#include "python/InterpreterUtil.hpp"

namespace py = pybind11;

namespace routing_py_adapter {

    class Routing_Py_Adapter {

    public:
        /**
         * @brief Construct Routing_Py_Adapter with configured paths
         * 
         * @param t_route_config_file_with_path path to a t-route yaml configuration file
         */
        Routing_Py_Adapter(std::string t_route_config_file_with_path);

        /**
         * Function to run @p number_of_timesteps of routing, extracting 
         * lateral inflows from @p flow_vector
         * 
         * FIXME This is current unimplemented and will require some addtional
         * work to properly map flows to the correct t-route network segments.
         * This may not even be the correct concept to implement this type of
         * "integrated" routing.  But the basic idea is that after a catchment 
         * update occurs, it contributes some amount of lateral flow to the channel
         * as represented by @p flow_vector and this flow should be routed down stream
         * for @p number_of_timesteps .  Note though, that all lateral inflows are required
         * for a single routing pass of the network, so this would probably end up being a
         * flow map that we pass to a custom t-route function that extracts the lateral inflow
         * vector for each identity and constructs the correct lateral inflow setup to make
         * a full routing pass.
         * 
         * See NOTE in @ref route(int, int) route() about python module availablity.
         *
         * @param number_of_timesteps
         * @param delta_time
         * @param flow_vector
         */
        void route(int number_of_timesteps, int delta_time,
              const std::vector<double> &flow_vector);


        /**
         * Function to run a full set of routing computations using the nexus output files
         * from an ngen simulation.
         * 
         * Currently, these parameters are ignored and are read instead from the yaml configuration
         * file contained in #t_route_config_path
         * 
         * NOTE this funtion uses a pybind11 embedded interperter to load the t-route namespace package
         * ngen-main and then executes the routing in the python interperter.  
         * It is assumed that the ngen-main module is available in the interperters PYTHON_PATH.
         * If the module cannot be found, then a ModuleNotFoundError will be thrown.
         * Similarly, ngen-main depends on severl other python modules.  If any of these are not in the
         * environments PYTHON_PATH, errors will occur.
         * 
         * It is reccommended to intall all t-route packages into a loaded virtual environment or
         * to the system site-packages.
         * 
         * @param number_of_timesteps
         * @param delta_time
         */
        void route(int number_of_timesteps, int delta_time);


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


        /** Handle to the interperter util.
         * 
         * Order is important, must be constructed before anything depending on it
         * and destructed after all dependent members.
        */
        std::shared_ptr<utils::ngenPy::InterpreterUtil> interperter;

        /** A binding to the Python numpy package/module. */
        py::object np;

        /** A binding to the t-route module. */
        py::object t_route_module;
        
        /** Path to a t-route yaml configuration file */
        std::string t_route_config_path;
    };

}


#endif //ACTIVATE_PYTHON

#endif //NGEN_ROUTING_PY_ADAPTER_H
