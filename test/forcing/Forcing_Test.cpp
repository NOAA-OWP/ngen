#include <vector>
#include "gtest/gtest.h"
//#include "forcing/Forcing.h"
//#include "forcing/Forcing.cpp"
#include "Forcing.h"
#include <memory>


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







