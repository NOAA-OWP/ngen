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

    std::shared_ptr<Forcing> Forcing_Object1; //smart pointer to a Forcing object

    typedef struct tm time_type;

    std::shared_ptr<time_type> start_date_time; //smart pointer to time struct

    std::shared_ptr<time_type> end_date_time; //smart pointer to time struct
};


void ForcingTest::SetUp() {
    setupForcing();
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





