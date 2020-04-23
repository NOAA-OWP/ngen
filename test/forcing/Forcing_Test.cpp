#include <vector>
#include "gtest/gtest.h"
//#include "forcing/Forcing.h"
//#include "forcing/Forcing.cpp"
#include "Forcing.h"
#include <memory>
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <chrono>  // chrono::system_clock
#include <ctime>


class ForcingTest : public ::testing::Test {

    protected:

    void SetUp() override;

    void TearDown() override;

    void setupForcing();

    std::shared_ptr<Forcing> Forcing_Object1; //smart pointer to a Forcing object

};


void ForcingTest::SetUp() {
    
    setupForcing();

}

void ForcingTest::TearDown() {

}


//Construct a forcing object
void ForcingTest::setupForcing()
{
    //auto now1 = std::chrono::system_clock::now();

    //auto now2 = std::chrono::system_clock::now();

    //auto now1 = std::chrono::high_resolution_clock::now();

    //auto now2 = std::chrono::high_resolution_clock::now();


    //std::chrono::system_clock::time_point now1 = std::chrono::system_clock::now();

    //std::chrono::system_clock::time_point now2 = std::chrono::system_clock::now();


    //std::chrono::steady_clock::time_point now1 = std::chrono::steady_clock::now();

    //std::chrono::steady_clock::time_point now2 = std::chrono::steady_clock::now();


    //std::chrono::steady_clock::time_point start_date_time;

    //std::chrono::steady_clock::time_point end_date_time;


    //Forcing_Object1 = std::make_shared<Forcing>(0.0, 0.0, 0);

cout << "a0";

    string forcing_file_name = "../test/forcing/Sample_Tropical_Hourly_Rainfall.csv";

cout << "a1";
    
    struct tm *start_date_time;

    start_date_time->tm_year = 108;
    start_date_time->tm_mon = 5;
    start_date_time->tm_mday = 19;
    start_date_time->tm_hour = 15;

cout << "a2";

    //string start_date_time_str = asctime(start_date_time);

    //cout << start_date_time_str;


    //mktime ( start_date_time );

    //cout << "start_date_time: " + start_date_time << endl; 

    //cout << "start_date_time: " + asctime(start_date_time) << endl; 

    //char buffer [80];
    //strftime (buffer,80,"Now it's %I:%M%p.",start_date_time);
    //puts (buffer);

    //char* dt = ctime(&start_date_time);


    //char buffer[26];
    //strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", start_date_time);
    //puts(buffer);

    //char buffer[26];
    //strftime(buffer, 6, "%Y-%m-%d %H", start_date_time);
    //puts(buffer);
    //cout << buffer;
cout << "a3";

    struct tm *end_date_time;

    end_date_time->tm_year = 108;
    end_date_time->tm_mon = 5;
    end_date_time->tm_mday = 22;
    end_date_time->tm_hour = 20;

cout << "a4";

    //cout << "end_date_time: " + asctime(end_date_time) << endl; 

    Forcing_Object1 = std::make_shared<Forcing>(0.0, 0.0, 0, forcing_file_name, start_date_time, end_date_time);



    //Forcing_Object1 = std::make_shared<Forcing>(0.0, 0.0, 0, forcing_file_name, now1, now2);
}


//Test Forcing object
TEST_F(ForcingTest, TestForcingDataRead)
{
    //string forcing_file_name = "Sample_Tropical_Hourly_Rainfall.csv";

    //string forcing_file_name = "/home/jdmattern/Documents/ngen/test/forcing/Sample_Tropical_Hourly_Rainfall.csv";

    string forcing_file_name = "../test/forcing/Sample_Tropical_Hourly_Rainfall.csv";
   
   char cwd[PATH_MAX];
   if (getcwd(cwd, sizeof(cwd)) != NULL) {
       printf("Current working dir: %s\n", cwd);
   } else {
       perror("getcwd() error");
   }

   
   //forcing_file_name = getcwd();

   //Forcing_Object1->read_forcing(forcing_file_name);

}





