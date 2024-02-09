#include "gtest/gtest.h"
#include "Simulation_Time.hpp"
#include <memory>
#include <vector>
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <ctime>
#include <time.h>

#include "output/NetcdfCatchmentOutputWriter.hpp"

class NetcdfOuputTest : public ::testing::Test {

    protected:

    void SetUp() override;

    void TearDown() override;
};


void NetcdfOuputTest::SetUp() {

}

void NetcdfOuputTest::TearDown() {
}

///Test simulation time Object
TEST_F(NetcdfOuputTest, TestNetcdfCreation)
{
    data_output::NetcdfOutputWriter("netcdf_output_test.nc");

    SUCCEED();
}





