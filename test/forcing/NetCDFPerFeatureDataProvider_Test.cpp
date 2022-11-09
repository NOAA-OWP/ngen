#ifdef NETCDF_ACTIVE
#include <vector>
#include "gtest/gtest.h"
#include "NetCDFPerFeatureDataProvider.hpp"
#include "StreamHandler.hpp"
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

    std::shared_ptr<data_access::NetCDFPerFeatureDataProvider> nc_provider;

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
    std::vector<std::string> forcing_file_names = { 
        "data/forcing/cats-27_52_67-2015_12_01-2015_12_30.nc",
        "../data/forcing/cats-27_52_67-2015_12_01-2015_12_30.nc",
        "../../data/forcing/cats-27_52_67-2015_12_01-2015_12_30.nc"
        };
    std::string forcing_file_name = utils::FileChecker::find_first_readable(forcing_file_names);

    // Using this to compute epoch times... this is what's done in Formulation_Constructors.hpp, FWIW...
    forcing_params forcing_p(forcing_file_name, "NetCDF", "2015-12-01 00:00:00", "2015-12-30 23:00:00");

    nc_provider = std::make_shared<data_access::NetCDFPerFeatureDataProvider>(forcing_file_name, forcing_p.simulation_start_t, forcing_p.simulation_end_t, utils::getStdErr() );
    start_date_time = std::make_shared<time_type>();
    end_date_time = std::make_shared<time_type>();
}

///Test AORC Forcing Object
TEST_F(NetCDFPerFeatureDataProviderTest, TestForcingDataRead)
{
    // check to see that the variable "T2D" exists
    auto var_names = nc_provider->get_avaliable_variable_names();
    auto pos = std::find(var_names.begin(), var_names.end(), CSDMS_STD_NAME_SURFACE_TEMP);
    if ( pos != var_names.end() )
    {
        //std::cout << "Found variable "<<CSDMS_STD_NAME_SURFACE_TEMP<<"\n";
    }
    else
    {
        //std::cout << "The variable "<<CSDMS_STD_NAME_SURFACE_TEMP<<" is missing\n";
        FAIL();
    }

    auto start_time = nc_provider->get_data_start_time();
    auto ids = nc_provider->get_ids();
    auto duration = nc_provider->record_duration();

    //std::cout << "Checking values in catchment "<<ids[0]<<" at time "<<start_time<<" with duration "<<duration<<"..."<<std::endl;

    // read exactly one time step correctly aligned
    double val1 = nc_provider->get_value(NetCDFDataSelector(ids[0], CSDMS_STD_NAME_SURFACE_TEMP, start_time, duration, "K"), data_access::MEAN);

    //double tol = 0.00000612;
    double tol = 0.00002;

    EXPECT_NEAR(val1, 285.8, tol);

    // read 1/2 of a time step correctly aligned
    double val2 = nc_provider->get_value(NetCDFDataSelector(ids[0], CSDMS_STD_NAME_SURFACE_TEMP, start_time, duration / 2, "K"), data_access::MEAN);

    EXPECT_NEAR(val2, 285.8, tol);

    // read 4 time steps correctly aligned
    double val3 = nc_provider->get_value(NetCDFDataSelector(ids[0], CSDMS_STD_NAME_SURFACE_TEMP, start_time, duration * 4, "K"), data_access::MEAN);

    EXPECT_NEAR(val3, 284.95, tol);

    // read exactly one time step correctly aligned but with a incorrect variable
    EXPECT_THROW(
        double val4 = nc_provider->get_value(NetCDFDataSelector(ids[0], "T3D", start_time, duration, "K"), data_access::MEAN);, 
        std::runtime_error);
    
}
#endif
