#ifndef NGEN_PARTITIONING_NEXUS_CONTRIBUTORS_HPP
#define NGEN_PARTITIONING_NEXUS_CONTRIBUTORS_HPP

#include <optional>
#include <string>
#include <vector>

namespace ngen {
namespace partitioning {

    /**
     * Nexuses, each with the catchments that drain to it, in the order that a partitioning strategy
     * cuts them into contiguous blocks.
     */
    struct NexusContributors
    {
        /// Nexus IDs, in order of first appearance among the catchments they were grouped from. A
        /// group with no nexus ID is one catchment that drains to no nexus.
        std::vector<std::optional<std::string>> nexus_ids;
        /// The catchments draining to `nexus_ids[i]` are `catchment_ids[i]`, in their order among
        /// the catchments they were grouped from
        std::vector<std::vector<std::string>> catchment_ids;
    };

} // namespace partitioning
} // namespace ngen

#endif // NGEN_PARTITIONING_NEXUS_CONTRIBUTORS_HPP
