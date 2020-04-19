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
    Forcing_Object1 = std::make_shared<Forcing>(0.0, 0.0, 0);
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

   Forcing_Object1->read_forcing(forcing_file_name);



}




