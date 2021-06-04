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

    Routing_Py_Adapter(std::string t_route_connection_path, std::string input_path, 
                       const std::vector<std::string> &ids);


  private:

    int empty_var;




  };


}




#endif //ACTIVATE_PYTHON

#endif //NGEN_ROUTING_PY_ADAPTER_H
