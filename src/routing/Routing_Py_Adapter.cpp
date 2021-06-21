#ifdef ACTIVATE_PYTHON

#include <exception>
#include <utility>

#include "Routing_Py_Adapter.hpp"
//#include "boost/algorithm/string.hpp"

using namespace routing_py_adapter;


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
Routing_Py_Adapter::Routing_Py_Adapter(std::string t_route_connection_path, std::string input_path,
                                       const std::vector<std::string> &catchment_subset_ids,
                                       int number_of_timesteps, int delta_time,
                                       const std::vector<double> &flow_vector)
{}

/**
 * Parameterized constructor Routing_Py_Adapter without the flow_vector
 *
 * @param t_route_connection_path
 * @param input_path
 * @param catchment_subset_ids
 * @param number_of_timesteps
 * @param delta_time
 */
Routing_Py_Adapter::Routing_Py_Adapter(std::string t_route_connection_path, std::string input_path,
                                       const std::vector<std::string> &catchment_subset_ids,
                                       int number_of_timesteps, int delta_time)
{

  //Cast vector of catchment_subset_ids to Python list 
  py::list catchment_subset_ids_list = py::cast(catchment_subset_ids);

  //Bind python sys module
  py::module_ sys = py::module_::import("sys");

  //Create object sys_path for the Python system path
  py::object sys_path = sys.attr("path");

  //Create object to append to the sys_path
  py::object sys_path_append = sys_path.attr("append");

  //Append the t_route_connection_path
  sys_path_append(t_route_connection_path);

  //Append the input_path
  sys_path_append(input_path);

  //Leave this call here for now for calling previous version of module
  //this->t_route_module = py::module_::import("next_gen_network_module");

  //Import next_gen_route_main
  this->t_route_module = py::module_::import("next_gen_route_main");

  //Create object for the set_paths subroutine
  py::object set_paths = t_route_module.attr("set_paths");

  //Call set_paths subroutine
  set_paths(t_route_connection_path);

  //Create object for ngen_routing subroutine
  py::object ngen_routing = t_route_module.attr("ngen_routing");

  //Call ngen_routing subroutine
  ngen_routing(number_of_timesteps, delta_time);

  //Create object for call_read_catchment_lateral_flows subroutine
  py::object call_read_catchment_lateral_flows = t_route_module.attr("call_read_catchment_lateral_flows");

  //Call call_read_catchment_lateral_flows subroutine
  call_read_catchment_lateral_flows(input_path);

}


void Routing_Py_Adapter::convert_vector_to_numpy_array(const std::vector<double> &flow_vector)
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
  auto result = py::array_t<double>(flow_vector.size());
  auto result_buffer = result.request();
  double *result_ptr = (double *) result_buffer.ptr;

  //Copy std::vector -> py::array
  std::memcpy(result_ptr, flow_vector.data(), flow_vector.size()*sizeof(double));

  //Create object for receive_flow_values subroutine
  py::object receive_flow_values = t_route_module.attr("receive_flow_values");

  //Call receive_flow_values subroutine
  receive_flow_values(result);

}


#endif //ACTIVATE_PYTHON
