#include <utilities/Partition_Automatic.hpp>

#if NGEN_WITH_MPI

#include <stdexcept>
#include <string>

#include <utilities/parallel_utils.h>

namespace ngen {
namespace partitioning {

PartitionData scatter_partitions(const std::vector<PartitionData>& partitions, int root, MPI_Comm comm)
{
    int mpi_rank, mpi_num_procs;
    MPI_Comm_rank(comm, &mpi_rank);
    MPI_Comm_size(comm, &mpi_num_procs);

    std::vector<std::vector<std::string>> nexus_ids;
    std::vector<std::vector<std::string>> catchment_ids;
    if (mpi_rank == root) {
        if (partitions.size() != static_cast<std::size_t>(mpi_num_procs)) {
            throw std::invalid_argument("scatter_partitions: given " + std::to_string(partitions.size())
                                        + " parts for " + std::to_string(mpi_num_procs) + " ranks");
        }
        nexus_ids.reserve(partitions.size());
        catchment_ids.reserve(partitions.size());
        for (const auto& part : partitions) {
            if (!part.remote_connections.empty()) {
                throw std::invalid_argument("scatter_partitions: cannot send parts with remote connections");
            }
            nexus_ids.emplace_back(part.nexus_ids.begin(), part.nexus_ids.end());
            catchment_ids.emplace_back(part.catchment_ids.begin(), part.catchment_ids.end());
        }
    }

    std::vector<std::string> local_nexus_ids = parallel::scatter_strings(nexus_ids, root, comm);
    std::vector<std::string> local_catchment_ids = parallel::scatter_strings(catchment_ids, root, comm);

    PartitionData local;
    local.mpi_world_rank = mpi_rank;
    local.nexus_ids.insert(local_nexus_ids.begin(), local_nexus_ids.end());
    local.catchment_ids.insert(local_catchment_ids.begin(), local_catchment_ids.end());
    return local;
}

} // namespace partitioning
} // namespace ngen

#endif // NGEN_WITH_MPI
