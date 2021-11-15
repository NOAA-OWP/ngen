#ifdef ACTIVATE_PYTHON

#include <exception>
#include <utility>

#include "Routing_Py_Adapter.hpp"

using namespace routing_py_adapter;


void Routing_Py_Adapter::route(int number_of_timesteps, int delta_time,
                          const std::vector<double> &flow_vector){
  throw "Routing_Py_Adapter::route overload with flow_vector unimplemented.";
};

/**
 * Parameterized constructor Routing_Py_Adapter without the flow_vector
 *
 * @param t_route_connection_path
 * @param t_route_config_file_with_path
 * @param number_of_timesteps
 * @param delta_time
 */
void Routing_Py_Adapter::route(int number_of_timesteps, int delta_time)
{

  //Append the t_route_connection_path
  utils::ngenPy::InterpreterUtil::addToPyPath(this->t_route_module_path);

  //Import ngen_main
  this->t_route_module = utils::ngenPy::InterpreterUtil::getPyModule("ngen_main");//py::module_::import("ngen_main");

  std::vector<std::string> arg_vector;

  arg_vector.push_back("-f");

  arg_vector.push_back(this->t_route_config_path);

  //Cast vector of args to Python list 
  py::list arg_list = py::cast(arg_vector);

  //Create object for the ngen_main subroutine
  py::object ngen_main = t_route_module.attr("ngen_main");

  //Call ngen_main subroutine
  ngen_main(arg_list);

  std::cout << "Finished routing" << std::endl;

}


#endif //ACTIVATE_PYTHON
