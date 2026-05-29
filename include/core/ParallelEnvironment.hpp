#ifndef NGEN_DRIVER_PARALLEL_ENVIRONMENT_HPP
#define NGEN_DRIVER_PARALLEL_ENVIRONMENT_HPP

#include <NGenConfig.h>

#if NGEN_WITH_MPI
#include <mpi.h>
#endif

namespace ngen {
namespace driver {

/**
 * RAII owner of the process-wide parallel runtime.
 *
 * On construction this initializes MPI (MPI_Init) and records this process's
 * rank and the total process count; on destruction it finalizes MPI
 * (MPI_Finalize). In a non-MPI build it is a no-op that reports rank 0 of a
 * single process.
 *
 * A program should construct exactly one instance and keep it alive for the
 * duration of any parallel work. Because it owns a process-global resource it
 * is neither copyable nor movable.
 *
 * Note: MPI_Finalize runs from the destructor, so an instance must be allowed
 * to go out of scope normally for finalization to occur. Terminating via
 * exit() (which does not run destructors) bypasses finalization.
 */
class ParallelEnvironment
{
public:
    ParallelEnvironment();
    ~ParallelEnvironment();

    ParallelEnvironment(const ParallelEnvironment&) = delete;
    ParallelEnvironment& operator=(const ParallelEnvironment&) = delete;
    ParallelEnvironment(ParallelEnvironment&&) = delete;
    ParallelEnvironment& operator=(ParallelEnvironment&&) = delete;

    //! This process's rank within the environment (0 in a non-MPI build).
    int rank() const noexcept { return rank_; }

    //! Total number of processes in the environment (1 in a non-MPI build).
    int size() const noexcept { return size_; }

    //! Synchronize all processes in the environment (no-op in a non-MPI build).
    void barrier() const noexcept;

#if NGEN_WITH_MPI
    //! The communicator spanning all processes in this environment.
    MPI_Comm comm() const noexcept { return MPI_COMM_WORLD; }
#endif

private:
    int rank_ = 0;
    int size_ = 1;
};

} // namespace driver
} // namespace ngen

#endif // NGEN_DRIVER_PARALLEL_ENVIRONMENT_HPP
