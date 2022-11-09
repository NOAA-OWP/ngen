#ifdef ROUTING_PYBIND_TESTS_ACTIVE
#include "gtest/gtest.h"
#include "Routing_Py_Adapter.hpp"
#include <string>
#include <pybind11/embed.h>
#include <pybind11/stl.h>
namespace py = pybind11;

//using namespace std;


class RoutingPyBindTest : public ::testing::Test {
private:
  static std::shared_ptr<utils::ngenPy::InterpreterUtil> interperter;

protected:

    RoutingPyBindTest() {

    }

    ~RoutingPyBindTest() override {

    }
};
//Make sure the interperter is instansiated and lives throught the test class
std::shared_ptr<utils::ngenPy::InterpreterUtil> RoutingPyBindTest::interperter = utils::ngenPy::InterpreterUtil::getInstance();

TEST_F(RoutingPyBindTest, TestRoutingPyBind)
{

  //Use below if calling constructor that includes nexus values
  //std::vector<double> nexus_values_vec{1.1, 2.2, 3.3, 4.4, 5.5};

  //SET THESE AS INPUTS
  std::string t_route_config_file_with_path = "./test/data/routing/ngen_routing_config_unit_test.yaml";

  //Note: Currently, delta_time is set in the t-route yaml configuration file, and the
  //number_of_timesteps is determined from the total number of nexus outputs in t-rout
  //It is recommended to still pass these values to the routing_py_adapter object in
  //case a future implmentation needs these two values from the ngen framework.
  int number_of_timesteps = 10;
  int delta_time = 3600; 

  routing_py_adapter::Routing_Py_Adapter routing_py_adapter(t_route_config_file_with_path);
  routing_py_adapter.route(number_of_timesteps, delta_time);
 
  ASSERT_TRUE(true);

}


#endif  // ROUTING_PYBIND_TESTS_ACTIVE

