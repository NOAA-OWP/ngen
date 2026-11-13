#ifndef NGEN_PARTITION_BY_NEXUS_HPP
#define NGEN_PARTITION_BY_NEXUS_HPP

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include <FeatureBuilder.hpp>

#include "core/Partition_Data.hpp"

/**
 * Partitioning of a hydrofabric by nexus, for parallel execution with no remote connections. This
 * is how automatic partitioning assigns catchments to ranks.
 *
 * Every nexus is assigned to exactly one part, and every catchment is assigned to the part of the
 * nexus it drains to. A nexus thus receives all of its contributions from catchments in its own
 * part, so no part needs a remote connection to another.
 *
 * Nothing here uses MPI; Partition_Automatic.hpp distributes the result across ranks.
 */
namespace ngen {
namespace partitioning {

    /** A catchment and the nexus it drains to, if any. */
    struct CatchmentNexusLink
    {
        std::string catchment_id;
        std::optional<std::string> nexus_id;
    };

    /**
     * Read the nexus each catchment in @p catchments drains to.
     *
     * Takes catchments as any hydrofabric reader presents them, with the downstream nexus under
     * `toid`. A catchment with no `toid` drains to no nexus.
     *
     * @param catchments Catchment (divide) features
     * @return One link per feature, in collection order
     */
    std::vector<CatchmentNexusLink> catchment_nexus_links(const geojson::GeoJSON& catchments);

    /** Nexuses, each with the catchments that drain to it. */
    struct NexusContributors
    {
        /// Nexus IDs, in order of first appearance among the links they were grouped from. A
        /// group with no nexus ID is one catchment that drains to no nexus.
        std::vector<std::optional<std::string>> nexus_ids;
        /// The catchments draining to `nexus_ids[i]` are `catchment_ids[i]`, in link order
        std::vector<std::vector<std::string>> catchment_ids;
    };

    /**
     * Group @p links by nexus.
     *
     * Nexuses keep the order in which they first appear in @p links, so that a hydrofabric whose
     * features are stored in network order yields parts made of neighboring nexuses. Each catchment
     * that drains to no nexus becomes a group of its own, in its place in that order.
     */
    NexusContributors group_by_nexus(const std::vector<CatchmentNexusLink>& links);

    /**
     * Cut a sequence of @p num_items items into @p num_parts contiguous blocks of nearly equal size.
     *
     * Block sizes differ by at most one, which minimizes the largest block. The first
     * `num_items % num_parts` blocks are the larger ones.
     *
     * @return `num_parts + 1` offsets; block `p` holds items `[offsets[p], offsets[p + 1])`
     * @throws std::invalid_argument if @p num_parts is less than 1
     */
    std::vector<std::size_t> cut_evenly(std::size_t num_items, int num_parts);

    /**
     * Assign each nexus in @p nexuses, and the catchments draining to it, to one of @p num_parts parts.
     *
     * Nexuses are cut evenly by count, in the order given.
     *
     * @return One PartitionData per part, with `mpi_world_rank` set to the part's index and no
     *         remote connections. A group with no nexus ID contributes its catchment only.
     * @throws std::invalid_argument if @p num_parts is less than 1
     */
    std::vector<PartitionData> partition_by_nexus(const NexusContributors& nexuses, int num_parts);

} // namespace partitioning
} // namespace ngen

#endif // NGEN_PARTITION_BY_NEXUS_HPP
