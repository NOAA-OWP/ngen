#include "gtest/gtest.h"
#include "Simulation_Time.h"
#include <memory>
#include <vector>
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <ctime>
#include <time.h>

class SimulationTimeTest : public ::testing::Test {

    protected:

    void SetUp() override;

    void TearDown() override;

    void setupSimulationTime();

    std::shared_ptr<Simulation_Time> Simulation_Time_Object1; //smart pointer to a Simulation_Time object

    typedef struct tm time_type;

    std::shared_ptr<time_type> start_date_time; //smart pointer to time struct

    std::shared_ptr<time_type> end_date_time; //smart pointer to time struct

};


void SimulationTimeTest::SetUp() {

    setupSimulationTime();
}

void SimulationTimeTest::TearDown() {
}


//Construct a simulation time object
void SimulationTimeTest::setupSimulationTime()
{

    simulation_time_params simulation_time_p("2015-12-14 21:00:00", "2015-12-30 23:00:00", 3600);

    Simulation_Time_Object1 = std::make_shared<Simulation_Time>(simulation_time_p);

}

///Test simulation time Object
TEST_F(SimulationTimeTest, TestSimulationTime)
{
    int total_output_times;

    total_output_times = Simulation_Time_Object1->get_total_output_times();

    EXPECT_EQ(387, total_output_times);

    std::string current_timestamp;

    current_timestamp = Simulation_Time_Object1->get_timestamp(5);

    std::string compare_timestamp = "2015-12-15 02:00:00";

    //EXPECT_TRUE(compare_timestamp == current_timestamp); 

    EXPECT_EQ(current_timestamp, compare_timestamp); 

}


