#include <gtest/gtest.h>

#include "ForcingsEngineLumpedDataProvider.hpp"

#include "NGenConfig.h"
#if NGEN_WITH_MPI
#include <mpi.h>
#endif

#include "AorcForcing.hpp"
#include "utilities/python/InterpreterUtil.hpp"

#include <boost/range/combine.hpp>

struct mpi_info {
    int initialized = 0;
    int finalized   = 0;
    int size        = 1;
    int rank        = 0;
};

struct ForcingsEngineLumpedDataProviderTest
  : public testing::Test
{
    using provider_type = data_access::ForcingsEngineLumpedDataProvider::ForcingsEngineDataProvider;

    ForcingsEngineLumpedDataProviderTest()
    {
        #if NGEN_WITH_MPI
        MPI_Comm_size(MPI_COMM_WORLD, &mpi_.size);
        MPI_Comm_rank(MPI_COMM_WORLD, &mpi_.rank);
        #endif
    }

    static void SetUpTestSuite();
    static void TearDownTestSuite();

  protected:
    static constexpr const char* config_file = "extern/ngen-forcing/NextGen_Forcings_Engine_BMI/NextGen_Forcings_Engine/config.yml";
    static const forcing_params default_params;

    static std::shared_ptr<utils::ngenPy::InterpreterUtil> gil_;
    static provider_type* provider_;
    static mpi_info mpi_;
};

/* Convenience type alias */
using TestFixture = ForcingsEngineLumpedDataProviderTest;

/* Static member initialization */
const forcing_params TestFixture::default_params = { "", "ForcingsEngine", "2024-01-17 01:00:00", "2024-01-17 06:00:00" };
std::shared_ptr<utils::ngenPy::InterpreterUtil> TestFixture::gil_ = nullptr;
TestFixture::provider_type* TestFixture::provider_ = nullptr;
mpi_info TestFixture::mpi_ = {};

// Initialize MPI if available, get Python GIL, and initialize forcings engine.
void TestFixture::SetUpTestSuite()
{
    #if NGEN_WITH_MPI
    MPI_Init(nullptr, nullptr);
    #endif

    TestFixture::gil_ = utils::ngenPy::InterpreterUtil::getInstance();

    data_access::assert_forcings_engine_requirements();

    // Create a lumped forcings engine instance
    TestFixture::provider_ = data_access::ForcingsEngineLumpedDataProvider::lumped_instance(
        config_file,
        default_params.start_time,
        default_params.end_time
    );
}

// Destroy providers, GIL, and finalize MPI
void TestFixture::TearDownTestSuite()
{
    provider_->finalize_all();
    gil_.reset();

    #if NGEN_WITH_MPI
    PMPI_Finalize();
    #endif
}

// ============================================================================

/**
 * Tests for the flyweight-like design of provider storage by getting
 * a new instance of the forcings engine and verifying that it points
 * to the same address as the static initialized `provider_` member.
 */
TEST_F(ForcingsEngineLumpedDataProviderTest, Storage)
{
    auto* inst_a = data_access::ForcingsEngineLumpedDataProvider::instance(config_file, default_params.start_time, default_params.end_time);
    ASSERT_EQ(inst_a, provider_);

    auto* inst_b = data_access::ForcingsEngineLumpedDataProvider::instance(config_file, default_params.start_time, default_params.end_time);
    ASSERT_EQ(inst_a, inst_b);
}

TEST_F(ForcingsEngineLumpedDataProviderTest, Timing)
{
    EXPECT_EQ(provider_->get_data_start_time(), default_params.simulation_start_t);
    EXPECT_EQ(provider_->get_data_stop_time(), default_params.simulation_end_t);
    EXPECT_EQ(provider_->record_duration(), 3600); // Dependent on config file
    EXPECT_EQ(provider_->get_ts_index_for_time(default_params.simulation_start_t), 0);
    EXPECT_EQ(provider_->get_ts_index_for_time(default_params.simulation_end_t), 5);
}

TEST_F(ForcingsEngineLumpedDataProviderTest, VariableAccess)
{
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

    // Check that each output variable exists in the list of expected variables
    for (const auto& output : outputs) {
        EXPECT_NE(
            std::find(expected_variables.begin(), expected_variables.end(), output),
            expected_variables.end()
        );
    }

    const auto selector = CatchmentAggrDataSelector{
        "cat-1015786",
        "LWDOWN",
        provider_->get_data_start_time(),
        3600,
        "seconds"
    };

    const auto result = provider_->get_values(
        selector,
        data_access::ReSampleMethod::SUM
    );

    ASSERT_GT(result.size(), 0);
    for (const auto& r : result) {
        EXPECT_NEAR(r, 164.45626831054688, 1e6);
    }
}
