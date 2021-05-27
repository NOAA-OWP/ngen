#ifndef NGEN_ROUTING_PY_ADAPTER_H
#define NGEN_ROUTING_PY_ADAPTER_H

#ifdef ACTIVATE_PYTHON

#include <exception>
#include <memory>
#include <string>
//#include "bmi.hxx"
#include "pybind11/pybind11.h"
#include "pybind11/pytypes.h"
#include "pybind11/numpy.h"
//#include "JSONProperty.hpp"
//#include "StreamHandler.hpp"


namespace py = pybind11;


class Routing_Py_Adapter {

public:

  Routing_Py_Adapter();



private:

  int empty_var;







};







#endif //ACTIVATE_PYTHON

#endif //NGEN_ROUTING_PY_ADAPTER_H
