#include <vector>
#include <fstream>
#include "gtest/gtest.h"
#include "hymod/include/Hymod.h"

class HymodKernelTest : public ::testing::Test {

    protected:



    HymodKernelTest() {

    }

    ~HymodKernelTest() override {

    }

    void SetUp() override;

    void TearDown() override;

    void setupArbitraryExampleCase();

};

void HymodKernelTest::SetUp() {
    setupArbitraryExampleCase();
}

void HymodKernelTest::TearDown() {

}

//! Setup an arbitrary example case with essentially made-up values for the components, and place the components in the
/*!
 *  Setup an arbitrary example case with essentially made-up values for the components, and place the components in the
 *  appropriate member collections.
 */
void HymodKernelTest::setupArbitraryExampleCase() {

}

//! Test that Hymod executes its 'run' function fully when passed arbitrary valid arguments.
TEST_F(HymodKernelTest, TestRun0)
{
    double et_storage = 0.0;

    hymod_params params{1000.0, 1.0, 10.0, 0.1, 0.01, 3};
    double storage = 1.0;

    double reservior_storage[] = {1.0, 1.0, 1.0};
    hymod_state state(0.0,0.0, reservior_storage);

    double new_reservior_storage[] = {1.0, 1.0, 1.0};
    hymod_state new_state(0.0,0.0, new_reservior_storage);

    hymod_fluxes fluxes(0.0, 0.0, 0.0);
    double input_flux = 1.0;

    //hymod_kernel::run(params, h_state, ks_fluxes, new_state, new_fluxes, input_flux, et_params);
    hymod_kernel::run(params,
            state,
            new_state,
            fluxes,
            input_flux,
            &et_storage);
    ASSERT_TRUE(true);
}

TEST_F(HymodKernelTest, TestWithKnownInput)
{
    std::vector<hymod_state> states;
    std::vector<hymod_fluxes> fluxes;
    std::vector< std::vector<double> > backing_storage;

    // initalize hymod params
    hymod_params params{400.0, 0.5, 1.3, 0.2, 0.02, 3};

    // initalize hymod state for time step zero
    backing_storage.push_back(std::vector<double>{0.0, 0.0, 0.0});
    states.push_back(hymod_state{0.9, 0.0, backing_storage[0].data()});

    // initalize hymod fluxes
    fluxes.push_back(hymod_fluxes(0.0, 0.0, 0.0));

    // open the file that contains forcings
    std::ifstream input_file("test/data/model/hymod/hymod_forcing.txt");

    if ( !input_file )
    {
        std::cout << "Test file not found";
        ASSERT_TRUE(false);
    }

    // read forcing from the input file



    ASSERT_TRUE(true);
}

//! Test that Hymod executes its 'calc_et' function and returns the expected result.
/*!
    In the current implementation (at the time this test was created), the static method simple returns 0.0 regardless
    of the parameters.
*/
TEST_F(HymodKernelTest, TestCalcET0) {
    // Since currently the function doesn't care about params, borrow this from example 0 ...
    double et_storage = 0.0;

    ASSERT_EQ(hymod_kernel::calc_et(3.0, &et_storage), 0.0);
}


