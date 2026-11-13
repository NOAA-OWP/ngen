#include <partitioning/Partition_By_Nexus.hpp>

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

#include <partitioning/Strategy_Contributing_Catchments.hpp>
#include <partitioning/Strategy_Nexus_Count.hpp>

namespace ngen {
namespace partitioning {

namespace details {

    std::vector<CatchmentNexusLink> catchment_nexus_links(const geojson::GeoJSON& catchments)
    {
        std::vector<CatchmentNexusLink> links;
        links.reserve(catchments->get_size());
        for (const auto& feature : *catchments) {
            CatchmentNexusLink link{feature->get_id(), std::nullopt};
            if (feature->has_property("toid")) {
                link.nexus_id = feature->get_property("toid").as_string();
            }
            links.push_back(std::move(link));
        }
        return links;
    }

    NexusContributors group_by_nexus(const std::vector<CatchmentNexusLink>& links)
    {
        NexusContributors grouped;
        std::unordered_map<std::string, std::size_t> index_of;
        for (const auto& link : links) {
            if (!link.nexus_id) {
                grouped.nexus_ids.emplace_back(std::nullopt);
                grouped.catchment_ids.push_back({link.catchment_id});
                continue;
            }
            auto inserted = index_of.emplace(*link.nexus_id, grouped.nexus_ids.size());
            if (inserted.second) {
                grouped.nexus_ids.push_back(link.nexus_id);
                grouped.catchment_ids.emplace_back();
            }
            grouped.catchment_ids[inserted.first->second].push_back(link.catchment_id);
        }
        return grouped;
    }

} // namespace details

namespace {

    /**
     * Assign the groups of @p nexuses to parts by a cut of their sequence into contiguous blocks.
     *
     * @param offsets One more than the number of parts; part `p` gets groups
     *        `[offsets[p], offsets[p + 1])`
     */
    std::vector<PartitionData> assign_blocks(const NexusContributors& nexuses, const std::vector<std::size_t>& offsets)
    {
        std::size_t const num_parts = offsets.size() - 1;
        std::vector<PartitionData> parts(num_parts);
        for (std::size_t p = 0; p < num_parts; ++p) {
            PartitionData& part = parts[p];
            part.mpi_world_rank = static_cast<int>(p);
            for (std::size_t n = offsets[p]; n < offsets[p + 1]; ++n) {
                if (nexuses.nexus_ids[n]) {
                    part.nexus_ids.insert(*nexuses.nexus_ids[n]);
                }
                part.catchment_ids.insert(nexuses.catchment_ids[n].begin(), nexuses.catchment_ids[n].end());
            }
        }
        return parts;
    }

    /**
     * Cut @p nexuses into @p num_parts blocks by @p strategy.
     *
     * @param catchments The catchment features @p nexuses were grouped from, for strategies that
     *        need more than the grouping
     */
    std::vector<std::size_t> cut([[maybe_unused]] const geojson::GeoJSON& catchments,
                                 const NexusContributors& nexuses,
                                 int num_parts,
                                 Strategy strategy)
    {
        switch (strategy) {
            case Strategy::nexus_count:
                return cut_by_nexus_count(nexuses, num_parts);
            case Strategy::contributing_catchments:
                return cut_by_contributing_catchments(nexuses, num_parts);
        }
        throw std::invalid_argument("Unknown partitioning strategy " + std::to_string(static_cast<int>(strategy)));
    }

} // namespace

std::vector<PartitionData> partition_by_nexus(const geojson::GeoJSON& catchments, int num_parts, Strategy strategy)
{
    NexusContributors const nexuses = details::group_by_nexus(details::catchment_nexus_links(catchments));
    return assign_blocks(nexuses, cut(catchments, nexuses, num_parts, strategy));
}

} // namespace partitioning
} // namespace ngen
