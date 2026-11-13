#include <partitioning/Strategy_Nexus_Count.hpp>

#include <stdexcept>
#include <string>

namespace ngen {
namespace partitioning {

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

std::vector<std::size_t> cut_by_nexus_count(const NexusContributors& nexuses, int num_parts)
{
    return cut_evenly(nexuses.nexus_ids.size(), num_parts);
}

} // namespace partitioning
} // namespace ngen
