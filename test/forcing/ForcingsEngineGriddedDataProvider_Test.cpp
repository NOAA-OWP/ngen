#include <gtest/gtest.h>

#include "ForcingsEngineDataProvider.hpp"
#include "ForcingsEngineGriddedDataProvider.hpp"

#include "NGenConfig.h"
#if NGEN_WITH_MPI
#include <mpi.h>
#endif

#include "AorcForcing.hpp"
#include "utilities/python/InterpreterUtil.hpp"

struct mpi_info {
    int initialized = 0;
    int finalized   = 0;
    int size        = 1;
    int rank        = 0;
};

struct ForcingsEngineGriddedDataProviderTest
  : public testing::Test
{
    using provider_type = data_access::ForcingsEngineGriddedDataProvider::ForcingsEngineDataProvider;

    ForcingsEngineGriddedDataProviderTest()
    {
        #if NGEN_WITH_MPI
        MPI_Comm_size(MPI_COMM_WORLD, &mpi_.size);
        MPI_Comm_rank(MPI_COMM_WORLD, &mpi_.rank);
        #endif
    }

    static void SetUpTestSuite();
    static void TearDownTestSuite();

  protected:
    static constexpr const char* config_file = "";

    static const forcing_params default_params;
    static std::shared_ptr<utils::ngenPy::InterpreterUtil> gil_;
    static std::unique_ptr<provider_type> provider_;
    static mpi_info mpi_;
};

using TestFixture = ForcingsEngineGriddedDataProviderTest;
const forcing_params TestFixture::default_params = { "", "ForcingsEngine", "2024-01-17 01:00:00", "2024-01-17 06:00:00" };
std::shared_ptr<utils::ngenPy::InterpreterUtil> TestFixture::gil_ = nullptr;
std::unique_ptr<TestFixture::provider_type> TestFixture::provider_ = nullptr;
mpi_info TestFixture::mpi_ = {};

void TestFixture::SetUpTestSuite()
{
    #if NGEN_WITH_MPI
    MPI_Init(nullptr, nullptr);
    #endif

    TestFixture::gil_ = utils::ngenPy::InterpreterUtil::getInstance();

    data_access::assert_forcings_engine_requirements();

    TestFixture::provider_ = data_access::make_forcings_engine<data_access::ForcingsEngineGriddedDataProvider>(
      config_file,
      default_params.start_time,
      default_params.end_time
    );
}

void TestFixture::TearDownTestSuite()
{
    data_access::detail::forcings_engine_instances.clear();
    gil_.reset();

    #if NGEN_WITH_MPI
    PMPI_Finalize();
    #endif
}

/**
 * Tests for the flyweight-like design of provider storage by getting
 * a new instance of the forcings engine and verifying that it points
 * to the same address as the static initialized `provider_` member.
 */
TEST_F(ForcingsEngineGriddedDataProviderTest, Storage)
{
    auto inst_a = data_access::make_forcings_engine<data_access::ForcingsEngineGriddedDataProvider>(config_file, default_params.start_time, default_params.end_time);
    ASSERT_EQ(inst_a->model(), provider_->model());

    auto inst_b = data_access::make_forcings_engine<data_access::ForcingsEngineGriddedDataProvider>(config_file, default_params.start_time, default_params.end_time);
    ASSERT_EQ(inst_a->model(), inst_b->model());
}

TEST_F(ForcingsEngineGriddedDataProviderTest, VariableAccess)
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

    for (const auto& expected : expected_variables) {
        EXPECT_NE(
            std::find(outputs.begin(), outputs.end(), expected),
            outputs.end()
        );
    }

    // const auto selector = GridDataSelector{
    //     SelectorConfig{provider_->get_data_start_time(), 3600, "LWDOWN", "seconds"},
    //     
    // }
}
