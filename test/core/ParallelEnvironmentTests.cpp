#include <gtest/gtest.h>

#include <ParallelEnvironment.hpp>

//! In a non-MPI build, ParallelEnvironment is a serial no-op: rank 0 of 1.
//!
//! In an MPI build, construction drives MPI_Init/MPI_Finalize (each callable
//! only once per process) and this unit test is not run under mpirun, so it is
//! skipped to avoid perturbing the process-global MPI state shared with other
//! tests in the aggregate test binary.
TEST(ParallelEnvironment_Test, SerialDefaults)
{
#if NGEN_WITH_MPI
    GTEST_SKIP() << "Construction drives MPI_Init/MPI_Finalize once per process; "
                    "exercised by the driver and under mpirun, not in this unit test.";
#else
    ngen::driver::ParallelEnvironment env;
    EXPECT_EQ(env.rank(), 0);
    EXPECT_EQ(env.size(), 1);
    env.barrier(); // no-op in a non-MPI build; must not throw
#endif
}
