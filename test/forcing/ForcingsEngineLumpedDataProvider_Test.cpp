#include <gtest/gtest.h>

#ifndef NGEN_FORCINGS_LUMPED_CONFIG_PATH
#error "Lumped config file path not defined! Set `-DNGEN_FORCINGS_LUMPED_CONFIG_PATH`. " \
       "Note: you should not be seeing this compile error. Please contact an ngen developer."
#endif

#include "ForcingsEngineTestHelpers.hpp"

#include <forcing/ForcingsEngineLumpedDataProvider.hpp>

#include <string>

struct ForcingsEngineLumpedDataProviderTest
  : public ForcingsEngineDataProviderTest
{
    using provider_type = data_access::ForcingsEngineLumpedDataProvider;

    ForcingsEngineLumpedDataProviderTest()
      : ForcingsEngineDataProviderTest()
    {}

    static std::unique_ptr<provider_type> provider_;

    static void SetUpTestSuite();
};

using TestFixture = ForcingsEngineLumpedDataProviderTest;

std::unique_ptr<TestFixture::provider_type> TestFixture::provider_ = nullptr;

constexpr const char* config_file = NGEN_FORCINGS_LUMPED_CONFIG_PATH;

void TestFixture::SetUpTestSuite()
{
    std::cout << "Initializing (Lumped) ForcingsEngineDataProviderTest" << std::endl;
    ForcingsEngineDataProviderTest::SetUpTestSuite();

    TestFixture::time_start = data_access::detail::parse_time("2023-01-17 01:00:00");
    TestFixture::time_end   = TestFixture::time_start + 3600 + 3600;

    TestFixture::provider_ = std::make_unique<data_access::ForcingsEngineLumpedDataProvider>(
        /*init=*/config_file,
        /*time_begin_seconds=*/TestFixture::time_start,
        /*time_end_seconds=*/TestFixture::time_end,
        /*divide_id=*/"cat-11223"
    );
}

/**
 * Tests for the flyweight-like design of provider storage by getting
 * a new instance of the forcings engine and verifying that it points
 * to the same address as the static initialized `provider_` member,
 * based on matching `init`, and shared over distinct `divide_id`.
 */
TEST_F(ForcingsEngineLumpedDataProviderTest, Storage)
{
    auto new_inst = std::make_unique<data_access::ForcingsEngineLumpedDataProvider>(
        /*init=*/config_file,
        /*time_begin_seconds=*/TestFixture::time_start,
        /*time_end_seconds=*/TestFixture::time_end,
        /*divide_id=*/"cat-11371"
    );

    ASSERT_EQ(new_inst->model(), provider_->model());
}

TEST_F(ForcingsEngineLumpedDataProviderTest, VariableAccess)
{
    ASSERT_EQ(provider_->divide(), 11223UL);
    ASSERT_EQ(provider_->divide_index(), 0);

    constexpr std::array<const char*, 8> expected_variables = {
        "U2D_ELEMENT",
        "V2D_ELEMENT",
        "LWDOWN_ELEMENT",
        "SWDOWN_ELEMENT",
        "T2D_ELEMENT",
        "Q2D_ELEMENT",
        "PSFC_ELEMENT",
        "RAINRATE_ELEMENT"
    };

    const auto outputs = provider_->get_available_variable_names();

    // Check that each expected variable is in the list of available outputs.
    for (const auto& expected : expected_variables) {
        EXPECT_NE(
            std::find(outputs.begin(), outputs.end(), expected),
            outputs.end()
        );
    }

    auto selector = CatchmentAggrDataSelector{"cat-11223", "PSFC", time_start, 3600, "seconds"};
    auto result   = provider_->get_value(selector, data_access::ReSampleMethod::SUM);
    EXPECT_NEAR(result, 99580.52, 1e-2);

    selector = CatchmentAggrDataSelector{"cat-11223", "LWDOWN", time_start + 3600, 3600, "seconds"};
    auto result2 = provider_->get_values(selector, data_access::ReSampleMethod::SUM);
    ASSERT_GT(result2.size(), 0);
    EXPECT_NEAR(result2[0], 309.800018, 1e-6);
}
