#include <ParallelEnvironment.hpp>

namespace ngen {
namespace driver {

ParallelEnvironment::ParallelEnvironment()
{
#if NGEN_WITH_MPI
    MPI_Init(nullptr, nullptr);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank_);
    MPI_Comm_size(MPI_COMM_WORLD, &size_);
#endif
}

ParallelEnvironment::~ParallelEnvironment()
{
#if NGEN_WITH_MPI
    MPI_Finalize();
#endif
}

void ParallelEnvironment::barrier() const noexcept
{
#if NGEN_WITH_MPI
    MPI_Barrier(MPI_COMM_WORLD);
#endif
}

} // namespace driver
} // namespace ngen
