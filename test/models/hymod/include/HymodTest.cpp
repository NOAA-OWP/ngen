#include <vector>
#include <fstream>
#include <string>
#include <sstream>


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


    // create the struct used for ET
    pdm03_struct pdm_et_data;
    pdm_et_data.B = 1.3;
    pdm_et_data.Kv = 0.99;
    pdm_et_data.modelDay = 0.0;
    pdm_et_data.Huz = 400.0;
    pdm_et_data.Cpar = pdm_et_data.Huz / (1.0+pdm_et_data.B);

    double latitude = 41.13;

    // open the file that contains forcings
    std::ifstream input_file("test/data/model/hymod/hymod_forcing.txt");

    if ( !input_file )
    {
        // Account for possibly being within build directory also
        input_file = std::ifstream("../test/data/model/hymod/hymod_forcing.txt");
        if (!input_file) {
            std::cerr << "Test file not found\n";
            ASSERT_TRUE(false);
        }
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

        long year;
        long month;
        long day;
        double mean_areal_precipitation;
        double climatic_potential_evaporation;
        double daily_streamflow_discharge;
        double daily_maximum_air_temperature;
        double daily_minimum_air_temperature;

        std::stringstream ss(buffer);

        ss >> year >> month >> day
            >> mean_areal_precipitation
            >> climatic_potential_evaporation
            >> daily_streamflow_discharge
            >> daily_maximum_air_temperature
            >> daily_minimum_air_temperature;

//        std::cout << " " << year << " " << month << " " << day
//            << " " << input_flux
//            << " " << climatic_potential_evaporation
//            << " " << daily_streamflow_discharge
//            << " " << daily_maximum_air_temperature
//            << " " << daily_minimum_air_temperature << "\n";

        // initalize hymod state for next time step
        backing_storage.push_back(std::vector<double>{0.0, 0.0, 0.0});
        states.push_back(hymod_state{0.0, 0.0, backing_storage[backing_storage.size()-1].data()});

        // initalize hymod fluxes for this time step
        fluxes.push_back(hymod_fluxes(0.0, 0.0, 0.0));

        int pos = fluxes.size() - 1;
        double et_stand_in = 0;

        //calcuate inital PE
        int day_of_year = (int)(greg_2_jul(year, month, day,12,0,0.0) - greg_2_jul(year,1,1,12,0,0.0) + 1);
        double average_tmp = (daily_maximum_air_temperature + daily_minimum_air_temperature) / 2.0;
        pdm_et_data.PE = calculateHamonPE(average_tmp, latitude, day_of_year);

        // update other et values
        pdm_et_data.effPrecip = mean_areal_precipitation;

        hymod_kernel::run(86400.0, params, states[pos], states[pos+1], fluxes[pos], mean_areal_precipitation, &pdm_et_data);

    } while( input_file );



    ASSERT_TRUE(true);
}

//! Test that Hymod executes its 'calc_evapotranspiration' function and returns the expected result.
/* TODO: get some actual examples to test with.
 * For now, commenting this out until some valid tests can actual be set up, with expected ET values.
TEST_F(HymodKernelTest, TestCalcET0) {
    // Since currently the function doesn't care about params, borrow this from example 0 ...
    double et_storage = 0.0;

    ASSERT_EQ(hymod_kernel::calc_et(3.0, &et_storage), 0.0);
}
*/


