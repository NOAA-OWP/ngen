#include <utilities/Partition_By_Nexus.hpp>

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

namespace ngen {
namespace partitioning {

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

std::vector<std::size_t> cut_evenly(std::size_t num_items, int num_parts)
{
    if (num_parts < 1) {
        throw std::invalid_argument("Cannot cut into " + std::to_string(num_parts) + " parts");
    }
    std::size_t const parts = static_cast<std::size_t>(num_parts);
    std::size_t const base = num_items / parts;
    std::size_t const larger = num_items % parts;

    std::vector<std::size_t> offsets(parts + 1);
    offsets[0] = 0;
    for (std::size_t p = 0; p < parts; ++p) {
        offsets[p + 1] = offsets[p] + base + (p < larger ? 1 : 0);
    }
    return offsets;
}

std::vector<PartitionData> partition_by_nexus(const NexusContributors& nexuses, int num_parts)
{
    std::vector<std::size_t> const offsets = cut_evenly(nexuses.nexus_ids.size(), num_parts);

    std::vector<PartitionData> parts(num_parts);
    for (int p = 0; p < num_parts; ++p) {
        PartitionData& part = parts[p];
        part.mpi_world_rank = p;
        for (std::size_t n = offsets[p]; n < offsets[p + 1]; ++n) {
            if (nexuses.nexus_ids[n]) {
                part.nexus_ids.insert(*nexuses.nexus_ids[n]);
            }
            part.catchment_ids.insert(nexuses.catchment_ids[n].begin(), nexuses.catchment_ids[n].end());
        }
    }
    return parts;
}

} // namespace partitioning
} // namespace ngen
