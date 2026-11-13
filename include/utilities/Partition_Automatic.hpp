#ifndef NGEN_PARTITION_AUTOMATIC_HPP
#define NGEN_PARTITION_AUTOMATIC_HPP

#include <NGenConfig.h>

#if NGEN_WITH_MPI

#include <optional>
#include <string>
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
     * @param partitions On @p root, one part per rank, indexed by rank. Unused on other ranks.
     * @param root The rank holding @p partitions
     * @param comm The communicator whose ranks the parts are for
     * @return This rank's part, with `mpi_world_rank` set to its rank in @p comm
     * @throws std::invalid_argument on @p root, before any communication, if @p partitions does not
     *         have one part per rank
     */
    PartitionData scatter_partitions(const std::vector<PartitionData>& partitions, int root, MPI_Comm comm);

    /**
     * Partition a hydrofabric's catchments automatically across the ranks of @p comm.
     *
     * @p root reads only the divides' connectivity, per hydrofabric::Content::Connectivity, and
     * assigns each nexus, with every catchment that drains to it, to one rank per
     * partition_by_nexus(). Each rank receives its part.
     * Failures are reported on standard error.
     *
     * Collective over @p comm.
     *
     * @param catchment_data_path Path to the hydrofabric's divides. Used only on @p root.
     * @param nexus_data_path Path to the hydrofabric's nexuses. Used only on @p root.
     * @param catchment_subset_ids Divides to partition, or all of them if empty. Used only on @p root.
     * @param root The rank that reads the divides and computes the partitioning
     * @param comm The communicator to partition across, one part per rank
     * @return This rank's part; or, on every rank, no value if @p root could not read the divides
     *         or if any rank's part has no catchments
     */
    std::optional<PartitionData> partition_automatically(const std::string& catchment_data_path,
                                                         const std::string& nexus_data_path,
                                                         const std::vector<std::string>& catchment_subset_ids,
                                                         int root,
                                                         MPI_Comm comm);

} // namespace partitioning
} // namespace ngen

#endif // NGEN_WITH_MPI

#endif // NGEN_PARTITION_AUTOMATIC_HPP
