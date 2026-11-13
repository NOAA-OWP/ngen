#ifndef NGEN_PARTITION_AUTOMATIC_HPP
#define NGEN_PARTITION_AUTOMATIC_HPP

#include <NGenConfig.h>

#if NGEN_WITH_MPI

#include <vector>

#include <mpi.h>

#include "core/Partition_Data.hpp"

namespace ngen {
namespace partitioning {

    /**
     * Send each rank of @p comm its part of a partitioning computed on @p root.
     *
     * Collective over @p comm.
     *
     * @param partitions On @p root, one part per rank, indexed by rank, each without remote
     *        connections. Unused on other ranks.
     * @param root The rank holding @p partitions
     * @param comm The communicator whose ranks the parts are for
     * @return This rank's part, with `mpi_world_rank` set to its rank in @p comm
     * @throws std::invalid_argument on @p root, before any communication, if @p partitions does not
     *         have one part per rank, or if any part has remote connections
     */
    PartitionData scatter_partitions(const std::vector<PartitionData>& partitions, int root, MPI_Comm comm);

} // namespace partitioning
} // namespace ngen

#endif // NGEN_WITH_MPI

#endif // NGEN_PARTITION_AUTOMATIC_HPP
