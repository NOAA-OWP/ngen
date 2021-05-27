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

  //double a = 2.0;
  
  //EXPECT_DOUBLE_EQ(3.0, a);

  
  ASSERT_TRUE(true);


}


#endif  // ROUTING_PYBIND_TESTS_ACTIVE

