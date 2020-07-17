#include "gtest/gtest.h"
#include "tshirt/include/Tshirt.h"
#include "tshirt/include/tshirt_params.h"
#include "GIUH.hpp"

class TshirtModelTest : public ::testing::Test {

protected:

    TshirtModelTest() {

    }

    ~TshirtModelTest() override {

    }

    void SetUp() override;

    void TearDown() override;

    void setupArbitraryExampleCase();

};

void TshirtModelTest::SetUp() {

}

void TshirtModelTest::TearDown() {

}

// Simple test to make sure the run function executes and that the inherent mass-balance check returned by run is good.
TEST_F(TshirtModelTest, TestRun0) {

    double et_storage = 0.0;

    tshirt::tshirt_params params{1000.0, 1.0, 10.0, 0.1, 0.01, 3, 1.0, 1.0, 1.0, 1.0, 8, 1.0, 1.0, 100.0};
    double storage = 1.0;
    double input_flux = 10.0;

    // Testing with implied 0.0's state
    tshirt::tshirt_state state_1(1.0, 1.0);
    tshirt::tshirt_model model_1(params, make_shared<tshirt::tshirt_state>(state_1));
    shared_ptr<pdm03_struct> et_params_1 = make_shared<pdm03_struct>(pdm03_struct());
    int model_1_result = model_1.run(86400.0, input_flux, et_params_1);

    // Testing with explicit state of correct size
    tshirt::tshirt_state state_2(1.0, 1.0,
                                 vector<double> { 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0 });
    tshirt::tshirt_model model_2(params, make_shared<tshirt::tshirt_state>(state_2));
    shared_ptr<pdm03_struct> et_params_2 = make_shared<pdm03_struct>(pdm03_struct());
    int model_2_result = model_2.run(86400.0, input_flux, et_params_2);

    // TODO: figure out how to test for bogus/mismatched nash_n and state nash vector size (without silent error)

    // Should return 0 if mass balance check was good
    EXPECT_EQ(model_1_result, 0);
    EXPECT_EQ(model_2_result, 0);

}

