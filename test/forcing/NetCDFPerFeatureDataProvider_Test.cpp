#include <vector>
#include "gtest/gtest.h"
#include "NetCDFPerFeatureDataProvider.hpp"
#include "FileChecker.h"
#include <memory>
#include <vector>
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <ctime>
#include <time.h>


using data_access::NetCDFPerFeatureDataProvider;

class NetCDFPerFeatureDataProviderTest : public ::testing::Test {

    protected:

    void SetUp() override;

    void TearDown() override;

    void setupForcing();

    std::shared_ptr<data_access::NetCDFPerFeatureDataProvider> Forcing_Object;

    typedef struct tm time_type;

    std::shared_ptr<time_type> start_date_time;

    std::shared_ptr<time_type> end_date_time;

};

void NetCDFPerFeatureDataProviderTest::SetUp() {
    //setupForcing();

    setupForcing();
}


void NetCDFPerFeatureDataProviderTest::TearDown() 
{

}

//Construct a forcing object
void NetCDFPerFeatureDataProviderTest::setupForcing()
{
    Forcing_Object = std::make_shared<data_access::NetCDFPerFeatureDataProvider>("/local/ngen/data/huc01/huc_01/forcing/netcdf/huc01.nc");
    start_date_time = std::make_shared<time_type>();
    end_date_time = std::make_shared<time_type>();
}

///Test AORC Forcing Object
TEST_F(NetCDFPerFeatureDataProviderTest, TestForcingDataRead)
{
    std::shared_ptr<NetCDFPerFeatureDataProvider> nc_provider = std::make_shared<NetCDFPerFeatureDataProvider>("/local/ngen/data/huc01/huc_01/forcing/netcdf/huc01.nc");

    // check to see that the variable "T2D" exists
    auto var_names = nc_provider->get_avaliable_variable_names();
    auto pos = std::find(var_names.begin(), var_names.end(), "T2D");

    if ( pos != var_names.end() )
    {
        std::cout << "Found variable T2D\n";
    }
    else
    {
        std::cout << "The variable \"T2D\" is missing\n";
        FAIL();
    }

    
    //double val = Forcing_Object->get_value("cat-10095","RAINRATE",0,1,"mm/h");
    //EXPECT_NEAR(current_precipitation, 6.9999999999999996e-07, 0.00000005);
}
