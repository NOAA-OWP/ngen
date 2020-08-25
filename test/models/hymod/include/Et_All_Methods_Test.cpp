#include "gtest/gtest.h"
#include <stdio.h>
#include "kernels/EtEnergyBalanceMethodEx.hpp"
#include "kernels/EtAerodynamicMethodEx.hpp"
#include "kernels/EtCombinationMethodEx.hpp"
#include "kernels/EtPriestleyTaylorMethodEx.hpp"
#include "kernels/EtPenmanMonteithMethodEx.hpp"

class EtCalcKernelTest : public ::testing::Test {

    protected:

    EtCalcKernelTest() {

    }

    ~EtCalcKernelTest() override {

    }

    void SetUp() override;

    void TearDown() override;

    void setupArbitraryExampleCase1();

};

void EtCalcKernelTest::SetUp() {
    setupArbitraryExampleCase1();
}

void EtCalcKernelTest::TearDown() {

}

void EtCalcKernelTest::setupArbitraryExampleCase1() {

}

TEST_F(EtCalcKernelTest, TestEnergyBalanceMethod)
{
double et_m_per_s;

et_m_per_s = energy_balance_method_realization();

//EXPECT_DOUBLE_EQ (8.594743e-08, et_m_per_s);
EXPECT_LT(abs(et_m_per_s-8.594743e-08), 1.0e-08);
ASSERT_TRUE(true);
}

TEST_F(EtCalcKernelTest, TestAerodynamicMethod)
{
double et_m_per_s;

et_m_per_s = aerodynamic_method_realization();

//EXPECT_DOUBLE_EQ (8.977490e-08, et_m_per_s);
EXPECT_LT(abs(et_m_per_s-8.977490e-08), 1.0e-08);
ASSERT_TRUE(true);
}

TEST_F(EtCalcKernelTest, TestCombinationMethod)
{
double et_m_per_s;

et_m_per_s = combination_method_realization();

//EXPECT_DOUBLE_EQ (8.694909e-08, et_m_per_s);
EXPECT_LT(abs(et_m_per_s-8.694909e-08), 1.0e-08);
ASSERT_TRUE(true);
}

TEST_F(EtCalcKernelTest, TestPriestleyTaylorMethod)
{
double et_m_per_s;

et_m_per_s = priestley_taylor_method_realization();

//EXPECT_DOUBLE_EQ (8.249098e-08, et_m_per_s);
EXPECT_LT(abs(et_m_per_s-8.249098e-08), 1.0e-08);
ASSERT_TRUE(true);
}

TEST_F(EtCalcKernelTest, TestPenmanMonteithMethod)
{
double et_m_per_s;

et_m_per_s = penman_monteith_method_realization();

//EXPECT_DOUBLE_EQ (1.106268e-07, et_m_per_s);
EXPECT_LT(abs(et_m_per_s-1.106268e-07), 1.0e-07);
ASSERT_TRUE(true);
}
