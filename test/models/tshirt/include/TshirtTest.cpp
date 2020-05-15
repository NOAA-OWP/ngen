#include "gtest/gtest.h"
#include "tshirt/include/Tshirt.h"

class TshirtKernelTest : public ::testing::Test {

protected:

    TshirtKernelTest() {

    }

    ~TshirtKernelTest() override {

    }

    void SetUp() override;

    void TearDown() override;

    void setupArbitraryExampleCase();

};

void TshirtKernelTest::SetUp() {

}

void TshirtKernelTest::TearDown() {

}

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

//! Test that Tshirt executes its 'run' function fully when passed arbitrary valid arguments.
TEST_F(TshirtKernelTest, TestRun0)
{
    // TODO: fix what was copied below from Hymod test for Tshirt


    double et_storage = 0.0;

    tshirt::tshirt_params params{1000.0, 1.0, 10.0, 0.1, 0.01, 3, 1.0, 1.0, 1.0, 1.0, 8, 1.0, 1.0, 100.0};
    double storage = 1.0;

    tshirt::tshirt_state state(1.0, 1.0);

    tshirt::tshirt_state new_state(0.0, 0.0);

    tshirt::tshirt_fluxes fluxes(0.0, 0.0, 0.0, 0.0, 0.0);
    double input_flux = 1.0;

    giuh::giuh_kernel giuh_obj = giuh::giuh_kernel();

    //hymod_kernel::run(params, h_state, ks_fluxes, new_state, new_fluxes, input_flux, et_params);
    tshirt::tshirt_kernel::run(86400.0,
                               params,
                               state,
                               new_state,
                               fluxes,
                               input_flux,
                               &giuh_obj,
                               &et_storage);

    ASSERT_TRUE(true);
}

TEST_F(TshirtModelTest, TestRun0) {

    double et_storage = 0.0;

    tshirt::tshirt_params params{1000.0, 1.0, 10.0, 0.1, 0.01, 3, 1.0, 1.0, 1.0, 1.0, 8, 1.0, 1.0, 100.0};
    double storage = 1.0;

    tshirt::tshirt_state state(1.0, 1.0);

    double input_flux = 1.0;

    tshirt::tshirt_model model(params, make_shared<tshirt::tshirt_state>(state));

    shared_ptr<pdm03_struct> et_params = make_shared<pdm03_struct>(pdm03_struct());

    model.run(86400.0, input_flux, et_params);

    ASSERT_TRUE(true);

}

