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
  try {
    this->t_route_module = utils::ngenPy::InterpreterUtil::getPyModule("ngen_routing.ngen_main");
    std::cout<<"WARN: Legacy t-route module detected; use of this version is deprecated!"<<std::endl;
  }
  catch (const pybind11::error_already_set& e){
    try {
      // The legacy module has a `nwm_routing.__main__`, so we have to try this one second!
      this->t_route_module = utils::ngenPy::InterpreterUtil::getPyModule("nwm_routing.__main__");
    }
    catch (const pybind11::error_already_set& e){
      std::cerr<<"FAIL: Unable to import a supported routing module."<<std::endl;
      throw e;
    }
  }
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

  py::object ngen_main;
  try {
    // Try the legacy method first... this time because if we lose an exeption, we should favor one from the newer version.
    ngen_main = t_route_module.attr("ngen_main");
  }
  catch (const pybind11::error_already_set& e){
    ngen_main = t_route_module.attr("main_v04");
  }

  ngen_main(arg_list);

  std::cout << "Finished routing" << std::endl;

}


#endif //ACTIVATE_PYTHON
