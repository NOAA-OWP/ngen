#include <NGenConfig.h>

#if NGEN_WITH_PYTHON

#include <exception>
#include <utility>
#include <iostream>
#include "Routing_Py_Adapter.hpp"
#include <Logger.hpp>

std::stringstream routing_ss("");

using namespace routing_py_adapter;

Routing_Py_Adapter::Routing_Py_Adapter(std::string t_route_config_file_with_path):
  t_route_config_path(t_route_config_file_with_path){
  //hold a reference to the interpreter, ensures an interpreter exists as long as the reference is held
  interpreter = utils::ngenPy::InterpreterUtil::getInstance();
  //Import ngen_main.  Will throw error if module isn't available
  //in the embedded interpreters PYTHON_PATH
  try {
    LOG("Trying t-route module ngen_routing.ngen_main.", LogLevel::INFO);
    this->t_route_module = utils::ngenPy::InterpreterUtil::getPyModule("ngen_routing.ngen_main");
    LOG("Routing module ngen_routing.ngen_main detected and used. Use of this version is deprecated!", LogLevel::WARNING);
  }
  catch (const pybind11::error_already_set& e){
    try {
      LOG("Module ngen_routing.ngen_main not found; Trying different t-route module", LogLevel::WARNING);
      LOG("Trying t-route module nwm_routing.__main__.", LogLevel::INFO);
      // The legacy module has a `nwm_routing.__main__`, so we have to try this one second!
      this->t_route_module = utils::ngenPy::InterpreterUtil::getPyModule("nwm_routing.__main__");
      LOG("Routing module nwm_routing.__main__ detected and used.", LogLevel::INFO);
    }
    catch (const pybind11::error_already_set& e){
      LOG("Unable to import a supported routing module.", LogLevel::SEVERE);
      throw e;
    }
  }
}

void Routing_Py_Adapter::route(int number_of_timesteps, int delta_time,
                          const std::vector<double> &flow_vector){
  LOG("Routing_Py_Adapter::route overload with flow_vector unimplemented.", LogLevel::SEVERE);
  throw "Routing_Py_Adapter::route overload with flow_vector unimplemented.";
}

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
    LOG("Used t-route module ngen_main.", LogLevel::INFO);
  }
  catch (const pybind11::error_already_set& e){
    LOG("Using t-route module main_v04.", LogLevel::INFO);
    ngen_main = t_route_module.attr("main_v04");
  }

  ngen_main(arg_list);

  LOG("Finished routing", LogLevel::INFO);

}


#endif //NGEN_WITH_PYTHON
