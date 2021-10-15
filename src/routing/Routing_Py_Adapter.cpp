#ifdef ACTIVATE_PYTHON

#include <exception>
#include <utility>

#include "Routing_Py_Adapter.hpp"

using namespace routing_py_adapter;


/**
 * Parameterized constructor Routing_Py_Adapter with the flow_vector
 *
 * @param t_route_connection_path
 * @param t_route_config_file_with_path
 * @param input_path
 * @param number_of_timesteps
 * @param delta_time
 * @param flow_vector
 */
Routing_Py_Adapter::Routing_Py_Adapter(std::string t_route_connection_path, 
                                       std::string t_route_config_file_with_path,
                                       std::string input_path,
                                       int number_of_timesteps, int delta_time,
                                       const std::vector<double> &flow_vector)
{}

/**
 * Parameterized constructor Routing_Py_Adapter without the flow_vector
 *
 * @param t_route_connection_path
 * @param t_route_config_file_with_path
 * @param number_of_timesteps
 * @param delta_time
 */
Routing_Py_Adapter::Routing_Py_Adapter(std::string t_route_connection_path,
                                       std::string t_route_config_file_with_path,
                                       int number_of_timesteps, int delta_time)
{

  //Bind python sys module
  py::module_ sys = py::module_::import("sys");

  //Create object sys_path for the Python system path
  py::object sys_path = sys.attr("path");

  //Create object to append to the sys_path
  py::object sys_path_append = sys_path.attr("append");

  //Append the t_route_connection_path
  utils::ngenPy::InterpreterUtil::addToPyPath(t_route_connection_path);

  //Import ngen_main
  this->t_route_module = py::module_::import("ngen_main");

  std::vector<std::string> arg_vector;

  arg_vector.push_back("-f");

  arg_vector.push_back(t_route_config_file_with_path);

  //Cast vector of args to Python list 
  py::list arg_list = py::cast(arg_vector);

  //Create object for the ngen_main subroutine
  py::object ngen_main = t_route_module.attr("ngen_main");

  //Call ngen_main subroutine
  ngen_main(arg_list);

  std::cout << "Finished routing" << std::endl;

}


#endif //ACTIVATE_PYTHON
