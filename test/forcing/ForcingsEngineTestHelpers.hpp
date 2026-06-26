#pragma once

#include <gtest/gtest.h>

#include <NGenConfig.h>
#if NGEN_WITH_MPI
#include <mpi.h>
#endif

#include <utilities/python/InterpreterUtil.hpp>
#include <forcing/ForcingsEngineDataProvider.hpp>

struct ForcingsEngineDataProviderTest
  : public testing::Test
{

    ForcingsEngineDataProviderTest()
    {
        #if NGEN_WITH_MPI
        MPI_Comm_size(MPI_COMM_WORLD, &mpi_.size);
        MPI_Comm_rank(MPI_COMM_WORLD, &mpi_.rank);
        #endif
    }

    static void SetUpTestSuite() {
        ForcingsEngineDataProviderTest::gil_ = utils::ngenPy::InterpreterUtil::getInstance();

        data_access::detail::assert_forcings_engine_requirements();
    }

    static void TearDownTestSuite() {
        data_access::detail::ForcingsEngineStorage::instances.clear();
    }
  
    static std::time_t time_start;
    static std::time_t time_end;

  private:
    struct mpi_info {
      int initialized = 0, finalized = 0, size = 1, rank = 0;

      mpi_info()
      {
          #if NGEN_WITH_MPI
          MPI_Init(nullptr, nullptr);
          #endif
      }

      ~mpi_info()
      {
          #if NGEN_WITH_MPI
          PMPI_Finalize();
          #endif
      }
    };
  
    static mpi_info mpi_;
    static std::shared_ptr<utils::ngenPy::InterpreterUtil> gil_;
};
