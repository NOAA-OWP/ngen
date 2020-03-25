#include <vector>
#include <fstream>
#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>


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
    hymod_kernel::run(86400.0,
            params,
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

    // open the file that contains forcings
    std::ifstream input_file("test/data/model/hymod/hymod_forcing.txt");

    if ( !input_file )
    {
        std::cerr << "Test file not found\n";
        ASSERT_TRUE(false);
    }

    // read forcing from the input file
    std::string buffer;

    // skip to the beggining of forcing data
    do
    {
        std::getline(input_file, buffer);
    } while (input_file and !boost::starts_with(buffer, "<DATA_START>") );

    // apply the forcings from the file
    do
    {
        std::getline(input_file, buffer);

        std::vector<std::string> parts;
        boost::split(parts,buffer, boost::is_any_of(" "), boost::token_compress_on);

        if ( parts.size() >= 5 )
        {
            //std::cout << parts[1] << " " << parts[2] << " " << parts[3] << " " << parts[4] << std::endl;
            int year = boost::lexical_cast<int>(parts[1]);
            int month = boost::lexical_cast<int>(parts[2]);
            int day = boost::lexical_cast<int>(parts[3]);
            double input_flux = boost::lexical_cast<double>(parts[4]);

            // initalize hymod state for next time step
            backing_storage.push_back(std::vector<double>{0.0, 0.0, 0.0});
            states.push_back(hymod_state{0.0, 0.0, backing_storage[backing_storage.size()-1].data()});

            // initalize hymod fluxes for this time step
            fluxes.push_back(hymod_fluxes(0.0, 0.0, 0.0));

            int pos = fluxes.size() - 1;
            double et_stand_in = 0;

            hymod_kernel::run(86400.0, params, states[pos], states[pos+1], fluxes[pos], input_flux, &et_stand_in);

        }
    } while( input_file );



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


