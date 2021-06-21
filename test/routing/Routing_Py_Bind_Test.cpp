#ifdef ROUTING_PYBIND_TESTS_ACTIVE
#include "gtest/gtest.h"
#include "routing/Routing_Py_Adapter.hpp"
#include <string>
#include <pybind11/embed.h>
//#include <pybind11/stl.h>
namespace py = pybind11;

//using namespace std;


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

  std::vector<double> nexus_values_vec{1.1, 2.2, 3.3, 4.4, 5.5};

  //SET THESE AS INPUTS
  std::string t_route_connection_path = "../../t-route/src/ngen_routing/src";
  std::string input_path = "../../t-route/test/input/next_gen";
  std::string supernetwork = "../../t-route/test/input/next_gen/flowpath_data.geojson";

  std::vector<std::string> catchment_subset_ids;

  catchment_subset_ids.push_back("cat-71");
  catchment_subset_ids.push_back("cat-42");
  catchment_subset_ids.push_back("cat-34");

  int number_of_timesteps = 720;
  int delta_time = 3600; 

  routing_py_adapter::Routing_Py_Adapter routing_py_adapter1(t_route_connection_path, input_path, catchment_subset_ids, number_of_timesteps, delta_time);
  
  //routing_py_adapter::Routing_Py_Adapter routing_py_adapter1(t_route_connection_path, input_path, catchment_subset_ids, nexus_values_vec);

 
  ASSERT_TRUE(true);

}


#endif  // ROUTING_PYBIND_TESTS_ACTIVE

