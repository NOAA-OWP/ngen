#include <gtest/gtest.h>

#ifndef NGEN_FORCINGS_GRIDDED_CONFIG_PATH
#error "Gridded config file path not defined! Set `-DNGEN_FORCINGS_GRIDDED_CONFIG_PATH`. " \
       "Note: you should not be seeing this compile error. Please contact an ngen developer."
#endif

#include "ForcingsEngineTestHelpers.hpp"

#include <forcing/ForcingsEngineGriddedDataProvider.hpp>

struct ForcingsEngineGriddedDataProviderTest
  : public ForcingsEngineDataProviderTest
{
    using provider_type = data_access::ForcingsEngineGriddedDataProvider;

    ForcingsEngineGriddedDataProviderTest()
      : ForcingsEngineDataProviderTest()
    {}

    static std::unique_ptr<provider_type> provider_;

    static void SetUpTestSuite();
};

using TestFixture = ForcingsEngineGriddedDataProviderTest;

std::unique_ptr<TestFixture::provider_type> TestFixture::provider_ = nullptr;

constexpr const char* config_file = NGEN_FORCINGS_GRIDDED_CONFIG_PATH;

constexpr auto cat_11223_mask = geojson::box_t{
  /*min_corner=*/{ -71.0554067, 43.1423006 },
  /*max_corner=*/{ -70.9668753, 43.185111 }
};

constexpr auto cat_11410_mask = geojson::box_t{
  /*min_corner=*/{ -71.0577593, 43.1375584 },
  /*max_corner=*/{ -71.0210404, 43.1761626 }
};

void TestFixture::SetUpTestSuite()
{
    std::cout << "Initializing (Gridded) ForcingsEngineDataProviderTest" << std::endl;
    ForcingsEngineDataProviderTest::SetUpTestSuite();

    TestFixture::time_start = data_access::detail::parse_time("2023-01-17 01:00:00");
    TestFixture::time_end   = TestFixture::time_start + 3600 + 3600;

    TestFixture::provider_ = std::make_unique<data_access::ForcingsEngineGriddedDataProvider>(
        /*init=*/config_file,
        /*time_begin_seconds=*/TestFixture::time_start,
        /*time_end_seconds=*/TestFixture::time_end,
        /*mask=*/cat_11223_mask
    );

    #if NGEN_WITH_MPI
    auto comm = MPI_Comm_c2f(MPI_COMM_WORLD);
    provider_->model()->model()->attr("set_value")("bmi_mpi_comm", py::array_t<MPI_Fint>(comm));
    #endif
}

/**
 * Tests for the flyweight-like design of provider storage by getting
 * a new instance of the forcings engine and verifying that it points
 * to the same address as the static initialized `provider_` member,
 * based on matching `init`, and shared over distinct `divide_id`.
 */
TEST_F(ForcingsEngineGriddedDataProviderTest, Storage)
{
    auto new_inst = std::make_unique<data_access::ForcingsEngineGriddedDataProvider>(
        /*init=*/config_file,
        /*time_begin_seconds=*/TestFixture::time_start,
        /*time_end_seconds=*/TestFixture::time_end,
        /*mask=*/cat_11410_mask
    );

    ASSERT_EQ(new_inst->model(), provider_->model());
}

TEST_F(ForcingsEngineGriddedDataProviderTest, VariableAccess)
{
    ASSERT_TRUE(boost::geometry::equals(provider_->mask().extent, cat_11223_mask));

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

    auto selector = GriddedDataSelector{"PSFC", time_start, 3600, "seconds"};
    auto result   = provider_->get_values(selector, data_access::ReSampleMethod::SUM);
    EXPECT_EQ(result.size(), provider_->mask().size());
    
    bool at_least_one = false;
    for (auto v : result) {
        if (v > 0) {
            at_least_one = true;
            break;
        }
    }
    EXPECT_TRUE(at_least_one) << "All values of `result` are 0";

    // EXPECT_NEAR(result, 99580.52, 1e-2);
    // selector = CatchmentAggrDataSelector{"cat-11223", "LWDOWN", time_start + 3600, 3600, "seconds"};
    // auto result2 = provider_->get_values(selector, data_access::ReSampleMethod::SUM);
    // ASSERT_GT(result2.size(), 0);
    // EXPECT_NEAR(result2[0], 0, 1e-6);
}
