#include "gtest/gtest.h"
#include <stdio.h>
#include "kernels/evapotranspiration/EtWrapperFunction.hpp"
#include "kernels/evapotranspiration/EtSetParams.hpp"

class EtCalcKernelTest : public ::testing::Test {

    protected:

    EtCalcKernelTest() {

    }

    ~EtCalcKernelTest() override {

    }

    void SetUp() override;

    void TearDown() override;

    void setupArbitraryExampleCase();

};

void EtCalcKernelTest::SetUp() {
    setupArbitraryExampleCase();
}

void EtCalcKernelTest::TearDown() {

}

void EtCalcKernelTest::setupArbitraryExampleCase() {

}


TEST_F(EtCalcKernelTest, TestEnergyBalanceMethod)
{
  struct evapotranspiration_options set_et_options;
  double et_m_per_s;
  int et_method_option;

  struct aorc_forcing_data aorc;

  struct evapotranspiration_options et_options;
  struct evapotranspiration_params  et_params;
  struct evapotranspiration_forcing et_forcing;
  struct intermediate_vars inter_vars;

  struct surface_radiation_params   surf_rad_params;
  struct surface_radiation_forcing  surf_rad_forcing;

  struct solar_radiation_options    solar_options;
  struct solar_radiation_parameters solar_params;
  struct solar_radiation_forcing    solar_forcing;

  et_setup(aorc, solar_options, solar_params, solar_forcing, set_et_options,
           et_params, et_forcing, surf_rad_params, surf_rad_forcing);

  //et_method_option = 1;    use_energy_balance_method
  //et_method_option = 2;    use_aerodynamic_method
  //et_method_option = 3;    use_combination_method
  //et_method_option = 4;    use_priestley_taylor_method
  //et_method_option = 5;    use_penman_monteith_method

  //set et_method_option 
  et_method_option = 1;

  //read in et_method_option from standard input for convenience of testing
  //std::cout << "Input et_method_option: 1-5" << std::endl;
  //std::cin >> et_method_option;

  //the following two variables are set in function et_setup()
  //set_et_options->yes_aorc=TRUE;
  //set_et_options->shortwave_radiation_provided=FALSE;
  set_et_options.use_energy_balance_method   = FALSE;
  set_et_options.use_aerodynamic_method      = FALSE;
  set_et_options.use_combination_method      = FALSE;
  set_et_options.use_priestley_taylor_method = FALSE;
  set_et_options.use_penman_monteith_method  = FALSE;

  if (et_method_option == 1)
    set_et_options.use_energy_balance_method   = TRUE;
  if (et_method_option == 2)
    set_et_options.use_aerodynamic_method      = TRUE;
  if (et_method_option == 3)
    set_et_options.use_combination_method      = TRUE;
  if (et_method_option == 4)
    set_et_options.use_priestley_taylor_method = TRUE;
  if (et_method_option == 5)
    set_et_options.use_penman_monteith_method  = TRUE;

  et_m_per_s = et_wrapper_function(&aorc, &solar_options, &solar_params, &solar_forcing,
                                   &set_et_options, &et_params, &et_forcing, 
                                   &surf_rad_params, &surf_rad_forcing);

  //EXPECT_DOUBLE_EQ (8.594743e-08, et_m_per_s);
  EXPECT_LT(abs(et_m_per_s-8.594743e-08), 1.0e-08);
  ASSERT_TRUE(true);

}


TEST_F(EtCalcKernelTest, TestAerodynamicMethod)
{
  struct evapotranspiration_options set_et_options;
  double et_m_per_s;
  int et_method_option;

  struct aorc_forcing_data aorc;

  struct evapotranspiration_options et_options;
  struct evapotranspiration_params  et_params;
  struct evapotranspiration_forcing et_forcing;
  struct intermediate_vars inter_vars;

  struct surface_radiation_params   surf_rad_params;
  struct surface_radiation_forcing  surf_rad_forcing;

  struct solar_radiation_options    solar_options;
  struct solar_radiation_parameters solar_params;
  struct solar_radiation_forcing    solar_forcing;

  et_setup(aorc, solar_options, solar_params, solar_forcing, set_et_options,
           et_params, et_forcing, surf_rad_params, surf_rad_forcing);

  //et_method_option = 1;    use_energy_balance_method
  //et_method_option = 2;    use_aerodynamic_method
  //et_method_option = 3;    use_combination_method
  //et_method_option = 4;    use_priestley_taylor_method
  //et_method_option = 5;    use_penman_monteith_method

  //set et_method_option 
  et_method_option = 2;

  //read in et_method_option from standard input for convenience of testing
  //std::cout << "Input et_method_option: 1-5" << std::endl;
  //std::cin >> et_method_option;

  //the following two variables are set in function et_setup()
  //set_et_options->yes_aorc=TRUE;
  //set_et_options->shortwave_radiation_provided=FALSE;
  set_et_options.use_energy_balance_method   = FALSE;
  set_et_options.use_aerodynamic_method      = FALSE;
  set_et_options.use_combination_method      = FALSE;
  set_et_options.use_priestley_taylor_method = FALSE;
  set_et_options.use_penman_monteith_method  = FALSE;

  if (et_method_option == 1)
    set_et_options.use_energy_balance_method   = TRUE;
  if (et_method_option == 2)
    set_et_options.use_aerodynamic_method      = TRUE;
  if (et_method_option == 3)
    set_et_options.use_combination_method      = TRUE;
  if (et_method_option == 4)
    set_et_options.use_priestley_taylor_method = TRUE;
  if (et_method_option == 5)
    set_et_options.use_penman_monteith_method  = TRUE;

  et_m_per_s = et_wrapper_function(&aorc, &solar_options, &solar_params, &solar_forcing,
                                   &set_et_options, &et_params, &et_forcing, 
                                   &surf_rad_params, &surf_rad_forcing);

  //EXPECT_DOUBLE_EQ (8.977490e-08, et_m_per_s);
  EXPECT_LT(abs(et_m_per_s-8.977490e-08), 1.0e-08);
  ASSERT_TRUE(true);

}


TEST_F(EtCalcKernelTest, TestCombinationMethod)
{
  struct evapotranspiration_options set_et_options;
  double et_m_per_s;
  int et_method_option;

  struct aorc_forcing_data aorc;

  struct evapotranspiration_options et_options;
  struct evapotranspiration_params  et_params;
  struct evapotranspiration_forcing et_forcing;
  struct intermediate_vars inter_vars;

  struct surface_radiation_params   surf_rad_params;
  struct surface_radiation_forcing  surf_rad_forcing;

  struct solar_radiation_options    solar_options;
  struct solar_radiation_parameters solar_params;
  struct solar_radiation_forcing    solar_forcing;

  et_setup(aorc, solar_options, solar_params, solar_forcing, set_et_options,
           et_params, et_forcing, surf_rad_params, surf_rad_forcing);

  //et_method_option = 1;    use_energy_balance_method
  //et_method_option = 2;    use_aerodynamic_method
  //et_method_option = 3;    use_combination_method
  //et_method_option = 4;    use_priestley_taylor_method
  //et_method_option = 5;    use_penman_monteith_method

  //set et_method_option 
  et_method_option = 3;

  //read in et_method_option from standard input for convenience of testing
  //std::cout << "Input et_method_option: 1-5" << std::endl;
  //std::cin >> et_method_option;

  //the following two variables are set in function et_setup()
  //set_et_options->yes_aorc=TRUE;
  //set_et_options->shortwave_radiation_provided=FALSE;
  set_et_options.use_energy_balance_method   = FALSE;
  set_et_options.use_aerodynamic_method      = FALSE;
  set_et_options.use_combination_method      = FALSE;
  set_et_options.use_priestley_taylor_method = FALSE;
  set_et_options.use_penman_monteith_method  = FALSE;

  if (et_method_option == 1)
    set_et_options.use_energy_balance_method   = TRUE;
  if (et_method_option == 2)
    set_et_options.use_aerodynamic_method      = TRUE;
  if (et_method_option == 3)
    set_et_options.use_combination_method      = TRUE;
  if (et_method_option == 4)
    set_et_options.use_priestley_taylor_method = TRUE;
  if (et_method_option == 5)
    set_et_options.use_penman_monteith_method  = TRUE;

  et_m_per_s = et_wrapper_function(&aorc, &solar_options, &solar_params, &solar_forcing,
                                   &set_et_options, &et_params, &et_forcing, 
                                   &surf_rad_params, &surf_rad_forcing);

  //EXPECT_DOUBLE_EQ (8.694909e-08, et_m_per_s);
  EXPECT_LT(abs(et_m_per_s-8.694909e-08), 1.0e-08);
  ASSERT_TRUE(true);

}


TEST_F(EtCalcKernelTest, TestPriestleyTaylorMethod)
{
  struct evapotranspiration_options set_et_options;
  double et_m_per_s;
  int et_method_option;

  struct aorc_forcing_data aorc;

  struct evapotranspiration_options et_options;
  struct evapotranspiration_params  et_params;
  struct evapotranspiration_forcing et_forcing;
  struct intermediate_vars inter_vars;

  struct surface_radiation_params   surf_rad_params;
  struct surface_radiation_forcing  surf_rad_forcing;

  struct solar_radiation_options    solar_options;
  struct solar_radiation_parameters solar_params;
  struct solar_radiation_forcing    solar_forcing;

  et_setup(aorc, solar_options, solar_params, solar_forcing, set_et_options,
           et_params, et_forcing, surf_rad_params, surf_rad_forcing);

  //et_method_option = 1;    use_energy_balance_method
  //et_method_option = 2;    use_aerodynamic_method
  //et_method_option = 3;    use_combination_method
  //et_method_option = 4;    use_priestley_taylor_method
  //et_method_option = 5;    use_penman_monteith_method

  //set et_method_option 
  et_method_option = 4;

  //read in et_method_option from standard input for convenience of testing
  //std::cout << "Input et_method_option: 1-5" << std::endl;
  //std::cin >> et_method_option;

  //the following two variables are set in function et_setup()
  //set_et_options->yes_aorc=TRUE;
  //set_et_options->shortwave_radiation_provided=FALSE;
  set_et_options.use_energy_balance_method   = FALSE;
  set_et_options.use_aerodynamic_method      = FALSE;
  set_et_options.use_combination_method      = FALSE;
  set_et_options.use_priestley_taylor_method = FALSE;
  set_et_options.use_penman_monteith_method  = FALSE;

  if (et_method_option == 1)
    set_et_options.use_energy_balance_method   = TRUE;
  if (et_method_option == 2)
    set_et_options.use_aerodynamic_method      = TRUE;
  if (et_method_option == 3)
    set_et_options.use_combination_method      = TRUE;
  if (et_method_option == 4)
    set_et_options.use_priestley_taylor_method = TRUE;
  if (et_method_option == 5)
    set_et_options.use_penman_monteith_method  = TRUE;

  et_m_per_s = et_wrapper_function(&aorc, &solar_options, &solar_params, &solar_forcing,
                                   &set_et_options, &et_params, &et_forcing, 
                                   &surf_rad_params, &surf_rad_forcing);

  //EXPECT_DOUBLE_EQ (8.249098e-08, et_m_per_s);
  EXPECT_LT(abs(et_m_per_s-8.249098e-08), 1.0e-08);
  ASSERT_TRUE(true);

}


TEST_F(EtCalcKernelTest, TestPenmanMonteithMethod)
{
  struct evapotranspiration_options set_et_options;
  double et_m_per_s;
  int et_method_option;

  struct aorc_forcing_data aorc;

  struct evapotranspiration_options et_options;
  struct evapotranspiration_params  et_params;
  struct evapotranspiration_forcing et_forcing;
  struct intermediate_vars inter_vars;

  struct surface_radiation_params   surf_rad_params;
  struct surface_radiation_forcing  surf_rad_forcing;

  struct solar_radiation_options    solar_options;
  struct solar_radiation_parameters solar_params;
  struct solar_radiation_forcing    solar_forcing;

  et_setup(aorc, solar_options, solar_params, solar_forcing, set_et_options,
           et_params, et_forcing, surf_rad_params, surf_rad_forcing);

  //et_method_option = 1;    use_energy_balance_method
  //et_method_option = 2;    use_aerodynamic_method
  //et_method_option = 3;    use_combination_method
  //et_method_option = 4;    use_priestley_taylor_method
  //et_method_option = 5;    use_penman_monteith_method

  //set et_method_option 
  et_method_option = 5;

  //read in et_method_option from standard input for convenience of testing
  //std::cout << "Input et_method_option: 1-5" << std::endl;
  //std::cin >> et_method_option;

  //the following two variables are set in function et_setup()
  //set_et_options->yes_aorc=TRUE;
  //set_et_options->shortwave_radiation_provided=FALSE;
  set_et_options.use_energy_balance_method   = FALSE;
  set_et_options.use_aerodynamic_method      = FALSE;
  set_et_options.use_combination_method      = FALSE;
  set_et_options.use_priestley_taylor_method = FALSE;
  set_et_options.use_penman_monteith_method  = FALSE;

  if (et_method_option == 1)
    set_et_options.use_energy_balance_method   = TRUE;
  if (et_method_option == 2)
    set_et_options.use_aerodynamic_method      = TRUE;
  if (et_method_option == 3)
    set_et_options.use_combination_method      = TRUE;
  if (et_method_option == 4)
    set_et_options.use_priestley_taylor_method = TRUE;
  if (et_method_option == 5)
    set_et_options.use_penman_monteith_method  = TRUE;

  et_m_per_s = et_wrapper_function(&aorc, &solar_options, &solar_params, &solar_forcing,
                                   &set_et_options, &et_params, &et_forcing, 
                                   &surf_rad_params, &surf_rad_forcing);

  //EXPECT_DOUBLE_EQ (1.106268e-07, et_m_per_s);
  EXPECT_LT(abs(et_m_per_s-1.106268e-07), 1.0e-07);
  ASSERT_TRUE(true);

}
