#include <vector>
#include "gtest/gtest.h"
#include "kernels/Nonlinear_Reservoir.hpp"
#include <memory>

//This class contains unit tests for the Nonlinear Reservoir
class NonlinearReservoirKernelTest : public ::testing::Test {

    protected:

    void SetUp() override;

    void TearDown() override;

    void setupNoOutletNonlinearReservoir();

    void setupOneOutletNonlinearReservoir();

    void setupMultipleOutletNonlinearReservoir();

    void setupMultipleOutletOutOfOrderNonlinearReservoir();

    std::shared_ptr<Nonlinear_Reservoir> NoOutletReservoir; //smart pointer to a Nonlinear_Reservoir with no outlets

    std::shared_ptr<Nonlinear_Reservoir> OneOutletReservoir; //smart pointer to a Nonlinear_Reservoir with one outlet

    std::shared_ptr<Nonlinear_Reservoir> MultipleOutletReservoir; //smart pointer to a Nonlinear_Reservoir with multiple outlets

    std::shared_ptr<Nonlinear_Reservoir> MultipleOutletOutOfOrderNonlinearReservoir; //smart pointer to a Nonlinear_Reservoir with multiple outlets out of order

    std::shared_ptr<Reservoir_Outlet> ReservoirOutlet1; //smart pointer to a Reservoir Outlet

    std::shared_ptr<Reservoir_Outlet> ReservoirOutlet2; //smart pointer to a Reservoir Outlet

    std::shared_ptr<Reservoir_Outlet> ReservoirOutlet3; //smart pointer to a Reservoir Outlet

    std::vector<Reservoir_Outlet> ReservoirOutletsVector;

    std::vector<Reservoir_Outlet> ReservoirOutletsVectorOutOfOrder;

};

void NonlinearReservoirKernelTest::SetUp() {
    
    setupNoOutletNonlinearReservoir();

    setupOneOutletNonlinearReservoir();

    setupMultipleOutletNonlinearReservoir();

    setupMultipleOutletOutOfOrderNonlinearReservoir();
}

void NonlinearReservoirKernelTest::TearDown() {

}

//Construct a reservoir with no outlets
void NonlinearReservoirKernelTest::setupNoOutletNonlinearReservoir()
{
    NoOutletReservoir = std::make_shared<Nonlinear_Reservoir>(0.0, 8.0, 2.0);
}

//Construct a reservoir with one outlet
void NonlinearReservoirKernelTest::setupOneOutletNonlinearReservoir()
{
    OneOutletReservoir = std::make_shared<Nonlinear_Reservoir>(0.0, 8.0, 3.5, 0.5, 0.7, 4.0, 100.0);
}

//Construct a reservoir with multiple outlets
void NonlinearReservoirKernelTest::setupMultipleOutletNonlinearReservoir()
{
    ReservoirOutlet1 = std::make_shared<Reservoir_Outlet>(0.2, 0.4, 4.0, 100.0);

    ReservoirOutlet2 = std::make_shared<Reservoir_Outlet>(0.3, 0.5, 10.0, 100.0);

    ReservoirOutlet3 = std::make_shared<Reservoir_Outlet>(0.4, 0.6, 15.0, 100.0);

    ReservoirOutletsVector.push_back(*ReservoirOutlet1);

    ReservoirOutletsVector.push_back(*ReservoirOutlet2);

    ReservoirOutletsVector.push_back(*ReservoirOutlet3);

    MultipleOutletReservoir = std::make_shared<Nonlinear_Reservoir>(0.0, 20.0, 2.0, ReservoirOutletsVector);
}

//Construct a reservoir with multiple outlets that are not ordered from lowest to highest activation threshold
void NonlinearReservoirKernelTest::setupMultipleOutletOutOfOrderNonlinearReservoir()
{
    ReservoirOutlet1 = std::make_shared<Reservoir_Outlet>(0.2, 0.4, 4.0, 100.0);

    ReservoirOutlet2 = std::make_shared<Reservoir_Outlet>(0.3, 0.5, 10.0, 100.0);

    ReservoirOutlet3 = std::make_shared<Reservoir_Outlet>(0.4, 0.6, 15.0, 100.0);

    ReservoirOutletsVectorOutOfOrder.push_back(*ReservoirOutlet3);

    ReservoirOutletsVectorOutOfOrder.push_back(*ReservoirOutlet2);

    ReservoirOutletsVectorOutOfOrder.push_back(*ReservoirOutlet1);

    MultipleOutletOutOfOrderNonlinearReservoir = std::make_shared<Nonlinear_Reservoir>(0.0, 20.0, 2.0, ReservoirOutletsVectorOutOfOrder);
}


//Test Nonlinear Reservoir with no outlets.
TEST_F(NonlinearReservoirKernelTest, TestRunNoOutletReservoir) 
{   
    double in_flux_meters_per_second;
    double excess;
    double final_storage;

    in_flux_meters_per_second = 0.05;

    NoOutletReservoir->response_meters_per_second(in_flux_meters_per_second, 10, excess);

    final_storage = NoOutletReservoir->get_storage_height_meters();

    final_storage = round( final_storage * 100.0 ) / 100.0;

    EXPECT_DOUBLE_EQ (2.5, final_storage);

    ASSERT_TRUE(true);
}

//Test Nonlinear Reservoir with one outlet.
TEST_F(NonlinearReservoirKernelTest, TestRunOneOutletReservoir) 
{
    double in_flux_meters_per_second;
    double excess;
    double final_storage;

    in_flux_meters_per_second = 0.2;

    OneOutletReservoir->response_meters_per_second(in_flux_meters_per_second, 10, excess);

    final_storage = OneOutletReservoir->get_storage_height_meters();

    final_storage = round( final_storage * 100.0 ) / 100.0;

    EXPECT_DOUBLE_EQ (2.98, final_storage);

    ASSERT_TRUE(true);
}

//Test Nonlinear Reservoir with one outlet where the storage remains below the activation threshold.
TEST_F(NonlinearReservoirKernelTest, TestRunOneOutletReservoirNoActivation) 
{   
    double in_flux_meters_per_second;
    double excess;
    double final_storage;

    in_flux_meters_per_second = 0.02;

    OneOutletReservoir->response_meters_per_second(in_flux_meters_per_second, 10, excess);

    final_storage = OneOutletReservoir->get_storage_height_meters();

    final_storage = round( final_storage * 100.0 ) / 100.0;

    EXPECT_DOUBLE_EQ (3.7, final_storage);

    ASSERT_TRUE(true);
}


//Test Nonlinear Reservoir with one outlet where the storage exceeds max storage.
TEST_F(NonlinearReservoirKernelTest, TestRunOneOutletReservoirExceedMaxStorage) 
{   
    double in_flux_meters_per_second;
    double excess;
    double final_storage;

    in_flux_meters_per_second = 3.0;

    OneOutletReservoir->response_meters_per_second(in_flux_meters_per_second, 10, excess);

    final_storage = OneOutletReservoir->get_storage_height_meters();

    final_storage = round( final_storage * 100.0 ) / 100.0;

    EXPECT_DOUBLE_EQ (8.0, final_storage);

    ASSERT_TRUE(true);
}


//Test Nonlinear Reservoir with one outlet where the storage falls below the minimum storage of zero.
TEST_F(NonlinearReservoirKernelTest, TestRunOneOutletReservoirFallsBelowMinimumStorage) 
{   
    double in_flux_meters_per_second;
    double excess;
    double final_storage;

    in_flux_meters_per_second = -1.0;

    OneOutletReservoir->response_meters_per_second(in_flux_meters_per_second, 10, excess);

    final_storage = OneOutletReservoir->get_storage_height_meters();

    final_storage = round( final_storage * 100.0 ) / 100.0;

    EXPECT_DOUBLE_EQ (0.0, final_storage);

    ASSERT_TRUE(true);
}


//Test Nonlinear Reservoir with multiple outlets
TEST_F(NonlinearReservoirKernelTest, TestRunMultipleOutletReservoir) 
{    
    double in_flux_meters_per_second;
    double excess;
    double final_storage;

    in_flux_meters_per_second = 1.6;

    MultipleOutletReservoir->response_meters_per_second(in_flux_meters_per_second, 10, excess);

    final_storage = MultipleOutletReservoir->get_storage_height_meters();

    final_storage = round( final_storage * 100.0 ) / 100.0;

    EXPECT_DOUBLE_EQ (13.76, final_storage);

    ASSERT_TRUE(true);
}

//Test Nonlinear Reservoir with multiple outlets that are initialized not in order from lowest to highest activation threshold
TEST_F(NonlinearReservoirKernelTest, TestRunMultipleOutletOutOfOrderNonlinearReservoir) 
{    
    double in_flux_meters_per_second;
    double excess;
    double final_storage;

    in_flux_meters_per_second = 1.6;

    MultipleOutletOutOfOrderNonlinearReservoir->response_meters_per_second(in_flux_meters_per_second, 10, excess);

    final_storage = MultipleOutletOutOfOrderNonlinearReservoir->get_storage_height_meters();

    final_storage = round( final_storage * 100.0 ) / 100.0;

    EXPECT_DOUBLE_EQ (13.76, final_storage);

    ASSERT_TRUE(true);
}

