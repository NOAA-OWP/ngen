#include <vector>
#include "gtest/gtest.h"
#include "reservoir/Reservoir_Timeless.hpp"
#include "reservoir/Reservoir_Outlet_Timeless.hpp"
#include "reservoir/Reservoir_Linear_Outlet_Timeless.hpp"
#include "reservoir/Reservoir_Exponential_Outlet_Timeless.hpp"
#include <memory>

//This class contains unit tests for the Reservoir
class ReservoirTimelessKernelTest : public ::testing::Test {

    protected:

    void SetUp() override;

    void TearDown() override;

    void setupNoOutletReservoir();

    void setupNoOutletReservoir2();

    void setupOneOutletReservoir();

    void setupOneOutletHighStorageReservoir();

    void setupMultipleOutletReservoir();

    void setupMultipleOutletSameActivationHeightReservoir();

    void setupMultipleOutletReservoirTooLargeActivationThreshold();

    void setupMultipleOutletOutOfOrderReservoir();

    void setupExponentialOutletReservoir();

    std::shared_ptr<Reservoir::Implicit_Time::Reservoir> NoOutletReservoir; //smart pointer to a Reservoir with no outlets

    std::shared_ptr<Reservoir::Implicit_Time::Reservoir> NoOutletReservoir2; //smart pointer to a Reservoir with no outlets

    std::shared_ptr<Reservoir::Implicit_Time::Reservoir> NoOutletReservoir3; //smart pointer to a Reservoir with no outlets

    std::shared_ptr<Reservoir::Implicit_Time::Reservoir> OneOutletReservoir; //smart pointer to a Reservoir with one outlet

    std::shared_ptr<Reservoir::Implicit_Time::Reservoir> OneOutletHighStorageReservoir; //smart pointer to a Reservoir with one outlet and high storage

    std::shared_ptr<Reservoir::Implicit_Time::Reservoir> MultipleOutletReservoir; //smart pointer to a Reservoir with multiple outlets

    std::shared_ptr<Reservoir::Implicit_Time::Reservoir> MultipleOutletReservoirTooLargeActivationThreshold; //smart pointer to a Reservoir with multiple outlets

    std::shared_ptr<Reservoir::Implicit_Time::Reservoir> MultipleOutletOutOfOrderReservoir; //smart pointer to a Reservoir with multiple outlets out of order

    std::shared_ptr<Reservoir::Implicit_Time::Reservoir> MultipleOutletSameActivationHeightReservoir; //smart pointer to a Reservoir with multiple outlets

    std::shared_ptr<Reservoir::Implicit_Time::Reservoir> SingleExponentialOutletReservoir; //smart pointer to a Reservoir with one exponential outlet

    std::shared_ptr<Reservoir::Implicit_Time::Reservoir_Outlet> ReservoirOutlet1; //smart pointer to a Reservoir Outlet

    std::shared_ptr<Reservoir::Implicit_Time::Reservoir_Outlet> ReservoirOutlet2; //smart pointer to a Reservoir Outlet

    std::shared_ptr<Reservoir::Implicit_Time::Reservoir_Outlet> ReservoirOutlet3; //smart pointer to a Reservoir Outlet

    std::shared_ptr<Reservoir::Implicit_Time::Reservoir_Outlet> ReservoirOutlet4; //smart pointer to a Reservoir Outlet

    std::shared_ptr<Reservoir::Implicit_Time::Reservoir_Outlet> ReservoirOutlet5; //smart pointer to a Reservoir Outlet

    std::shared_ptr<Reservoir::Implicit_Time::Reservoir_Outlet> ReservoirOutlet6; //smart pointer to a Reservoir Outlet

    std::shared_ptr<Reservoir::Implicit_Time::Reservoir_Outlet> ReservoirOutlet7; //smart pointer to a Reservoir Outlet

    std::shared_ptr<Reservoir::Implicit_Time::Reservoir_Outlet> ReservoirExponentialOutlet; //smart pointer to a Reservoir Outlet

    std::shared_ptr<Reservoir::Implicit_Time::Reservoir_Outlet> ReservoirLinearOutlet; //smart pointer to a Reservoir Outlet    

    std::vector <std::shared_ptr<Reservoir::Implicit_Time::Reservoir_Outlet>> ReservoirOutletsVector;

    std::vector <std::shared_ptr<Reservoir::Implicit_Time::Reservoir_Outlet>> ReservoirOutletsVector2;

    std::vector <std::shared_ptr<Reservoir::Implicit_Time::Reservoir_Outlet>> ReservoirOutletsVector3;

    std::vector <std::shared_ptr<Reservoir::Implicit_Time::Reservoir_Outlet>> ReservoirOutletsVectorOutOfOrder;

    std::vector <std::shared_ptr<Reservoir::Implicit_Time::Reservoir_Outlet>> ReservoirExponentialSingleOutletVector;

    std::vector <std::shared_ptr<Reservoir::Implicit_Time::Reservoir_Outlet>> ReservoirOutletsVectorMultipleTypes;    
};

void ReservoirTimelessKernelTest::SetUp() {
    
    setupNoOutletReservoir();

    setupNoOutletReservoir2();

    setupOneOutletReservoir();

    setupOneOutletHighStorageReservoir();

    setupMultipleOutletReservoir();

    setupMultipleOutletSameActivationHeightReservoir();

    setupMultipleOutletOutOfOrderReservoir();

    setupMultipleOutletReservoirTooLargeActivationThreshold();

    setupExponentialOutletReservoir();
}

void ReservoirTimelessKernelTest::TearDown() {

}

//Construct a reservoir with no outlets
void ReservoirTimelessKernelTest::setupNoOutletReservoir()
{
    NoOutletReservoir = std::make_shared<Reservoir::Implicit_Time::Reservoir>(0.0, 8.0, 2.0);
}

//Construct a reservoir with no outlets and then add a linear and a nonlinear outlet
void ReservoirTimelessKernelTest::setupNoOutletReservoir2()
{
    NoOutletReservoir2 = std::make_shared<Reservoir::Implicit_Time::Reservoir>(0.0, 8.0, 2.0);

    ReservoirLinearOutlet = std::make_shared<Reservoir::Implicit_Time::Reservoir_Linear_Outlet>(0.2, 6.0, 100.0);

    NoOutletReservoir2->add_outlet(ReservoirLinearOutlet);

    NoOutletReservoir2->add_outlet(0.3, 0.5, 0.0, 100.0);
}

//Construct a reservoir with one outlet
void ReservoirTimelessKernelTest::setupOneOutletReservoir()
{
    OneOutletReservoir = std::make_shared<Reservoir::Implicit_Time::Reservoir>(0.0, 8.0, 3.5, 0.5, 0.7, 4.0, 100.0);
}

//Construct a reservoir with one outlet and high storage
void ReservoirTimelessKernelTest::setupOneOutletHighStorageReservoir()
{
    OneOutletHighStorageReservoir = std::make_shared<Reservoir::Implicit_Time::Reservoir>(0.0, 8000.0, 3.5, 1.1, 1.2, 4.0, 0.005);
}

//Construct a reservoir with multiple outlets in already sorted order
void ReservoirTimelessKernelTest::setupMultipleOutletReservoir()
{
    //ReservoirOutlet1 = std::make_shared<Reservoir_Outlet>(0.2, 0.4, 4.0, 100.0);

    ReservoirOutlet1 = std::make_shared<Reservoir::Implicit_Time::Reservoir_Linear_Outlet>(0.2, 4.0, 100.0);

    ReservoirOutlet2 = std::make_shared<Reservoir::Implicit_Time::Reservoir_Outlet>(0.3, 0.5, 10.0, 100.0);

    ReservoirOutlet3 = std::make_shared<Reservoir::Implicit_Time::Reservoir_Outlet>(0.4, 0.6, 15.0, 100.0);

    ReservoirOutletsVector.push_back(ReservoirOutlet1);

    ReservoirOutletsVector.push_back(ReservoirOutlet2);

    ReservoirOutletsVector.push_back(ReservoirOutlet3);

    MultipleOutletReservoir = std::make_shared<Reservoir::Implicit_Time::Reservoir>(0.0, 20.0, 2.0, ReservoirOutletsVector);
}

//Construct a reservoir with multiple outlets with one outlet having an activation threshold above the max reservoir storage
void ReservoirTimelessKernelTest::setupMultipleOutletReservoirTooLargeActivationThreshold()
{
    ReservoirOutlet6 = std::make_shared<Reservoir::Implicit_Time::Reservoir_Linear_Outlet>(0.2, 15.0, 100.0);

    ReservoirOutlet7 = std::make_shared<Reservoir::Implicit_Time::Reservoir_Outlet>(0.3, 0.5, 10.0, 100.0);

    ReservoirOutletsVector3.push_back(ReservoirOutlet6);

    ReservoirOutletsVector3.push_back(ReservoirOutlet7);

    //MultipleOutletReservoirTooLargeActivationThreshold = std::make_shared<Reservoir::Implicit_Time::Reservoir>(0.0, 12.0, 2.0, ReservoirOutletsVector3);
}


//Construct a reservoir with multiple outlets of same activation height
void ReservoirTimelessKernelTest::setupMultipleOutletSameActivationHeightReservoir()
{
    ReservoirOutlet4 = std::make_shared<Reservoir::Implicit_Time::Reservoir_Linear_Outlet>(0.2, 8.0, 100.0);

    ReservoirOutlet5 = std::make_shared<Reservoir::Implicit_Time::Reservoir_Outlet>(0.3, 0.5, 8.0, 100.0);

    ReservoirOutletsVector2.push_back(ReservoirOutlet4);

    ReservoirOutletsVector2.push_back(ReservoirOutlet5);

    MultipleOutletSameActivationHeightReservoir = std::make_shared<Reservoir::Implicit_Time::Reservoir>(0.0, 20.0, 2.0, ReservoirOutletsVector2);
}

//Construct a reservoir with multiple outlets that are not ordered from lowest to highest activation threshold
void ReservoirTimelessKernelTest::setupMultipleOutletOutOfOrderReservoir()
{
    ReservoirOutlet1 = std::make_shared<Reservoir::Implicit_Time::Reservoir_Outlet>(0.2, 0.4, 4.0, 100.0);

    ReservoirOutlet2 = std::make_shared<Reservoir::Implicit_Time::Reservoir_Outlet>(0.3, 0.5, 10.0, 100.0);

    ReservoirOutlet3 = std::make_shared<Reservoir::Implicit_Time::Reservoir_Outlet>(0.4, 0.6, 15.0, 100.0);

    ReservoirOutletsVectorOutOfOrder.push_back(ReservoirOutlet3);

    ReservoirOutletsVectorOutOfOrder.push_back(ReservoirOutlet2);

    ReservoirOutletsVectorOutOfOrder.push_back(ReservoirOutlet1);

    MultipleOutletOutOfOrderReservoir = std::make_shared<Reservoir::Implicit_Time::Reservoir>(0.0, 20.0, 2.0, ReservoirOutletsVectorOutOfOrder);
}

//Construct a reservoir with one exponential outlet
void ReservoirTimelessKernelTest:: setupExponentialOutletReservoir()
{
    ReservoirExponentialOutlet = std::make_shared<Reservoir::Implicit_Time::Reservoir_Exponential_Outlet>(0.2, 0.4, 4.0, 100.0);

    ReservoirOutlet1 = std::make_shared<Reservoir::Implicit_Time::Reservoir_Outlet>(0.2, 0.4, 4.0, 100.0);

    ReservoirExponentialSingleOutletVector.push_back(ReservoirExponentialOutlet);

    //Test added different type outlets to outlet vector
    ReservoirOutletsVectorMultipleTypes.push_back(ReservoirOutlet1);

    ReservoirOutletsVectorMultipleTypes.push_back(ReservoirExponentialOutlet);

    SingleExponentialOutletReservoir = std::make_shared<Reservoir::Implicit_Time::Reservoir>(0.0, 20.0, 2.0, ReservoirExponentialSingleOutletVector);
}

//Test Reservoir with no outlets.
TEST_F(ReservoirTimelessKernelTest, TestRunNoOutletReservoir) 
{   
    double in_flux_meters;
    double excess;
    double final_storage;

    in_flux_meters = 0.05;

    NoOutletReservoir->response_meters(in_flux_meters, excess);

    final_storage = NoOutletReservoir->get_storage_height_meters();

    final_storage = round( final_storage * 100.0 ) / 100.0;

    EXPECT_DOUBLE_EQ (2.05, final_storage);

    ASSERT_TRUE(true);
}

//Test Reservoir with two outlets.
TEST_F(ReservoirTimelessKernelTest, TestRunTwoOutletReservoir) 
{
    double in_flux_meters;
    double excess;
    double final_storage;

    in_flux_meters = 0.2;

    NoOutletReservoir2->response_meters(in_flux_meters, excess);

    final_storage = NoOutletReservoir2->get_storage_height_meters();

    final_storage = round( final_storage * 100.0 ) / 100.0;

    EXPECT_DOUBLE_EQ (2.04, final_storage);

    ASSERT_TRUE(true);
}

//Test Reservoir with one outlet.
TEST_F(ReservoirTimelessKernelTest, TestRunOneOutletReservoir) 
{
    double in_flux_meters;
    double excess;
    double final_storage;

    in_flux_meters = 0.2;

    OneOutletReservoir->response_meters(in_flux_meters, excess);

    final_storage = OneOutletReservoir->get_storage_height_meters();

    final_storage = round( final_storage * 100.0 ) / 100.0;

    EXPECT_DOUBLE_EQ (3.7, final_storage);

    ASSERT_TRUE(true);
}

//Test Reservoir with one outlet where the storage remains below the activation threshold.
TEST_F(ReservoirTimelessKernelTest, TestRunOneOutletReservoirNoActivation) 
{   
    double in_flux_meters;
    double excess;
    double final_storage;

    in_flux_meters = 0.02;

    OneOutletReservoir->response_meters(in_flux_meters, excess);

    final_storage = OneOutletReservoir->get_storage_height_meters();

    final_storage = round( final_storage * 100.0 ) / 100.0;

    EXPECT_DOUBLE_EQ (3.52, final_storage);

    ASSERT_TRUE(true);
}


//Test Reservoir with one outlet where the storage exceeds max storage.
TEST_F(ReservoirTimelessKernelTest, TestRunOneOutletReservoirExceedMaxStorage) 
{   
    double in_flux_meters;
    double excess;
    double final_storage;

    in_flux_meters = 10.0;

    OneOutletReservoir->response_meters(in_flux_meters, excess);

    final_storage = OneOutletReservoir->get_storage_height_meters();

    final_storage = round( final_storage * 100.0 ) / 100.0;

    EXPECT_DOUBLE_EQ (8.0, final_storage);

    ASSERT_TRUE(true);
}


//Test Reservoir with one outlet where the storage falls below the minimum storage of zero.
TEST_F(ReservoirTimelessKernelTest, TestRunOneOutletReservoirFallsBelowMinimumStorage) 
{   
    double in_flux_meters;
    double excess;
    double final_storage;

    in_flux_meters = -10.0;

    OneOutletReservoir->response_meters(in_flux_meters, excess);

    final_storage = OneOutletReservoir->get_storage_height_meters();

    final_storage = round( final_storage * 100.0 ) / 100.0;

    EXPECT_DOUBLE_EQ (0.0, final_storage);

    ASSERT_TRUE(true);
}


//Test Reservoir with one outlet where the calculated outlet flux exceeds max flux.
TEST_F(ReservoirTimelessKernelTest, TestRunOneOutletReservoirExceedMaxFlux) 
{   
    double in_flux_meters;
    double excess;
    double final_storage;

    in_flux_meters = 180.0;

    OneOutletHighStorageReservoir->response_meters(in_flux_meters, excess);

    final_storage = OneOutletHighStorageReservoir->get_storage_height_meters();

    final_storage = round( final_storage * 100.0 ) / 100.0;

    EXPECT_DOUBLE_EQ (183.5, final_storage);

    ASSERT_TRUE(true);
}


//Test Reservoir with multiple outlets
TEST_F(ReservoirTimelessKernelTest, TestRunMultipleOutletReservoir) 
{    
    double in_flux_meters;
    double excess;
    double final_storage;

    in_flux_meters = 1.6;

    MultipleOutletReservoir->response_meters(in_flux_meters, excess);

    final_storage = MultipleOutletReservoir->get_storage_height_meters();

    final_storage = round( final_storage * 100.0 ) / 100.0;

    EXPECT_DOUBLE_EQ (3.6, final_storage);

    ASSERT_TRUE(true);
}


//Test Reservoir with multiple outlets that are initialized not in order from lowest to highest activation threshold
//and are then sorted from lowest to highest activation threshold
TEST_F(ReservoirTimelessKernelTest, TestRunMultipleOutletOutOfOrderReservoir) 
{    
    double in_flux_meters;
    double excess;
    double final_storage;

    in_flux_meters = 1.6;

    MultipleOutletOutOfOrderReservoir->response_meters(in_flux_meters, excess);

    final_storage = MultipleOutletOutOfOrderReservoir->get_storage_height_meters();

    final_storage = round( final_storage * 100.0 ) / 100.0;

    EXPECT_DOUBLE_EQ (3.6, final_storage);

    ASSERT_TRUE(true);
}


//Test Reservoir with multiple outlets accessor to an outlet flux
TEST_F(ReservoirTimelessKernelTest, TestRunMultipleOutletReservoirOutletFlux) 
{    
    double in_flux_meters;
    double excess;
    double final_storage;
    double second_outlet_flux;

    in_flux_meters = 8.6;

    MultipleOutletReservoir->response_meters(in_flux_meters, excess);

    second_outlet_flux = MultipleOutletReservoir->flux_meters_for_outlet(1);

    second_outlet_flux = round( second_outlet_flux * 100.0 ) / 100.0;

    EXPECT_DOUBLE_EQ (0.07, second_outlet_flux);
    
    ASSERT_TRUE(true);
}


//Test Reservoir with multiple outlets accessor to an outlet flux from an outlet index not present
TEST_F(ReservoirTimelessKernelTest, TestRunMultipleOutletReservoirOutletFluxOutOfBounds) 
{    
    double in_flux_meters;
    double excess;
    double final_storage;
    double first_outlet_flux;

    in_flux_meters = 8.6;

    MultipleOutletReservoir->response_meters(in_flux_meters, excess);

    first_outlet_flux = MultipleOutletReservoir->flux_meters_for_outlet(6);

    first_outlet_flux = round( first_outlet_flux * 100.0 ) / 100.0;

    EXPECT_DOUBLE_EQ (0.08, first_outlet_flux);
    
    ASSERT_TRUE(true);
}


//Test Reservoir with no outlets accessor to an outlet flux
TEST_F(ReservoirTimelessKernelTest, TestRunNoOutletReservoirOutletFluxAccessor) 
{    
    double in_flux_meters;
    double excess;
    double final_storage;
    double first_outlet_flux;

    in_flux_meters = 8.6;

    NoOutletReservoir->response_meters(in_flux_meters, excess);

    first_outlet_flux = NoOutletReservoir->flux_meters_for_outlet(0);

    first_outlet_flux = round( first_outlet_flux * 100.0 ) / 100.0;

    EXPECT_DOUBLE_EQ (0.00, first_outlet_flux);
    
    ASSERT_TRUE(true);
}


//Test Reservoir with one exponential outlet 
TEST_F(ReservoirTimelessKernelTest, TestRunSingleExponentialOutletReservoir) 
{    
    double in_flux_meters;
    double excess;
    double final_storage;
    double exponential_outlet_flux;

    in_flux_meters = 3.6;

    SingleExponentialOutletReservoir->response_meters(in_flux_meters, excess);

    exponential_outlet_flux = SingleExponentialOutletReservoir->flux_meters_for_outlet(0);

    exponential_outlet_flux = round( exponential_outlet_flux * 100.0 ) / 100.0;

    EXPECT_DOUBLE_EQ (0.02, exponential_outlet_flux);
    
    ASSERT_TRUE(true);
}


TEST_F(ReservoirTimelessKernelTest, TestReservoirAddOutlet)
{
    NoOutletReservoir3 = std::make_shared<Reservoir::Implicit_Time::Reservoir>(0.0, 8.0, 2.0);

    NoOutletReservoir3->add_outlet(ReservoirLinearOutlet);

    NoOutletReservoir3->add_outlet(0.3, 0.5, 0.0, 100.0);

    NoOutletReservoir3->add_outlet(0.3, 4.5, 100.0);

    ASSERT_TRUE(true);
}


TEST_F(ReservoirTimelessKernelTest, TestReservoirSortOutlets)
{
    MultipleOutletOutOfOrderReservoir->sort_outlets();

    ASSERT_TRUE(true);
}


TEST_F(ReservoirTimelessKernelTest, TestReservoirCheckHighestOutlet)
{
    MultipleOutletOutOfOrderReservoir->check_highest_outlet_against_max_storage();

    ASSERT_TRUE(true);
}


TEST_F(ReservoirTimelessKernelTest, TestAdjustReservoirOutletFlux)
{
    ReservoirOutlet1->adjust_flux(5.5);

    ASSERT_TRUE(true);
}


TEST_F(ReservoirTimelessKernelTest, TestReservoirOutletGetActivationThreshold)
{
    double activation_threshold_meters;

    activation_threshold_meters= ReservoirOutlet1->get_activation_threshold_meters();

    EXPECT_DOUBLE_EQ (4.0, activation_threshold_meters);

    ASSERT_TRUE(true);
}

//Test Reservoir with multiple outlets of same activation threshold height
TEST_F(ReservoirTimelessKernelTest, TestRunMultipleOutletSameActivationHeightReservoir) 
{    
    double in_flux_meters;
    double excess;
    double final_storage;

    in_flux_meters = 1.6;

    MultipleOutletSameActivationHeightReservoir->response_meters(in_flux_meters, excess);

    final_storage = MultipleOutletSameActivationHeightReservoir->get_storage_height_meters();

    final_storage = round( final_storage * 100.0 ) / 100.0;

    EXPECT_DOUBLE_EQ (3.6 , final_storage);

    ASSERT_TRUE(true);
}

//Test to ensure that program exits with given error message whenever an outlet activation threshold is above the
//max reservoir storage
TEST_F(ReservoirTimelessKernelTest, TestMultipleOutletReservoirTooLargeActivationThreshold)
{
    ASSERT_DEATH(MultipleOutletReservoirTooLargeActivationThreshold = std::make_shared<Reservoir::Implicit_Time::Reservoir>(0.0, 12.0, 2.0, ReservoirOutletsVector3), "ERROR: The activation_threshold_meters is greater than the maximum_storage_meters of a reservoir.");
}


