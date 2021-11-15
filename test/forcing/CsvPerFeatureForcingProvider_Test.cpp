#include <vector>
#include "gtest/gtest.h"
#include "CsvPerFeatureForcingProvider.hpp"
#include "FileChecker.h"
#include <memory>
#include <vector>
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <ctime>
#include <time.h>

class CsvPerFeatureForcingProviderTest : public ::testing::Test {

    protected:

    void SetUp() override;

    void TearDown() override;

    void setupForcing();

    std::shared_ptr<CsvPerFeatureForcingProvider> Forcing_Object;
    std::shared_ptr<CsvPerFeatureForcingProvider> Forcing_Object_2;

    typedef struct tm time_type;

    std::shared_ptr<time_type> start_date_time;

    std::shared_ptr<time_type> end_date_time;

};


void CsvPerFeatureForcingProviderTest::SetUp() {
    //setupForcing();

    setupForcing();
}


void CsvPerFeatureForcingProviderTest::TearDown() {
}


//Construct a forcing object
void CsvPerFeatureForcingProviderTest::setupForcing()
{
    std::vector<std::string> forcing_file_names = { 
        "test/data/forcing/cat-10_2015-12-01 00_00_00_2015-12-30 23_00_00.csv",
        "../test/data/forcing/cat-10_2015-12-01 00_00_00_2015-12-30 23_00_00.csv",
        "../../test/data/forcing/cat-10_2015-12-01 00_00_00_2015-12-30 23_00_00.csv"
        };
    std::string forcing_file_name = utils::FileChecker::find_first_readable(forcing_file_names);

    forcing_params forcing_p(forcing_file_name, "CsvPerFeature", "2015-12-14 21:00:00", "2015-12-30 23:00:00");

    Forcing_Object = std::make_shared<CsvPerFeatureForcingProvider>(forcing_p);

    forcing_file_names = { 
        "test/data/forcing/cat-27115-nwm-aorc-variant-derived-format.csv",
        "../test/data/forcing/cat-27115-nwm-aorc-variant-derived-format.csv",
        "../../test/data/forcing/cat-27115-nwm-aorc-variant-derived-format.csv"
        };
    forcing_file_name = utils::FileChecker::find_first_readable(forcing_file_names);

    forcing_params forcing_p_2(forcing_file_name, "CsvPerFeature", "2015-12-01 00:00:00", "2015-12-05 02:00:00");

    Forcing_Object_2 = std::make_shared<CsvPerFeatureForcingProvider>(forcing_p_2);
}

///Test AORC Forcing Object
TEST_F(CsvPerFeatureForcingProviderTest, TestForcingDataRead)
{
    double current_precipitation;

    time_t begin = Forcing_Object->get_forcing_output_time_begin("");

    int current_day_of_year;
    int i = 65;
    time_t t = begin+(i*3600);
    std::cerr << std::ctime(&t) << std::endl;

    current_precipitation = Forcing_Object->get_value(CSDMS_STD_NAME_LIQUID_EQ_PRECIP_RATE, begin+(i*3600), 3600, "");

    EXPECT_NEAR(current_precipitation, 7.9999999999999996e-07, 0.00000005);

    double temp_k = Forcing_Object->get_value(CSDMS_STD_NAME_SURFACE_TEMP, begin+(i*3600), 3600, "");

    EXPECT_NEAR(temp_k, 286.9, 0.00001);

    int current_epoch;

    i = 387;
    current_precipitation = Forcing_Object->get_value(CSDMS_STD_NAME_LIQUID_EQ_PRECIP_RATE, begin+(i*3600), 3600, "");

    EXPECT_NEAR(current_precipitation, 6.9999999999999996e-07, 0.00000005);

    //Check exceeding the forcing range to retrieve the last forcing precipation rate
    i = 388;
    current_precipitation = Forcing_Object->get_value(CSDMS_STD_NAME_LIQUID_EQ_PRECIP_RATE, begin+(i*3600), 3600, "");

    EXPECT_NEAR(current_precipitation, 6.9999999999999996e-07, 0.00000005);
}

TEST_F(CsvPerFeatureForcingProviderTest, TestForcingDataReadAltFormat)
{
    double current_precipitation;

    time_t begin = Forcing_Object_2->get_forcing_output_time_begin("");

    int current_day_of_year;
    int i = 8;
    time_t t = begin+(i*3600);
    std::cerr << std::ctime(&t) << std::endl;

    current_precipitation = Forcing_Object_2->get_value(CSDMS_STD_NAME_LIQUID_EQ_PRECIP_RATE, begin+(i*3600), 3600, "");

    EXPECT_NEAR(current_precipitation, 0.00032685, 0.00000001);

    double temp_k = Forcing_Object_2->get_value(CSDMS_STD_NAME_SURFACE_TEMP, begin+(i*3600), 3600, "");

    EXPECT_NEAR(temp_k, 265.77, 0.00001);

    int current_epoch;

    i = 34;
    current_precipitation = Forcing_Object_2->get_value(CSDMS_STD_NAME_LIQUID_EQ_PRECIP_RATE, begin+(i*3600), 3600, "");

    EXPECT_NEAR(current_precipitation, 0.00013539, 0.00000001);

}


TEST_F(CsvPerFeatureForcingProviderTest, TestForcingDataUnitConversion)
{
    double current_precipitation;

    time_t begin = Forcing_Object->get_forcing_output_time_begin("");

    int current_day_of_year;
    int i = 65;
    time_t t = begin+(i*3600);
    std::cerr << std::ctime(&t) << std::endl;

    current_precipitation = Forcing_Object->get_value(CSDMS_STD_NAME_LIQUID_EQ_PRECIP_RATE, begin+(i*3600), 3600, "");

    EXPECT_NEAR(current_precipitation, 7.9999999999999996e-07, 0.00000005);

    double temp_f = Forcing_Object->get_value(CSDMS_STD_NAME_SURFACE_TEMP, begin+(i*3600), 3600, "degF");

    EXPECT_NEAR(temp_f, 56.749989014, 0.00001);

}

///Test AORC Forcing Object
TEST_F(CsvPerFeatureForcingProviderTest, TestGetAvailableForcingOutputs)
{
    const std::vector<std::string>& afos = Forcing_Object->get_available_forcing_outputs();
    EXPECT_EQ(afos.size(), 18);
    EXPECT_TRUE(std::find(afos.begin(), afos.end(), "DLWRF_surface") != afos.end());
    EXPECT_TRUE(std::find(afos.begin(), afos.end(), CSDMS_STD_NAME_SOLAR_LONGWAVE) != afos.end());

}

