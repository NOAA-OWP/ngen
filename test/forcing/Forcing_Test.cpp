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


};


void ForcingTest::SetUp() {
    
    //setupNoOutletNonlinearReservoir();


}

void ForcingTest::TearDown() {

}
