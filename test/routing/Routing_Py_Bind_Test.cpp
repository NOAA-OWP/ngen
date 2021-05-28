#ifdef ROUTING_PYBIND_TESTS_ACTIVE
#include "gtest/gtest.h"
#include "routing/Routing_Py_Adapter.hpp"

#include <pybind11/embed.h>
namespace py = pybind11;

class RoutingPyBindTest : public ::testing::Test {

protected:

    RoutingPyBindTest() {

    }

    ~RoutingPyBindTest() override {

    }
};

TEST_F(RoutingPyBindTest, TestRoutingPyBind)
{
  // Start Python interpreter and keep it alive
  py::scoped_interpreter guard{};


  py::print("Hello, World!"); // use the Python API

  py::module_ sys = py::module_::import("sys");
  py::print(sys.attr("path"));

  py::object sys_path = sys.attr("path"); 
  //py::print(sys_path);

  py::object sys_path_append = sys_path.attr("append");

  sys_path_append("/glade/work/dmattern/t-route/src/external_connections");
  //sys_path_append("/glade/work/dmattern/t-route");

  sys_path_append("/glade/work/dmattern/t-route/src/python_framework_v02");
  sys_path_append("/glade/work/dmattern/t-route/src/python_routing_v02");
  sys_path_append("/glade/work/dmattern/t-route/test/input/next_gen");

 
  //py::exec(sys_path.attr("append('/glade/work/dmattern/t-route/src/external_connections')"));
  //py::exec(sys_path.attr("append('/glade/work/dmattern/t-route/src/external_connections')"));



 //py::exec(sys.attr("path").attr("append('/glade/work/dmattern/t-route/src/external_connections')"));

  //py::print(sys.insert(1, "/glade/work/dmattern/t-route/src/external_connections"));
  //py::print(sys.attr("path")(1, "/glade/work/dmattern/t-route/src/external_connections"));

  //py::eval("sys.path.append('/glade/work/dmattern/t-route/src/external_connections')");
  //py::eval("sys.path.append('/glade/work/dmattern/t-route/src/external_connections')");
  

  //py::sys.path.insert(1, "/glade/work/dmattern/t-route/src/external_connections");
  //py::exec(sys.path.insert(1, "/glade/work/dmattern/t-route/src/external_connections"));

  //py::module_ t_route = py::module_::import("/glade/work/dmattern/t-route/src/external_connections/next_gen_network_module");
 
  

  //Use this
  py::module_ t_route_module = py::module_::import("next_gen_network_module");
 
  py::print(sys.attr("path"));

  
  ASSERT_TRUE(true);


}


#endif  // ROUTING_PYBIND_TESTS_ACTIVE

