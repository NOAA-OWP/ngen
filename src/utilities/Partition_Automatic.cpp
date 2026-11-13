#include <utilities/Partition_Automatic.hpp>

#if NGEN_WITH_MPI

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

#include <utilities/Partition_By_Nexus.hpp>
#include <utilities/parallel_utils.h>
#include <HydrofabricReaderFactory.hpp>

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

std::optional<PartitionData> partition_automatically(const std::string& catchment_data_path,
                                                     const std::string& nexus_data_path,
                                                     const std::vector<std::string>& catchment_subset_ids,
                                                     int root,
                                                     MPI_Comm comm)
{
    int mpi_rank, mpi_num_procs;
    MPI_Comm_rank(comm, &mpi_rank);
    MPI_Comm_size(comm, &mpi_num_procs);

    std::vector<PartitionData> partitions;
    int partitioned = 1;
    if (mpi_rank == root) {
        try {
            std::cout << "Partitioning catchments automatically" << std::endl;
            auto hydrofabric = hydrofabric::make_hydrofabric_reader(catchment_data_path, nexus_data_path,
                                                                    hydrofabric::Content::Connectivity);
            auto const links = catchment_nexus_links(hydrofabric->read_divides(catchment_subset_ids));
            partitions = partition_by_nexus(group_by_nexus(links), mpi_num_procs);
        }
        catch (const std::exception& e) {
            std::cerr << "Unable to partition automatically: " << e.what() << std::endl;
            partitioned = 0;
        }
    }
    MPI_Bcast(&partitioned, 1, MPI_INT, root, comm);
    if (!partitioned) {
        return std::nullopt;
    }

    PartitionData local = scatter_partitions(partitions, root, comm);

    int nonempty = local.catchment_ids.empty() ? 0 : 1;
    if (!nonempty) {
        std::cerr << "Rank " << mpi_rank << " has no catchments after automatic partitioning;"
                  << " use fewer processes" << std::endl;
    }
    int all_nonempty = 0;
    MPI_Allreduce(&nonempty, &all_nonempty, 1, MPI_INT, MPI_LAND, comm);
    if (!all_nonempty) {
        return std::nullopt;
    }
    return local;
}

} // namespace partitioning
} // namespace ngen

#endif // NGEN_WITH_MPI
