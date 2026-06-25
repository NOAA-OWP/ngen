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
    auto var_names = nc_provider->get_available_variable_names();
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
    double val1 = nc_provider->get_value(CatchmentAggrDataSelector(ids[0], CSDMS_STD_NAME_SURFACE_TEMP, start_time, duration, "K"), data_access::MEAN);

    //double tol = 0.00000612;
    double tol = 0.00002;

    EXPECT_NEAR(val1, 285.8, tol);

    // read 1/2 of a time step correctly aligned
    double val2 = nc_provider->get_value(CatchmentAggrDataSelector(ids[0], CSDMS_STD_NAME_SURFACE_TEMP, start_time, duration / 2, "K"), data_access::MEAN);

    EXPECT_NEAR(val2, 285.8, tol);

    // read 4 time steps correctly aligned
    double val3 = nc_provider->get_value(CatchmentAggrDataSelector(ids[0], CSDMS_STD_NAME_SURFACE_TEMP, start_time, duration * 4, "K"), data_access::MEAN);

    EXPECT_NEAR(val3, 284.95, tol);

    // read exactly one time step correctly aligned but with a incorrect variable
    EXPECT_THROW(
        double val4 = nc_provider->get_value(CatchmentAggrDataSelector(ids[0], "T3D", start_time, duration, "K"), data_access::MEAN);,
        std::runtime_error);

}

// Each recognized time `units` token maps to the expected unit and scale factor,
// and a bare units string reports no reference epoch.
TEST(NetCDFTimeMetadata, InterpretTimeUnitsRecognized)
{
    using P = NetCDFPerFeatureDataProvider;
    struct Case { const char* str; P::TimeUnit unit; double scale; };
    const std::vector<Case> cases = {
        {"h",            P::TIME_HOURS,        3600},
        {"hours",        P::TIME_HOURS,        3600},
        {"m",            P::TIME_MINUTES,      60},
        {"minutes",      P::TIME_MINUTES,      60},
        {"s",            P::TIME_SECONDS,      1},
        {"seconds",      P::TIME_SECONDS,      1},
        {"ms",           P::TIME_MILLISECONDS, .001},
        {"milliseconds", P::TIME_MILLISECONDS, .001},
        {"us",           P::TIME_MICROSECONDS, .000001},
        {"microseconds", P::TIME_MICROSECONDS, .000001},
        {"ns",           P::TIME_NANOSECONDS,  .000000001},
        {"nanoseconds",  P::TIME_NANOSECONDS,  .000000001},
    };
    for (const auto& c : cases) {
        auto info = P::interpret_time_units(c.str);
        ASSERT_TRUE(info.has_value()) << "expected to recognize unit '" << c.str << "'";
        EXPECT_EQ(info->unit, c.unit) << "unit mismatch for '" << c.str << "'";
        EXPECT_DOUBLE_EQ(info->scale_factor, c.scale) << "scale mismatch for '" << c.str << "'";
        EXPECT_FALSE(info->epoch_start_time.has_value()) << "bare units must not set an epoch for '" << c.str << "'";
    }
}

// CF "<unit> since <date>" units yield the base unit plus the embedded reference epoch.
TEST(NetCDFTimeMetadata, InterpretTimeUnitsCFSinceEpoch)
{
    using P = NetCDFPerFeatureDataProvider;
    auto secs = P::interpret_time_units("seconds since 1970-01-01 00:00:00");
    ASSERT_TRUE(secs.has_value());
    EXPECT_EQ(secs->unit, P::TIME_SECONDS);
    EXPECT_DOUBLE_EQ(secs->scale_factor, 1);
    ASSERT_TRUE(secs->epoch_start_time.has_value());
    EXPECT_EQ(*secs->epoch_start_time, 0);

    auto hrs = P::interpret_time_units("hours since 2000-01-01 00:00:00");
    ASSERT_TRUE(hrs.has_value());
    EXPECT_EQ(hrs->unit, P::TIME_HOURS);
    EXPECT_DOUBLE_EQ(hrs->scale_factor, 3600);
    ASSERT_TRUE(hrs->epoch_start_time.has_value());
    EXPECT_EQ(*hrs->epoch_start_time, 946684800);
}

// Unrecognized or empty unit strings yield no value, so callers keep their defaults.
TEST(NetCDFTimeMetadata, InterpretTimeUnitsUnrecognized)
{
    using P = NetCDFPerFeatureDataProvider;
    EXPECT_FALSE(P::interpret_time_units("").has_value());
    EXPECT_FALSE(P::interpret_time_units("days").has_value());
    EXPECT_FALSE(P::interpret_time_units("fortnights").has_value());
}

// parse_epoch converts a timestamp to UTC epoch seconds and honors the supplied format.
TEST(NetCDFTimeMetadata, ParseEpoch)
{
    using P = NetCDFPerFeatureDataProvider;
    // unambiguous 4-digit-year format
    EXPECT_EQ(P::parse_epoch("1970-01-01 00:00:00", "%Y-%m-%d %H:%M:%S"), 0);
    EXPECT_EQ(P::parse_epoch("1970-01-02 00:00:00", "%Y-%m-%d %H:%M:%S"), 86400);
    EXPECT_EQ(P::parse_epoch("2000-01-01 00:00:00", "%Y-%m-%d %H:%M:%S"), 946684800);
    // the provider's default epoch string and format
    EXPECT_EQ(P::parse_epoch("01/01/1970 00:00:00", "%D %T"), 0);
    // a timestamp that doesn't match the format is an error, not a silent zero
    EXPECT_THROW(P::parse_epoch("not a date", "%Y-%m-%d %H:%M:%S"), std::runtime_error);
}
#endif
