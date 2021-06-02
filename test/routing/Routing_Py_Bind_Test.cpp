#ifdef ROUTING_PYBIND_TESTS_ACTIVE
#include "gtest/gtest.h"
#include "routing/Routing_Py_Adapter.hpp"
#include <string>
#include <pybind11/embed.h>
namespace py = pybind11;

using namespace std;


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

  py::object sys_path_append = sys_path.attr("append");


  ////////////////
  //SET THESE AS INPUTS
  string t_route_connection_path = "../../t-route/src/external_connections";
  string input_path = "../../t-route/test/input/next_gen";


  //USE THIS
  sys_path_append(t_route_connection_path);
  sys_path_append(input_path);
  ////////////////////

  //Use this
  py::module_ t_route_module = py::module_::import("next_gen_network_module");
 
  py::print(sys.attr("path"));

  py::object test_add = t_route_module.attr("test_add"); 

  ////////////////////
  int c = 1;
  int d = 2;
  int e = 0;
  //e = test_add("c,d");
  //e = test_add(c,d);
  //py::print(e);
  py::print(test_add(c,d)); //WORKS
  ////////////////

  py::object set_paths = t_route_module.attr("set_paths"); 

  set_paths(t_route_connection_path);





 
  ASSERT_TRUE(true);


}


#endif  // ROUTING_PYBIND_TESTS_ACTIVE

