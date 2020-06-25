#include <vector>
#include "gtest/gtest.h"
#include "Forcing.h"
#include <memory>
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <ctime>
#include <time.h>

class ForcingTest : public ::testing::Test {

    protected:

    void SetUp() override;

    void TearDown() override;

    void setupForcing();

    void setupForcing_AORC();

    std::shared_ptr<Forcing> Forcing_Object1; //smart pointer to a Forcing object

    std::shared_ptr<Forcing> Forcing_Object_AORC; //smart pointer to a Forcing object

    typedef struct tm time_type;

    std::shared_ptr<time_type> start_date_time; //smart pointer to time struct

    std::shared_ptr<time_type> end_date_time; //smart pointer to time struct


    typedef struct tm time_type_AORC;

    std::shared_ptr<time_type> start_date_time_AORC; //smart pointer to time struct

    std::shared_ptr<time_type> end_date_time_AORC; //smart pointer to time struct

};


void ForcingTest::SetUp() {
    //setupForcing();

    setupForcing_AORC();
}


void ForcingTest::TearDown() {
}


//Construct a forcing object
void ForcingTest::setupForcing()
{
    string forcing_file_name = "../test/forcing/Sample_Tropical_Hourly_Rainfall.csv";

    start_date_time = std::make_shared<time_type>();

    start_date_time->tm_year = 108;
    start_date_time->tm_mon = 5;
    start_date_time->tm_mday = 19;
    start_date_time->tm_hour = 15;

    end_date_time = std::make_shared<time_type>();

    end_date_time->tm_year = 108;
    end_date_time->tm_mon = 5;
    end_date_time->tm_mday = 22;
    end_date_time->tm_hour = 20;

    Forcing_Object1 = std::make_shared<Forcing>(0.0, 0, forcing_file_name, start_date_time, end_date_time);
}


//Construct a forcing object AORC
void ForcingTest::setupForcing_AORC()
{
    string forcing_file_name_AORC = "../test/forcing/cat-10_2015-12-01 00_00_00_2015-12-30 23_00_00.csv";
    forcing_params forcing_p(forcing_file_name_AORC, "2015-12-14 21:00:00", "2015-12-30 23:00:00");

    Forcing_Object_AORC = std::make_shared<Forcing>(forcing_p);
}

/*Original Forcing Object Test
//Test Forcing object
TEST_F(ForcingTest, TestForcingDataRead)
{
   double current_precipitation;
   int current_day_of_year;
   for (int i = 0; i < 76; i++)
   {
      current_precipitation = Forcing_Object1->get_next_hourly_precipitation_meters_per_second();
   }

   double last_precipitation_rounded = round(current_precipitation * 1000.0) / 1000.0;
   double compare_precipitation_rounded = round(3.24556e-06 * 1000.0) / 1000.0;
   EXPECT_DOUBLE_EQ(compare_precipitation_rounded, last_precipitation_rounded);
   current_day_of_year = Forcing_Object1->get_day_of_year();
   EXPECT_EQ(173, current_day_of_year);
}
*/

///Test AORC Forcing Object
TEST_F(ForcingTest, TestForcingDataRead)
{
   double current_precipitation;

   int current_day_of_year;
   for (int i = 0; i < 66; i++)
   {
      current_precipitation = Forcing_Object_AORC->get_next_hourly_precipitation_meters_per_second();
   }

   double last_precipitation_rounded = round(current_precipitation * 10000000.0) / 10000000.0;

   double compare_precipitation_rounded = round(7.9999999999999996e-07 * 10000000.0) / 10000000.0;

   EXPECT_DOUBLE_EQ(compare_precipitation_rounded, last_precipitation_rounded);

   //Test accessor to AORC data struct
   AORC_data AORC_test_struct = Forcing_Object_AORC->get_AORC_data();

   double temp_k = AORC_test_struct.TMP_2maboveground_K;

   double temp_k_rounded = round(temp_k * 10000.0) / 10000.0;

   double temp_k_compare_rounded = round(286.9 * 10000.0) / 10000.0;

   EXPECT_DOUBLE_EQ(temp_k_compare_rounded, temp_k_rounded);

   current_day_of_year = Forcing_Object_AORC->get_day_of_year();

   EXPECT_EQ(350, current_day_of_year);

   int current_epoch;

   current_epoch = Forcing_Object_AORC->get_time_epoch();

   EXPECT_EQ(1450360800, current_epoch);

   //Check exceeding the forcing range to retrieve the last forcing precipation rate
   for (int i = 66; i < 389; i++)
   {
      current_precipitation = Forcing_Object_AORC->get_next_hourly_precipitation_meters_per_second();
   }

   last_precipitation_rounded = round(current_precipitation * 10000000.0) / 10000000.0;

   compare_precipitation_rounded = round(6.9999999999999996e-07 * 10000000.0) / 10000000.0;

   EXPECT_DOUBLE_EQ(compare_precipitation_rounded, last_precipitation_rounded);

   current_day_of_year = Forcing_Object_AORC->get_day_of_year();

   EXPECT_EQ(363, current_day_of_year);
}
