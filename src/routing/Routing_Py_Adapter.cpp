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
 * @param t_route_config_file_with_path
 * @param input_path
 * @param catchment_subset_ids
 * @param number_of_timesteps
 * @param delta_time
 * @param flow_vector
 */
Routing_Py_Adapter::Routing_Py_Adapter(std::string t_route_connection_path, 
                                       std::string t_route_config_file_with_path,
                                       std::string input_path,
                                       const std::vector<std::string> &catchment_subset_ids,
                                       int number_of_timesteps, int delta_time,
                                       const std::vector<double> &flow_vector)
{}

/**
 * Parameterized constructor Routing_Py_Adapter without the flow_vector
 *
 * @param t_route_connection_path
 * @param t_route_config_file_with_path
 * @param input_path
 * @param catchment_subset_ids
 * @param number_of_timesteps
 * @param delta_time
 */
Routing_Py_Adapter::Routing_Py_Adapter(std::string t_route_connection_path,
                                       std::string t_route_config_file_with_path,
                                       std::string input_path,
                                       const std::vector<std::string> &catchment_subset_ids,
                                       int number_of_timesteps, int delta_time)
{

  //Cast vector of catchment_subset_ids to Python list 
  py::list catchment_subset_ids_list = py::cast(catchment_subset_ids);

  //Append the t_route_connection_path
  utils::ngenPy::InterpreterUtil::addToPyPath(t_route_connection_path);

  //Append the input_path
  utils::ngenPy::InterpreterUtil::addToPyPath(input_path);

  //Leave this call here for now for calling previous version of module
  //this->t_route_module = py::module_::import("next_gen_network_module");

  //Import ngen_nwm_main
  this->t_route_module = py::module_::import("ngen_nwm_main");

  //Create object for the set_paths subroutine
  py::object set_paths = t_route_module.attr("set_paths");

  //Call set_paths subroutine
  set_paths(t_route_connection_path);

  std::vector<std::string> arg_vector;

  arg_vector.push_back("-f");

  arg_vector.push_back(t_route_config_file_with_path);

  //Cast vector of args to Python list 
  py::list arg_list = py::cast(arg_vector);

  //Create object for the main_v02 subroutine
  py::object main_v02 = t_route_module.attr("main_v02");

  //Call main_v02 subroutine
  main_v02(arg_list);

  //Commented out for now because not using these methods in t-route.
  //Might refactor to use them though.
  /*
  //Create object for ngen_routing subroutine
  py::object ngen_routing = t_route_module.attr("ngen_routing");

  //Call ngen_routing subroutine
  ngen_routing(number_of_timesteps, delta_time);

  //Create object for call_read_catchment_lateral_flows subroutine
  py::object call_read_catchment_lateral_flows = t_route_module.attr("call_read_catchment_lateral_flows");

  //Call call_read_catchment_lateral_flows subroutine
  call_read_catchment_lateral_flows(input_path);
  */

  std::cout << "Finished routing" << std::endl;

}


#endif //ACTIVATE_PYTHON
