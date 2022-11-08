#ifdef ACTIVATE_PYTHON

#include <exception>
#include <utility>
#include <iostream>
#include "Routing_Py_Adapter.hpp"

using namespace routing_py_adapter;

Routing_Py_Adapter::Routing_Py_Adapter(std::string t_route_config_file_with_path):
  t_route_config_path(t_route_config_file_with_path){
  //hold a reference to the interpreter, ensures an interperter exists as long as the reference is held
  interperter = utils::ngenPy::InterpreterUtil::getInstance();
  //Import ngen_main.  Will throw error if module isn't available
  //in the embedded interperters PYTHON_PATH
  this->t_route_module = utils::ngenPy::InterpreterUtil::getPyModule("ngen_routing.ngen_main");
  }

void Routing_Py_Adapter::route(int number_of_timesteps, int delta_time,
                          const std::vector<double> &flow_vector){
  throw "Routing_Py_Adapter::route overload with flow_vector unimplemented.";
};

void Routing_Py_Adapter::route(int number_of_timesteps, int delta_time)
{

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
