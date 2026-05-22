#include <string>
#include <vector>
#include "gtest/gtest.h"
#include "TrivialForcingProvider.hpp"
#include "OptionalWrappedDataProvider.hpp"

using namespace data_access;

class OptionalWrappedDataProvider_Test : public ::testing::Test {
protected:

    void SetUp() override;

    //void TearDown() override;

    test::TrivialForcingProvider backingProvider;

    std::vector<OptionalWrappedDataProvider> providers;

};

void OptionalWrappedDataProvider_Test::SetUp() {
    // Example 0: no waits
    providers.push_back(OptionalWrappedDataProvider(OUTPUT_NAME_1, OUTPUT_DEFAULT_1));
    // Example 1: 1 wait
    providers.push_back(OptionalWrappedDataProvider(OUTPUT_NAME_1, OUTPUT_DEFAULT_1, 1));
    // Example 2: 2 wait
    providers.push_back(OptionalWrappedDataProvider(OUTPUT_NAME_1, OUTPUT_DEFAULT_1, 2));
    // Example 3: no defaults and no waits
    providers.push_back(OptionalWrappedDataProvider(OUTPUT_NAME_1));

    backingProvider = test::TrivialForcingProvider();
}

// TODO: add more test scenarios

/**
 * Test when no override set, before backing provider set.
 */
TEST_F(OptionalWrappedDataProvider_Test, test_is_default_override_0_a) {
    int example_index = 0;

    OptionalWrappedDataProvider &optProvider = providers[example_index];
    //optProvider.setWrappedProvider(&backingProvider);
    // Args don't really matter (apart from the name) for backing trivial item
    bool value = optProvider.isDefaultOverride(OUTPUT_NAME_1);

    ASSERT_FALSE(value);
}

/**
 * Test when no override set, after backing provider set.
 */
TEST_F(OptionalWrappedDataProvider_Test, test_is_default_override_0_b) {
    int example_index = 0;

    OptionalWrappedDataProvider &optProvider = providers[example_index];
    optProvider.setWrappedProvider(&backingProvider);
    // Args don't really matter (apart from the name) for backing trivial item
    bool value = optProvider.isDefaultOverride(OUTPUT_NAME_1);

    ASSERT_FALSE(value);
}

/**
 * Test when override is set, but before backing provider set.
 */
TEST_F(OptionalWrappedDataProvider_Test, test_is_default_override_1_a) {
    int example_index = 1;

    OptionalWrappedDataProvider &optProvider = providers[example_index];
    //optProvider.setWrappedProvider(&backingProvider);
    // Args don't really matter (apart from the name) for backing trivial item
    bool value = optProvider.isDefaultOverride(OUTPUT_NAME_1);

    ASSERT_FALSE(value);
}

/**
 * Test when override is set, after backing provider set.
 */
TEST_F(OptionalWrappedDataProvider_Test, test_is_default_override_1_b) {
    int example_index = 1;

    OptionalWrappedDataProvider &optProvider = providers[example_index];
    optProvider.setWrappedProvider(&backingProvider);

    // Try first time, when it should override
    bool value = optProvider.isDefaultOverride(OUTPUT_NAME_1);
    ASSERT_TRUE(value);

    // Try again, after getting the (default) value once, meaning it should no longer override
    // Args don't really matter (apart from the name) for backing trivial item
    double output_value = optProvider.get_value(CatchmentAggrDataSelector("",OUTPUT_NAME_1, 0, 10, "m"), data_access::SUM);
    value = optProvider.isDefaultOverride(OUTPUT_NAME_1);
    ASSERT_FALSE(value);
}

/**
 * Test when default provided but no override behavior is set, for single call after provider has been set.
 */
TEST_F(OptionalWrappedDataProvider_Test, test_get_value_0_a) {
    int example_index = 0;

    OptionalWrappedDataProvider &optProvider = providers[example_index];
    optProvider.setWrappedProvider(&backingProvider);
    // Args don't really matter (apart from the name) for backing trivial item
    double value = optProvider.get_value(CatchmentAggrDataSelector("",OUTPUT_NAME_1, 0, 10, "m"), data_access::SUM);

    ASSERT_EQ(value, OUTPUT_VALUE_1);
}

/**
 * Test when default provided but no override behavior is set, for several calls after provider has been set.
 */
TEST_F(OptionalWrappedDataProvider_Test, test_get_value_0_b) {
    int example_index = 0;

    OptionalWrappedDataProvider &optProvider = providers[example_index];
    optProvider.setWrappedProvider(&backingProvider);
    // Args don't really matter (apart from the name) for backing trivial item
    double value;

    for (int i = 0; i < 10; ++i) {
        value = optProvider.get_value(CatchmentAggrDataSelector("",OUTPUT_NAME_1, 0, 10, "m"), data_access::SUM);
        ASSERT_EQ(value, OUTPUT_VALUE_1);
    }
}

/**
 * Test when default provided but no override behavior is set, for several calls without a provider having been set.
 */
TEST_F(OptionalWrappedDataProvider_Test, test_get_value_0_c) {
    int example_index = 0;

    OptionalWrappedDataProvider &optProvider = providers[example_index];
    // Args don't really matter (apart from the name) for backing trivial item
    double value;

    for (int i = 0; i < 10; ++i) {
        value = optProvider.get_value(CatchmentAggrDataSelector("",OUTPUT_NAME_1, 0, 10, "m"), data_access::SUM);
        ASSERT_EQ(value, OUTPUT_DEFAULT_1);
    }
}

/**
 * Test when default is provided and 1 override wait is set.
 */
TEST_F(OptionalWrappedDataProvider_Test, test_get_value_1_a) {
    int example_index = 1;

    OptionalWrappedDataProvider &optProvider = providers[example_index];
    optProvider.setWrappedProvider(&backingProvider);
    // Args don't really matter (apart from the name) for backing trivial item
    double value = optProvider.get_value(CatchmentAggrDataSelector("", OUTPUT_NAME_1, 0, 10, "m"), data_access::SUM);
    ASSERT_EQ(value, OUTPUT_DEFAULT_1);

    // Second time should be the actual value
    value = optProvider.get_value(CatchmentAggrDataSelector("", OUTPUT_NAME_1, 0, 10, "m"), data_access::SUM);
    ASSERT_EQ(value, OUTPUT_VALUE_1);
}

/**
 * Test when default is provided and 2 override waits are set.
 */
TEST_F(OptionalWrappedDataProvider_Test, test_get_value_2_a) {
    int example_index = 2;

    OptionalWrappedDataProvider &optProvider = providers[example_index];
    optProvider.setWrappedProvider(&backingProvider);
    // Args don't really matter (apart from the name) for backing trivial item
    double value;
 
    for (int i = 0; i < 2; ++i) {
        value = optProvider.get_value(CatchmentAggrDataSelector("", OUTPUT_NAME_1, 0, 10, "m"), data_access::SUM);
        ASSERT_EQ(value, OUTPUT_DEFAULT_1);
    }

    // Third time should be the actual value
    value = optProvider.get_value(CatchmentAggrDataSelector("", OUTPUT_NAME_1, 0, 10, "m"), data_access::SUM);
    ASSERT_EQ(value, OUTPUT_VALUE_1);
}

/**
 * Test when default is not provided, for a single call after provider has been set.
 */
TEST_F(OptionalWrappedDataProvider_Test, test_get_value_3_a) {
    int example_index = 3;

    OptionalWrappedDataProvider &optProvider = providers[example_index];
    optProvider.setWrappedProvider(&backingProvider);
    // Args don't really matter (apart from the name) for backing trivial item
    double value = optProvider.get_value(CatchmentAggrDataSelector("", OUTPUT_NAME_1, 0, 10, "m"), data_access::SUM);
    double backing_value = backingProvider.get_value(CatchmentAggrDataSelector("", OUTPUT_NAME_1, 0, 10, "m"), data_access::SUM);

    ASSERT_EQ(value, backing_value);
}

/**
 * Test when default is not provided, for several calls after provider has been set.
 */
TEST_F(OptionalWrappedDataProvider_Test, test_get_value_3_b) {
    int example_index = 3;

    OptionalWrappedDataProvider &optProvider = providers[example_index];
    optProvider.setWrappedProvider(&backingProvider);
    // Args don't really matter (apart from the name) for backing trivial item
    double value, backing_value;

    for (int i = 0; i < 10; ++i) {
        value = optProvider.get_value(CatchmentAggrDataSelector("", OUTPUT_NAME_1, 0, 10, "m"), data_access::SUM);
        backing_value = backingProvider.get_value(CatchmentAggrDataSelector("", OUTPUT_NAME_1, 0, 10, "m"), data_access::SUM);
        ASSERT_EQ(value, backing_value);
    }
}
