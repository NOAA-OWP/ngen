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

  py::print("Hello, World!"); // use the Python API


  std::vector<double> nexus_values_vec{1.1, 2.2, 3.3, 4.4, 5.5};
  
  int nexus_vec_size = nexus_values_vec.size();

  double* dest_ptr = NULL; // Pointer initialized with null

  //TODO: Check on best method for this. Might need extra memory
  int vec_byte_size = nexus_vec_size * 8; //currently for double

  //dest_ptr = new double;

  if( !(dest_ptr  = new double[nexus_vec_size] )) {  // Request memory for the variable
    std::cout << "Error: out of memory." << std::endl;
    exit(1);
  }

  //get_via_numpy_array<double>("nexus_values_vec", dest_ptr, NULL, nexus_vec_size, vec_byte_size, true);



  //SET THESE AS INPUTS
  std::string t_route_connection_path = "../../t-route/src/external_connections";
  std::string input_path = "../../t-route/test/input/next_gen";
  std::string supernetwork = "../../t-route/test/input/next_gen/flowpath_data.geojson";

  std::vector<std::string> catchment_subset_ids;
  //vector<string> catchment_subset_ids;

  catchment_subset_ids.push_back("cat-71");
  catchment_subset_ids.push_back("cat-42");
  catchment_subset_ids.push_back("cat-34");

  routing_py_adapter::Routing_Py_Adapter routing_py_adapter1(t_route_connection_path, input_path, catchment_subset_ids, nexus_values_vec);

  std::vector<double> test_vec{1, 2, 3, 4, 5};


  py::list test_list3 = py::cast(catchment_subset_ids);
  
  //py::list test_list3 = py::cast(test_vec);

  py::print(test_list3);  

  /*
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
  */

 
  ASSERT_TRUE(true);

  delete dest_ptr; // free up the memory

}


#endif  // ROUTING_PYBIND_TESTS_ACTIVE

