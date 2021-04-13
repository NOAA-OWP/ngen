#include <vector>
#include "gtest/gtest.h"
#include "reservoir/Reservoir.hpp"
#include "reservoir/Reservoir_Outlet.hpp"
#include "reservoir/Reservoir_Linear_Outlet.hpp"
#include "reservoir/Reservoir_Exponential_Outlet.hpp"
#include <memory>

//This class contains unit tests for the Reservoir
class ReservoirKernelTest : public ::testing::Test {

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

    std::shared_ptr<Reservoir::Explicit_Time::Reservoir> NoOutletReservoir; //smart pointer to a Reservoir with no outlets

    std::shared_ptr<Reservoir::Explicit_Time::Reservoir> NoOutletReservoir2; //smart pointer to a Reservoir with no outlets

    std::shared_ptr<Reservoir::Explicit_Time::Reservoir> NoOutletReservoir3; //smart pointer to a Reservoir with no outlets

    std::shared_ptr<Reservoir::Explicit_Time::Reservoir> OneOutletReservoir; //smart pointer to a Reservoir with one outlet

    std::shared_ptr<Reservoir::Explicit_Time::Reservoir> OneOutletHighStorageReservoir; //smart pointer to a Reservoir with one outlet and high storage

    std::shared_ptr<Reservoir::Explicit_Time::Reservoir> MultipleOutletReservoir; //smart pointer to a Reservoir with multiple outlets

    std::shared_ptr<Reservoir::Explicit_Time::Reservoir> MultipleOutletReservoirTooLargeActivationThreshold; //smart pointer to a Reservoir with multiple outlets

    std::shared_ptr<Reservoir::Explicit_Time::Reservoir> MultipleOutletOutOfOrderReservoir; //smart pointer to a Reservoir with multiple outlets out of order

    std::shared_ptr<Reservoir::Explicit_Time::Reservoir> MultipleOutletSameActivationHeightReservoir; //smart pointer to a Reservoir with multiple outlets

    std::shared_ptr<Reservoir::Explicit_Time::Reservoir> SingleExponentialOutletReservoir; //smart pointer to a Reservoir with one exponential outlet

    std::shared_ptr<Reservoir::Explicit_Time::Reservoir_Outlet> ReservoirOutlet1; //smart pointer to a Reservoir Outlet

    std::shared_ptr<Reservoir::Explicit_Time::Reservoir_Outlet> ReservoirOutlet2; //smart pointer to a Reservoir Outlet

    std::shared_ptr<Reservoir::Explicit_Time::Reservoir_Outlet> ReservoirOutlet3; //smart pointer to a Reservoir Outlet

    std::shared_ptr<Reservoir::Explicit_Time::Reservoir_Outlet> ReservoirOutlet4; //smart pointer to a Reservoir Outlet

    std::shared_ptr<Reservoir::Explicit_Time::Reservoir_Outlet> ReservoirOutlet5; //smart pointer to a Reservoir Outlet

    std::shared_ptr<Reservoir::Explicit_Time::Reservoir_Outlet> ReservoirOutlet6; //smart pointer to a Reservoir Outlet

    std::shared_ptr<Reservoir::Explicit_Time::Reservoir_Outlet> ReservoirOutlet7; //smart pointer to a Reservoir Outlet

    std::shared_ptr<Reservoir::Explicit_Time::Reservoir_Outlet> ReservoirExponentialOutlet; //smart pointer to a Reservoir Outlet

    std::shared_ptr<Reservoir::Explicit_Time::Reservoir_Outlet> ReservoirLinearOutlet; //smart pointer to a Reservoir Outlet    

    std::vector <std::shared_ptr<Reservoir::Explicit_Time::Reservoir_Outlet>> ReservoirOutletsVector;

    std::vector <std::shared_ptr<Reservoir::Explicit_Time::Reservoir_Outlet>> ReservoirOutletsVector2;

    std::vector <std::shared_ptr<Reservoir::Explicit_Time::Reservoir_Outlet>> ReservoirOutletsVector3;

    std::vector <std::shared_ptr<Reservoir::Explicit_Time::Reservoir_Outlet>> ReservoirOutletsVectorOutOfOrder;

    std::vector <std::shared_ptr<Reservoir::Explicit_Time::Reservoir_Outlet>> ReservoirExponentialSingleOutletVector;

    std::vector <std::shared_ptr<Reservoir::Explicit_Time::Reservoir_Outlet>> ReservoirOutletsVectorMultipleTypes;    
};

void ReservoirKernelTest::SetUp() {
    
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

void ReservoirKernelTest::TearDown() {

}

//Construct a reservoir with no outlets
void ReservoirKernelTest::setupNoOutletReservoir()
{
    NoOutletReservoir = std::make_shared<Reservoir::Explicit_Time::Reservoir>(0.0, 8.0, 2.0);
}

//Construct a reservoir with no outlets and then add a linear and a nonlinear outlet
void ReservoirKernelTest::setupNoOutletReservoir2()
{
    NoOutletReservoir2 = std::make_shared<Reservoir::Explicit_Time::Reservoir>(0.0, 8.0, 2.0);

    ReservoirLinearOutlet = std::make_shared<Reservoir::Explicit_Time::Reservoir_Linear_Outlet>(0.2, 6.0, 100.0);

    NoOutletReservoir2->add_outlet(ReservoirLinearOutlet);

    NoOutletReservoir2->add_outlet(0.3, 0.5, 0.0, 100.0);
}

//Construct a reservoir with one outlet
void ReservoirKernelTest::setupOneOutletReservoir()
{
    OneOutletReservoir = std::make_shared<Reservoir::Explicit_Time::Reservoir>(0.0, 8.0, 3.5, 0.5, 0.7, 4.0, 100.0);
}

//Construct a reservoir with one outlet and high storage
void ReservoirKernelTest::setupOneOutletHighStorageReservoir()
{
    OneOutletHighStorageReservoir = std::make_shared<Reservoir::Explicit_Time::Reservoir>(0.0, 8000.0, 3.5, 1.1, 1.2, 4.0, 0.005);
}

//Construct a reservoir with multiple outlets in already sorted order
void ReservoirKernelTest::setupMultipleOutletReservoir()
{
    //ReservoirOutlet1 = std::make_shared<Reservoir_Outlet>(0.2, 0.4, 4.0, 100.0);

    ReservoirOutlet1 = std::make_shared<Reservoir::Explicit_Time::Reservoir_Linear_Outlet>(0.2, 4.0, 100.0);

    ReservoirOutlet2 = std::make_shared<Reservoir::Explicit_Time::Reservoir_Outlet>(0.3, 0.5, 10.0, 100.0);

    ReservoirOutlet3 = std::make_shared<Reservoir::Explicit_Time::Reservoir_Outlet>(0.4, 0.6, 15.0, 100.0);

    ReservoirOutletsVector.push_back(ReservoirOutlet1);

    ReservoirOutletsVector.push_back(ReservoirOutlet2);

    ReservoirOutletsVector.push_back(ReservoirOutlet3);

    MultipleOutletReservoir = std::make_shared<Reservoir::Explicit_Time::Reservoir>(0.0, 20.0, 2.0, ReservoirOutletsVector);
}

//Construct a reservoir with multiple outlets with one outlet having an activation threshold above the max reservoir storage
void ReservoirKernelTest::setupMultipleOutletReservoirTooLargeActivationThreshold()
{
    ReservoirOutlet6 = std::make_shared<Reservoir::Explicit_Time::Reservoir_Linear_Outlet>(0.2, 15.0, 100.0);

    ReservoirOutlet7 = std::make_shared<Reservoir::Explicit_Time::Reservoir_Outlet>(0.3, 0.5, 10.0, 100.0);

    ReservoirOutletsVector3.push_back(ReservoirOutlet6);

    ReservoirOutletsVector3.push_back(ReservoirOutlet7);

    //MultipleOutletReservoirTooLargeActivationThreshold = std::make_shared<Reservoir::Explicit_Time::Reservoir>(0.0, 12.0, 2.0, ReservoirOutletsVector3);
}


//Construct a reservoir with multiple outlets of same activation height
void ReservoirKernelTest::setupMultipleOutletSameActivationHeightReservoir()
{
    ReservoirOutlet4 = std::make_shared<Reservoir::Explicit_Time::Reservoir_Linear_Outlet>(0.2, 8.0, 100.0);

    ReservoirOutlet5 = std::make_shared<Reservoir::Explicit_Time::Reservoir_Outlet>(0.3, 0.5, 8.0, 100.0);

    ReservoirOutletsVector2.push_back(ReservoirOutlet4);

    ReservoirOutletsVector2.push_back(ReservoirOutlet5);

    MultipleOutletSameActivationHeightReservoir = std::make_shared<Reservoir::Explicit_Time::Reservoir>(0.0, 20.0, 2.0, ReservoirOutletsVector2);
}

//Construct a reservoir with multiple outlets that are not ordered from lowest to highest activation threshold
void ReservoirKernelTest::setupMultipleOutletOutOfOrderReservoir()
{
    ReservoirOutlet1 = std::make_shared<Reservoir::Explicit_Time::Reservoir_Outlet>(0.2, 0.4, 4.0, 100.0);

    ReservoirOutlet2 = std::make_shared<Reservoir::Explicit_Time::Reservoir_Outlet>(0.3, 0.5, 10.0, 100.0);

    ReservoirOutlet3 = std::make_shared<Reservoir::Explicit_Time::Reservoir_Outlet>(0.4, 0.6, 15.0, 100.0);

    ReservoirOutletsVectorOutOfOrder.push_back(ReservoirOutlet3);

    ReservoirOutletsVectorOutOfOrder.push_back(ReservoirOutlet2);

    ReservoirOutletsVectorOutOfOrder.push_back(ReservoirOutlet1);

    MultipleOutletOutOfOrderReservoir = std::make_shared<Reservoir::Explicit_Time::Reservoir>(0.0, 20.0, 2.0, ReservoirOutletsVectorOutOfOrder);
}

//Construct a reservoir with one exponential outlet
void ReservoirKernelTest:: setupExponentialOutletReservoir()
{
    ReservoirExponentialOutlet = std::make_shared<Reservoir::Explicit_Time::Reservoir_Exponential_Outlet>(0.2, 0.4, 4.0, 100.0);

    ReservoirOutlet1 = std::make_shared<Reservoir::Explicit_Time::Reservoir_Outlet>(0.2, 0.4, 4.0, 100.0);

    ReservoirExponentialSingleOutletVector.push_back(ReservoirExponentialOutlet);

    //Test added different type outlets to outlet vector
    ReservoirOutletsVectorMultipleTypes.push_back(ReservoirOutlet1);

    ReservoirOutletsVectorMultipleTypes.push_back(ReservoirExponentialOutlet);

    SingleExponentialOutletReservoir = std::make_shared<Reservoir::Explicit_Time::Reservoir>(0.0, 20.0, 2.0, ReservoirExponentialSingleOutletVector);
}

//Test Reservoir with no outlets.
TEST_F(ReservoirKernelTest, TestRunNoOutletReservoir) 
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

//Test Reservoir with two outlets.
TEST_F(ReservoirKernelTest, TestRunTwoOutletReservoir) 
{
    double in_flux_meters_per_second;
    double excess;
    double final_storage;

    in_flux_meters_per_second = 0.2;

    NoOutletReservoir2->response_meters_per_second(in_flux_meters_per_second, 10, excess);

    final_storage = NoOutletReservoir2->get_storage_height_meters();

    final_storage = round( final_storage * 100.0 ) / 100.0;

    EXPECT_DOUBLE_EQ (1.88, final_storage);

    ASSERT_TRUE(true);
}

//Test Reservoir with one outlet.
TEST_F(ReservoirKernelTest, TestRunOneOutletReservoir) 
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

//Test Reservoir with one outlet where the storage remains below the activation threshold.
TEST_F(ReservoirKernelTest, TestRunOneOutletReservoirNoActivation) 
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


//Test Reservoir with one outlet where the storage exceeds max storage.
TEST_F(ReservoirKernelTest, TestRunOneOutletReservoirExceedMaxStorage) 
{   
    double in_flux_meters_per_second;
    double excess;
    double final_storage;

    in_flux_meters_per_second = 5.0;

    OneOutletReservoir->response_meters_per_second(in_flux_meters_per_second, 10, excess);

    final_storage = OneOutletReservoir->get_storage_height_meters();

    final_storage = round( final_storage * 100.0 ) / 100.0;

    EXPECT_DOUBLE_EQ (8.0, final_storage);

    ASSERT_TRUE(true);
}


//Test Reservoir with one outlet where the storage falls below the minimum storage of zero.
TEST_F(ReservoirKernelTest, TestRunOneOutletReservoirFallsBelowMinimumStorage) 
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


//Test Reservoir with one outlet where the calculated outlet velocity exceeds max velocity.
TEST_F(ReservoirKernelTest, TestRunOneOutletReservoirExceedMaxVelocity) 
{   
    double in_flux_meters_per_second;
    double excess;
    double final_storage;

    in_flux_meters_per_second = 80.0;

    OneOutletHighStorageReservoir->response_meters_per_second(in_flux_meters_per_second, 10, excess);

    final_storage = OneOutletHighStorageReservoir->get_storage_height_meters();

    final_storage = round( final_storage * 100.0 ) / 100.0;

    EXPECT_DOUBLE_EQ (803.450, final_storage);

    ASSERT_TRUE(true);
}


//Test Reservoir with multiple outlets
TEST_F(ReservoirKernelTest, TestRunMultipleOutletReservoir) 
{    
    double in_flux_meters_per_second;
    double excess;
    double final_storage;

    in_flux_meters_per_second = 1.6;

    MultipleOutletReservoir->response_meters_per_second(in_flux_meters_per_second, 10, excess);

    final_storage = MultipleOutletReservoir->get_storage_height_meters();

    final_storage = round( final_storage * 100.0 ) / 100.0;

    EXPECT_DOUBLE_EQ (13.88, final_storage);

    ASSERT_TRUE(true);
}


//Test Reservoir with multiple outlets that are initialized not in order from lowest to highest activation threshold
//and are then sorted from lowest to highest activation threshold
TEST_F(ReservoirKernelTest, TestRunMultipleOutletOutOfOrderReservoir) 
{    
    double in_flux_meters_per_second;
    double excess;
    double final_storage;

    in_flux_meters_per_second = 1.6;

    MultipleOutletOutOfOrderReservoir->response_meters_per_second(in_flux_meters_per_second, 10, excess);

    final_storage = MultipleOutletOutOfOrderReservoir->get_storage_height_meters();

    final_storage = round( final_storage * 100.0 ) / 100.0;

    EXPECT_DOUBLE_EQ (13.76, final_storage);

    ASSERT_TRUE(true);
}


//Test Reservoir with multiple outlets accessor to an outlet velocity
TEST_F(ReservoirKernelTest, TestRunMultipleOutletReservoirOutletVelocity) 
{    
    double in_flux_meters_per_second;
    double excess;
    double final_storage;
    double second_outlet_velocity;

    in_flux_meters_per_second = 1.6;

    MultipleOutletReservoir->response_meters_per_second(in_flux_meters_per_second, 10, excess);

    second_outlet_velocity = MultipleOutletReservoir->velocity_meters_per_second_for_outlet(1);

    second_outlet_velocity = round( second_outlet_velocity * 100.0 ) / 100.0;

    EXPECT_DOUBLE_EQ (0.24, second_outlet_velocity);
    
    ASSERT_TRUE(true);
}


//Test Reservoir with multiple outlets accessor to an outlet velocity from an outlet index not present
TEST_F(ReservoirKernelTest, TestRunMultipleOutletReservoirOutletVelocityOutOfBounds) 
{    
    double in_flux_meters_per_second;
    double excess;
    double final_storage;
    double first_outlet_velocity;

    in_flux_meters_per_second = 8.6;

    MultipleOutletReservoir->response_meters_per_second(in_flux_meters_per_second, 10, excess);

    first_outlet_velocity = MultipleOutletReservoir->velocity_meters_per_second_for_outlet(2);

    first_outlet_velocity = round( first_outlet_velocity * 100.0 ) / 100.0;

    EXPECT_DOUBLE_EQ (0.40, first_outlet_velocity);
    
    ASSERT_TRUE(true);
}


//Test Reservoir with no outlets accessor to an outlet velocity
TEST_F(ReservoirKernelTest, TestRunNoOutletReservoirOutletVelocityAccessor) 
{    
    double in_flux_meters;
    double excess;
    double final_storage;
    double first_outlet_velocity;

    in_flux_meters = 8.6;

    NoOutletReservoir->response_meters_per_second(in_flux_meters, 10, excess);

    first_outlet_velocity = NoOutletReservoir->velocity_meters_per_second_for_outlet(0);

    first_outlet_velocity = round( first_outlet_velocity * 100.0 ) / 100.0;

    EXPECT_DOUBLE_EQ (0.00, first_outlet_velocity);
    
    ASSERT_TRUE(true);
}


//Test Reservoir with one exponential outlet 
TEST_F(ReservoirKernelTest, TestRunSingleExponentialOutletReservoir) 
{    
    double in_flux_meters_per_second;
    double excess;
    double final_storage;
    double exponential_outlet_velocity;

    in_flux_meters_per_second = 1.6;

    SingleExponentialOutletReservoir->response_meters_per_second(in_flux_meters_per_second, 10, excess);

    exponential_outlet_velocity = SingleExponentialOutletReservoir->velocity_meters_per_second_for_outlet(0);

    exponential_outlet_velocity = round( exponential_outlet_velocity * 100.0 ) / 100.0;

    EXPECT_DOUBLE_EQ (0.09, exponential_outlet_velocity);
    
    ASSERT_TRUE(true);
}


TEST_F(ReservoirKernelTest, TestReservoirAddOutlet)
{
    NoOutletReservoir3 = std::make_shared<Reservoir::Explicit_Time::Reservoir>(0.0, 8.0, 2.0);

    NoOutletReservoir3->add_outlet(ReservoirLinearOutlet);

    NoOutletReservoir3->add_outlet(0.3, 0.5, 0.0, 100.0);

    NoOutletReservoir3->add_outlet(0.3, 4.5, 100.0);

    ASSERT_TRUE(true);
}


TEST_F(ReservoirKernelTest, TestReservoirSortOutlets)
{
    MultipleOutletOutOfOrderReservoir->sort_outlets();

    ASSERT_TRUE(true);
}


TEST_F(ReservoirKernelTest, TestReservoirCheckHighestOutlet)
{
    MultipleOutletOutOfOrderReservoir->check_highest_outlet_against_max_storage();

    ASSERT_TRUE(true);
}


TEST_F(ReservoirKernelTest, TestAdjustReservoirOutletVelocity)
{
    ReservoirOutlet1->adjust_velocity(5.5);

    ASSERT_TRUE(true);
}


TEST_F(ReservoirKernelTest, TestReservoirOutletGetActivationThreshold)
{
    double activation_threshold_meters;

    activation_threshold_meters= ReservoirOutlet1->get_activation_threshold_meters();

    EXPECT_DOUBLE_EQ (4.0, activation_threshold_meters);

    ASSERT_TRUE(true);
}

//Test Reservoir with multiple outlets of same activation threshold height
TEST_F(ReservoirKernelTest, TestRunMultipleOutletSameActivationHeightReservoir) 
{    
    double in_flux_meters_per_second;
    double excess;
    double final_storage;

    in_flux_meters_per_second = 1.6;

    MultipleOutletSameActivationHeightReservoir->response_meters_per_second(in_flux_meters_per_second, 10, excess);

    final_storage = MultipleOutletSameActivationHeightReservoir->get_storage_height_meters();

    final_storage = round( final_storage * 100.0 ) / 100.0;

    EXPECT_DOUBLE_EQ (13.83 , final_storage);

    ASSERT_TRUE(true);
}

//Test to ensure that program exits with given error message whenever an outlet activation threshold is above the
//max reservoir storage
TEST_F(ReservoirKernelTest, TestMultipleOutletReservoirTooLargeActivationThreshold)
{
    ASSERT_DEATH(MultipleOutletReservoirTooLargeActivationThreshold = std::make_shared<Reservoir::Explicit_Time::Reservoir>(0.0, 12.0, 2.0, ReservoirOutletsVector3), "ERROR: The activation_threshold_meters is greater than the maximum_storage_meters of a reservoir.");
}


