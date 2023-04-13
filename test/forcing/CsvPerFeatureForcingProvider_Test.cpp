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
    std::shared_ptr<CsvPerFeatureForcingProvider> Forcing_Object_3; // explicit units

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

    forcing_file_names = { 
        "test/data/forcing/cat-27115-nwm-aorc-variant-derived-format-units.csv",
        "../test/data/forcing/cat-27115-nwm-aorc-variant-derived-format-units.csv",
        "../../test/data/forcing/cat-27115-nwm-aorc-variant-derived-format-units.csv"
    };
    forcing_file_name = utils::FileChecker::find_first_readable(forcing_file_names);

    forcing_params forcing_p_3(forcing_file_name, "CsvPerFeature", "2015-12-01 00:00:00", "2015-12-05 02:00:00");

    Forcing_Object_3 = std::make_shared<CsvPerFeatureForcingProvider>(forcing_p_3);
}

///Test AORC Forcing Object
TEST_F(CsvPerFeatureForcingProviderTest, TestForcingDataRead)
{
    double current_precipitation;

    time_t begin = Forcing_Object->get_data_start_time();

    int current_day_of_year;
    int i = 65;
    time_t t = begin+(i*3600);
    std::cerr << std::ctime(&t) << std::endl;

    current_precipitation = Forcing_Object->get_value(CSVDataSelector(CSDMS_STD_NAME_LIQUID_EQ_PRECIP_RATE, begin+(i*3600), 3600, ""), data_access::SUM);

    EXPECT_NEAR(current_precipitation, 7.9999999999999996e-07, 0.00000005);

    double temp_k = Forcing_Object->get_value(CSVDataSelector(CSDMS_STD_NAME_SURFACE_TEMP, begin+(i*3600), 3600, ""), data_access::MEAN);

    EXPECT_NEAR(temp_k, 286.9, 0.00001);

    int current_epoch;

    i = 387;
    current_precipitation = Forcing_Object->get_value(CSVDataSelector(CSDMS_STD_NAME_LIQUID_EQ_PRECIP_RATE, begin+(i*3600), 3600, ""), data_access::SUM);

    EXPECT_NEAR(current_precipitation, 6.9999999999999996e-07, 0.00000005);

    //Check exceeding the forcing range to retrieve the last forcing precipation rate
    i = 388;
    current_precipitation = Forcing_Object->get_value(CSVDataSelector(CSDMS_STD_NAME_LIQUID_EQ_PRECIP_RATE, begin+(i*3600), 3600, ""), data_access::SUM);

    EXPECT_NEAR(current_precipitation, 6.9999999999999996e-07, 0.00000005);
}

TEST_F(CsvPerFeatureForcingProviderTest, TestForcingDataReadAltFormat)
{
    double current_precipitation;

    time_t begin = Forcing_Object_2->get_data_start_time();

    int current_day_of_year;
    int i = 8;
    time_t t = begin+(i*3600);
    std::cerr << std::ctime(&t) << std::endl;

    current_precipitation = Forcing_Object_2->get_value(CSVDataSelector(CSDMS_STD_NAME_LIQUID_EQ_PRECIP_RATE, begin+(i*3600), 3600, ""), data_access::SUM);

    EXPECT_NEAR(current_precipitation, 0.00032685, 0.00000001);

    double temp_k = Forcing_Object_2->get_value(CSVDataSelector(CSDMS_STD_NAME_SURFACE_TEMP, begin+(i*3600), 3600, ""), data_access::MEAN);

    EXPECT_NEAR(temp_k, 265.77, 0.00001);

    int current_epoch;

    i = 34;
    current_precipitation = Forcing_Object_2->get_value(CSVDataSelector(CSDMS_STD_NAME_LIQUID_EQ_PRECIP_RATE, begin+(i*3600), 3600, ""), data_access::SUM);

    EXPECT_NEAR(current_precipitation, 0.00013539, 0.00000001);

}


TEST_F(CsvPerFeatureForcingProviderTest, TestForcingDataUnitConversion)
{
    double current_precipitation;

    time_t begin = Forcing_Object->get_data_start_time();

    int current_day_of_year;
    int i = 65;
    time_t t = begin+(i*3600);
    std::cerr << std::ctime(&t) << std::endl;

    current_precipitation = Forcing_Object->get_value(CSVDataSelector(CSDMS_STD_NAME_LIQUID_EQ_PRECIP_RATE, begin+(i*3600), 3600, ""), data_access::SUM);

    EXPECT_NEAR(current_precipitation, 7.9999999999999996e-07, 0.00000005);

    double temp_f = Forcing_Object->get_value(CSVDataSelector(CSDMS_STD_NAME_SURFACE_TEMP, begin+(i*3600), 3600, "degF"), data_access::MEAN);

    EXPECT_NEAR(temp_f, 56.749989014, 0.00001);

}

///Test AORC Forcing Object
TEST_F(CsvPerFeatureForcingProviderTest, TestGetAvailableForcingOutputs)
{
    const std::vector<std::string>& afos = Forcing_Object->get_avaliable_variable_names();
    EXPECT_EQ(afos.size(), 18);
    EXPECT_TRUE(std::find(afos.begin(), afos.end(), "DLWRF_surface") != afos.end());
    EXPECT_TRUE(std::find(afos.begin(), afos.end(), CSDMS_STD_NAME_SOLAR_LONGWAVE) != afos.end());

}

///Test CSV Units Parsing
TEST_F(CsvPerFeatureForcingProviderTest, TestForcingUnitHeaderParsing)
{
    time_t begin = Forcing_Object_3->get_data_start_time();
    int i = 8;
    time_t t = begin+(i*3600);

    const std::vector<std::string>& varnames = this->Forcing_Object_3->get_avaliable_variable_names();
    const std::vector<std::string>& expected = {
        "RAINRATE",
        "T2D",
        "Q2D",
        "U2D",
        "V2D",
        "TEST",
        "PSFC[Pa)",
        "SWDOWN(W m-2]",
        "LWDOWN      (W m-2]"
    };

    for (auto ite = expected.begin(); ite != expected.end(); ite++) {
        // make sure each expected column name is within varnames
        EXPECT_NE(std::find(varnames.begin(), varnames.end(), *ite), varnames.end());

        testing::internal::CaptureStderr();
        const auto val = this->Forcing_Object_3->get_value(
            CSVDataSelector(*ite, t, 3600, ""),
            data_access::SUM
        );
        const std::string cerr_output = testing::internal::GetCapturedStderr();

        if (ite - expected.begin() < 6) {
            // we are expecting warnings, since udunits is trying to map some unit to no units
            // if we don't get a warning, then the unit type was not parsed
            EXPECT_NE(cerr_output, "");
        } else {
            // failed to parse shouldn't output to stderr since the known unit is empty
            EXPECT_EQ(cerr_output, "");
        }
    }

    // quick lambda to check conversion values
    const auto check_conversion = [this, t](
        const std::string& in_units,
        const std::string& out_units,
        const std::string& var_name,
        const double out_val
    ) {
        auto val = this->Forcing_Object_3->get_value(CSVDataSelector(var_name, t, 3600, in_units), data_access::SUM);
        val = UnitsHelper::get_converted_value(in_units, val, out_units);
        EXPECT_NEAR(val, out_val, 0.00001);
    };

    // check some conversions
    check_conversion("K", "degC", CSDMS_STD_NAME_SURFACE_TEMP, -7.38);
    check_conversion("mm s^-1", "cm min^-1", "RAINRATE", 0.0019611);
    check_conversion("kg kg-1", "g kg-1", "Q2D", 1.92);
    check_conversion("kg kg-1", "g kg-1", NGEN_STD_NAME_SPECIFIC_HUMIDITY, 1.92);
    check_conversion("kg", "g", "TEST", 560.6602705);
}