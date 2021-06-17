#ifdef ACTIVATE_PYTHON

#include <exception>
#include <utility>

#include "Routing_Py_Adapter.hpp"
//#include "boost/algorithm/string.hpp"

using namespace routing_py_adapter;

Routing_Py_Adapter::Routing_Py_Adapter(std::string t_route_connection_path, std::string input_path,
                                       const std::vector<std::string> &ids,
                                       const std::vector<double> &flow_vector)
{}

Routing_Py_Adapter::Routing_Py_Adapter(std::string t_route_connection_path, std::string input_path,
                                       const std::vector<std::string> &ids, int number_of_timesteps,
                                       int delta_time)
{
  py::module_ sys = py::module_::import("sys");

  py::object sys_path = sys.attr("path");

  py::object sys_path_append = sys_path.attr("append");

  sys_path_append(t_route_connection_path);

  sys_path_append(input_path);

  //py::module_ t_route_module = py::module_::import("next_gen_network_module");
  //this->t_route_module = py::module_::import("next_gen_network_module");
  this->t_route_module = py::module_::import("next_gen_route_main");

  py::object set_paths = t_route_module.attr("set_paths");

  set_paths(t_route_connection_path);

  py::object ngen_routing = t_route_module.attr("ngen_routing");

  ngen_routing(number_of_timesteps, delta_time);

  py::object call_read_catchment_lateral_flows = t_route_module.attr("call_read_catchment_lateral_flows");

  call_read_catchment_lateral_flows(input_path);

}


void Routing_Py_Adapter::convert_vector_to_numpy_array(const std::vector<double> &flow_vector)
{

  //Convert map to py dict
  //Convert time index
  //Pass single time vector for all nexus vals

  // allocate py::array (to pass the result of the C++ function to Python)
  auto result = py::array_t<double>(flow_vector.size());
  auto result_buffer = result.request();
  double *result_ptr = (double *) result_buffer.ptr;

  // copy std::vector -> py::array
  std::memcpy(result_ptr, flow_vector.data(), flow_vector.size()*sizeof(double));

  py::object receive_flow_values = t_route_module.attr("receive_flow_values");

  receive_flow_values(result);

}


#endif //ACTIVATE_PYTHON
