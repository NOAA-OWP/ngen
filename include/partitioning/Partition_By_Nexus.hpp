#ifndef NGEN_PARTITIONING_PARTITION_BY_NEXUS_HPP
#define NGEN_PARTITIONING_PARTITION_BY_NEXUS_HPP

#include <optional>
#include <string>
#include <vector>

#include <FeatureBuilder.hpp>

#include "core/Partition_Data.hpp"
#include "partitioning/Nexus_Contributors.hpp"

/**
 * Partitioning of a hydrofabric by nexus, which is how automatic partitioning assigns catchments to
 * ranks.
 *
 * Every nexus is assigned to exactly one part, and every catchment is assigned to the part of the
 * nexus it drains to. A nexus thus receives all of its contributions from catchments in its own
 * part, so no part needs to communicate with another.
 *
 * Nexuses are cut into contiguous blocks, in the order given, by one of several strategies. Each
 * strategy is implemented in a Strategy_*.hpp of its own.
 *
 * Nothing here uses MPI; Partition_Automatic.hpp distributes the result across ranks.
 */
namespace ngen {
namespace partitioning {

    /** How partition_by_nexus() balances the parts. */
    enum class Strategy
    {
        /// Nearly equal numbers of nexuses in each part; see cut_by_nexus_count()
        nexus_count,
        /// Nearly equal numbers of catchments in each part; see cut_by_contributing_catchments()
        contributing_catchments,
    };

    /// The strategy used when none is given
    inline constexpr Strategy default_strategy = Strategy::contributing_catchments;

    /**
     * Assign each nexus that @p catchments drain to, and the catchments draining to it, to one of
     * @p num_parts parts.
     *
     * Nexuses are taken in order of first appearance among @p catchments, per
     * details::group_by_nexus().
     *
     * @param catchments Catchment (divide) features, with the downstream nexus under `toid`, which
     *        are also given to the strategy. The strategies so far use only each feature's ID
     *        and `toid`, so features read with hydrofabric::Content::Connectivity suffice; a
     *        strategy that needs more must have its callers read more.
     * @param strategy How the nexuses are cut into parts
     * @return One PartitionData per part, with `mpi_world_rank` set to the part's index. A
     *         catchment that drains to no nexus is assigned as a group of its own.
     * @throws std::invalid_argument if @p num_parts is less than 1 or @p strategy is not a Strategy
     */
    std::vector<PartitionData> partition_by_nexus(const geojson::GeoJSON& catchments,
                                                  int num_parts,
                                                  Strategy strategy = default_strategy);

    /// Steps of partition_by_nexus(), declared here to be tested
    namespace details {

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

        /**
         * Group @p links by nexus.
         *
         * Nexuses keep the order in which they first appear in @p links, so that a hydrofabric whose
         * features are stored in network order yields parts made of neighboring nexuses. Each
         * catchment that drains to no nexus becomes a group of its own, in its place in that order.
         */
        NexusContributors group_by_nexus(const std::vector<CatchmentNexusLink>& links);

    } // namespace details

} // namespace partitioning
} // namespace ngen

#endif // NGEN_PARTITIONING_PARTITION_BY_NEXUS_HPP
