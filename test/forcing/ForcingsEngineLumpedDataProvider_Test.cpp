#include <gtest/gtest.h>

#ifndef NGEN_LUMPED_CONFIG_PATH
#error "Lumped config file path not defined! Set `-DNGEN_LUMPED_CONFIG_PATH`"
#endif

#include <NGenConfig.h>
#if NGEN_WITH_MPI
#include <mpi.h>
#endif

#include <forcing/ForcingsEngineLumpedDataProvider.hpp>
#include <forcing/AorcForcing.hpp>
#include <utilities/python/InterpreterUtil.hpp>

#include <string>

struct ForcingsEngineLumpedDataProviderTest
  : public testing::Test
{
    using provider_type = data_access::ForcingsEngineLumpedDataProvider;

    ForcingsEngineLumpedDataProviderTest()
    {
        #if NGEN_WITH_MPI
        MPI_Comm_size(MPI_COMM_WORLD, &mpi_.size);
        MPI_Comm_rank(MPI_COMM_WORLD, &mpi_.rank);
        #endif
    }

    static constexpr const char* config_file = NGEN_LUMPED_CONFIG_PATH;
    static const std::time_t time_start;
    static const std::time_t time_end;
    static std::shared_ptr<utils::ngenPy::InterpreterUtil> gil_;
    static std::unique_ptr<provider_type> provider_;

    struct { int initialized = 0, finalized = 0, size = 1, rank = 0; } mpi_{};

    static void SetUpTestSuite();
    static void TearDownTestSuite();
};

using TestFixture = ForcingsEngineLumpedDataProviderTest;

constexpr const char* TestFixture::config_file;
const std::time_t TestFixture::time_start = data_access::detail::parse_time("2023-01-17 01:00:00");
const std::time_t TestFixture::time_end = TestFixture::time_start + 3600 + 3600;

std::shared_ptr<utils::ngenPy::InterpreterUtil> TestFixture::gil_ = nullptr;
std::unique_ptr<TestFixture::provider_type> TestFixture::provider_ = nullptr;

void TestFixture::SetUpTestSuite()
{
    #if NGEN_WITH_MPI
    MPI_Init(nullptr, nullptr);
    #endif

    TestFixture::gil_ = utils::ngenPy::InterpreterUtil::getInstance();

    data_access::detail::assert_forcings_engine_requirements();

    TestFixture::provider_ = std::make_unique<data_access::ForcingsEngineLumpedDataProvider>(
        /*init=*/TestFixture::config_file,
        /*time_begin_seconds=*/TestFixture::time_start,
        /*time_end_seconds=*/TestFixture::time_end,
        /*divide_id=*/"cat-11223"
    );
}

void TestFixture::TearDownTestSuite()
{
    data_access::detail::ForcingsEngineStorage::instances.clear();
    gil_.reset();

    #if NGEN_WITH_MPI
    PMPI_Finalize();
    #endif
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
        /*init=*/TestFixture::config_file,
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
    EXPECT_NEAR(result2[0], 0, 1e-6);
}
