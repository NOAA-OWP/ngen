#include <NGenConfig.h>

#if NGEN_WITH_NETCDF
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

    std::string forcing_file_name;
    std::unique_ptr<forcing_params> forcing_p;
};

void NetCDFPerFeatureDataProviderTest::SetUp() {
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
    forcing_file_name = utils::FileChecker::find_first_readable(forcing_file_names);

    // Using this to compute epoch times... this is what's done in Formulation_Constructors.hpp, FWIW...
    forcing_p = std::make_unique<forcing_params>(forcing_file_name, "NetCDF", "2015-12-01 00:00:00", "2015-12-30 23:00:00");
}

///Test AORC Forcing Object
TEST_F(NetCDFPerFeatureDataProviderTest, TestForcingDataReadUsingIds)
{
    std::vector<std::string> ids = {"cat-27", "cat-52", "cat-67"};

    NetCDFPerFeatureDataProvider nc_provider(forcing_file_name, forcing_p->simulation_start_t, forcing_p->simulation_end_t, utils::getStdErr() );
    for(const std::string& id: ids){
        nc_provider.hint_shared_provider_id(id);
    }

    auto start_time = nc_provider.get_data_start_time();
    auto duration = nc_provider.record_duration();

    double tol = 0.00002;
    std::array<double, 3> expected = {285.8, 285.9, 285.7};
    size_t i = 0;
    // read exactly one time step correctly aligned
    for (const double expect: expected){
        double result = nc_provider.get_value(CatchmentAggrDataSelector(ids[i], CSDMS_STD_NAME_SURFACE_TEMP, start_time, duration, "K"), data_access::MEAN);
        EXPECT_NEAR(result, expect, tol);
        i++;
    }

    ASSERT_EQ(ids, nc_provider.get_ids());
}

TEST_F(NetCDFPerFeatureDataProviderTest, TestForcingDataReadUsingIdsIsNotReverse)
{
    std::vector<std::string> ids = {"cat-27", "cat-52", "cat-67"};
    auto rev = ids;
    std::reverse(rev.begin(), rev.end());

    NetCDFPerFeatureDataProvider nc_provider(forcing_file_name, forcing_p->simulation_start_t, forcing_p->simulation_end_t, utils::getStdErr() );
    for(const std::string& id: ids){
        nc_provider.hint_shared_provider_id(id);
    }

    auto start_time = nc_provider.get_data_start_time();
    auto duration = nc_provider.record_duration();

    double tol = 0.00002;
    std::array<double, 3> expected = {285.8, 285.9, 285.7};
    size_t i = 0;
    // read exactly one time step correctly aligned
    for (const double expect: expected){
        double result = nc_provider.get_value(CatchmentAggrDataSelector(ids[i], CSDMS_STD_NAME_SURFACE_TEMP, start_time, duration, "K"), data_access::MEAN);
        EXPECT_NEAR(result, expect, tol);
        i++;
    }
    auto ids_out = nc_provider.get_ids();
    ASSERT_NE(rev, ids_out);
    ASSERT_EQ(ids, ids_out);
}

TEST_F(NetCDFPerFeatureDataProviderTest, TestForcingDataReadUsingIdsSingle)
{
    std::vector<std::string> ids = {"cat-52"};
    NetCDFPerFeatureDataProvider nc_provider(forcing_file_name, forcing_p->simulation_start_t, forcing_p->simulation_end_t, utils::getStdErr() );
    nc_provider.hint_shared_provider_id(ids[0]);

    auto start_time = nc_provider.get_data_start_time();
    auto duration = nc_provider.record_duration();

    double tol = 0.00002;
    double value = nc_provider.get_value(CatchmentAggrDataSelector(ids[0], CSDMS_STD_NAME_SURFACE_TEMP, start_time, duration, "K"), data_access::MEAN);
    EXPECT_NEAR(value, 285.9, tol);
    ASSERT_EQ(ids, nc_provider.get_ids());
}

TEST_F(NetCDFPerFeatureDataProviderTest, TestForcingDataReadSpan)
{
    std::vector<std::string> ids = {"cat-52", "cat-67"};
    NetCDFPerFeatureDataProvider nc_provider(forcing_file_name, forcing_p->simulation_start_t, forcing_p->simulation_end_t, utils::getStdErr() );
    for (const std::string& id: ids){
        nc_provider.hint_shared_provider_id(id);
    }

    auto start_time = nc_provider.get_data_start_time();
    auto duration = nc_provider.record_duration();

    double tol = 0.00002;
    std::array<double, 2> expected = {285.9, 285.7};
    size_t i = 0;
    // read exactly one time step correctly aligned
    for (const double expect: expected){
        double result = nc_provider.get_value(CatchmentAggrDataSelector(ids[i], CSDMS_STD_NAME_SURFACE_TEMP, start_time, duration, "K"), data_access::MEAN);
        EXPECT_NEAR(result, expect, tol);
        i++;
    }
    ASSERT_EQ(ids, nc_provider.get_ids());
}

TEST_F(NetCDFPerFeatureDataProviderTest, TestForcingDataReadMultiSpan)
{
    std::vector<std::string> ids = {"cat-27", "cat-67"};
    NetCDFPerFeatureDataProvider nc_provider(forcing_file_name, forcing_p->simulation_start_t, forcing_p->simulation_end_t, utils::getStdErr() );
    for (const std::string& id: ids){
        nc_provider.hint_shared_provider_id(id);
    }

    auto start_time = nc_provider.get_data_start_time();
    auto duration = nc_provider.record_duration();

    double tol = 0.00002;
    std::array<double, 2> expected = {285.8, 285.7};
    size_t i = 0;
    // read exactly one time step correctly aligned
    for (const double expect: expected){
        double result = nc_provider.get_value(CatchmentAggrDataSelector(ids[i], CSDMS_STD_NAME_SURFACE_TEMP, start_time, duration, "K"), data_access::MEAN);
        EXPECT_NEAR(result, expect, tol);
        i++;
    }
    ASSERT_EQ(ids, nc_provider.get_ids());
}

///Test AORC Forcing Object
TEST_F(NetCDFPerFeatureDataProviderTest, TestForcingDataRead)
{
    NetCDFPerFeatureDataProvider nc_provider(forcing_file_name, forcing_p->simulation_start_t, forcing_p->simulation_end_t, utils::getStdErr() );

    // check to see that the variable "T2D" exists
    auto var_names = nc_provider.get_available_variable_names();
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

    auto start_time = nc_provider.get_data_start_time();
    auto ids = nc_provider.get_ids();
    auto duration = nc_provider.record_duration();

    //std::cout << "Checking values in catchment "<<ids[0]<<" at time "<<start_time<<" with duration "<<duration<<"..."<<std::endl;

    // read exactly one time step correctly aligned
    double val1 = nc_provider.get_value(CatchmentAggrDataSelector(ids[0], CSDMS_STD_NAME_SURFACE_TEMP, start_time, duration, "K"), data_access::MEAN);

    //double tol = 0.00000612;
    double tol = 0.00002;

    EXPECT_NEAR(val1, 285.8, tol);

    // read 1/2 of a time step correctly aligned
    double val2 = nc_provider.get_value(CatchmentAggrDataSelector(ids[0], CSDMS_STD_NAME_SURFACE_TEMP, start_time, duration / 2, "K"), data_access::MEAN);

    EXPECT_NEAR(val2, 285.8, tol);

    // read 4 time steps correctly aligned
    double val3 = nc_provider.get_value(CatchmentAggrDataSelector(ids[0], CSDMS_STD_NAME_SURFACE_TEMP, start_time, duration * 4, "K"), data_access::MEAN);

    EXPECT_NEAR(val3, 284.95, tol);

    // read exactly one time step correctly aligned but with a incorrect variable
    EXPECT_THROW(
        double val4 = nc_provider.get_value(CatchmentAggrDataSelector(ids[0], "T3D", start_time, duration, "K"), data_access::MEAN);,
        std::runtime_error);
    
}
#endif
