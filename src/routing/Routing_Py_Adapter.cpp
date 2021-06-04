#ifdef ACTIVATE_PYTHON

#include <exception>
#include <utility>

#include "Routing_Py_Adapter.hpp"
//#include "boost/algorithm/string.hpp"

using namespace routing_py_adapter;


Routing_Py_Adapter::Routing_Py_Adapter(std::string t_route_connection_path, std::string input_path,
                                       const std::vector<std::string> &ids)
{
    int a = 1;
  py::module_ sys = py::module_::import("sys");

  py::object sys_path = sys.attr("path");

  py::object sys_path_append = sys_path.attr("append");

  sys_path_append(t_route_connection_path);

  sys_path_append(input_path);

  py::module_ t_route_module = py::module_::import("next_gen_network_module");

  py::object set_paths = t_route_module.attr("set_paths");

  set_paths(t_route_connection_path);

}







#endif //ACTIVATE_PYTHON
