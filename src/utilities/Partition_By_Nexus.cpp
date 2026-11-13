#include <utilities/Partition_By_Nexus.hpp>

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
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

namespace {

    /**
     * The weight of the block of items `[i, j)`, from the prefix sums of their weights.
     *
     * Every comparison of block weights goes through this one computation, so that its rounding
     * is the same wherever a block is weighed. It is non-decreasing in @p j and non-increasing in
     * @p i, since the prefix sums are non-decreasing and rounding is monotone.
     */
    inline double block_weight(const std::vector<double>& prefix, std::size_t i, std::size_t j)
    {
        return prefix[j] - prefix[i];
    }

    /// The last end `j >= i` of a block from @p i whose weight is within @p bound
    std::size_t farthest_end(const std::vector<double>& prefix, std::size_t i, double bound)
    {
        auto const past = std::partition_point(prefix.begin() + i, prefix.end(),
                                               [&](double p) { return p - prefix[i] <= bound; });
        return (past - prefix.begin()) - 1;
    }

    /**
     * The number of contiguous blocks, none heavier than @p bound, that cover the items whose
     * prefix sums are @p prefix, or `max_parts + 1` if more than @p max_parts are needed or some
     * item alone is heavier than @p bound.
     */
    std::size_t blocks_needed(const std::vector<double>& prefix, double bound, std::size_t max_parts)
    {
        std::size_t const num_items = prefix.size() - 1;
        std::size_t blocks = 0;
        std::size_t i = 0;
        while (i < num_items && blocks <= max_parts) {
            std::size_t const end = farthest_end(prefix, i, bound);
            if (end == i) {
                return max_parts + 1;
            }
            i = end;
            ++blocks;
        }
        return blocks;
    }

    /**
     * The least weight of the heaviest block of any cut into at most @p max_parts blocks.
     *
     * Non-negative doubles are ordered as their bit patterns are, so a binary search over the bit
     * patterns from 0 to the total weight, which always admits a single block, finds the least
     * bound that admits a cut exactly, in at most 64 steps.
     */
    double least_heaviest_block(const std::vector<double>& prefix, std::size_t max_parts)
    {
        std::uint64_t lo = std::bit_cast<std::uint64_t>(0.0);
        std::uint64_t hi = std::bit_cast<std::uint64_t>(block_weight(prefix, 0, prefix.size() - 1));
        while (lo < hi) {
            std::uint64_t const mid = lo + (hi - lo) / 2;
            if (blocks_needed(prefix, std::bit_cast<double>(mid), max_parts) <= max_parts) {
                hi = mid;
            }
            else {
                lo = mid + 1;
            }
        }
        return std::bit_cast<double>(lo);
    }

    /// cut_evenly_by_weight() of items that all have positive weight
    std::vector<std::size_t> cut_positive_weights(const std::vector<double>& weights, std::size_t parts)
    {
        std::size_t const num_items = weights.size();
        std::vector<double> prefix(num_items + 1, 0.0);
        for (std::size_t i = 0; i < num_items; ++i) {
            prefix[i + 1] = prefix[i] + weights[i];
        }
        if (!std::isfinite(prefix[num_items])) {
            throw std::overflow_error("Item weights sum to more than a double holds");
        }

        std::vector<std::size_t> offsets(parts + 1);
        if (num_items <= parts) {
            for (std::size_t p = 0; p <= parts; ++p) {
                offsets[p] = std::min(p, num_items);
            }
            return offsets;
        }

        double const bound = least_heaviest_block(prefix, parts);

        // For each start i, the farthest end of a block within bound, and how many such blocks
        // cover the items from i on. Both are non-increasing as i decreases. Every item alone is
        // within bound, as it lies in some block of a cut within bound.
        std::vector<std::size_t> farthest(num_items + 1, num_items);
        std::vector<std::size_t> needed(num_items + 1, 0);
        std::size_t j = num_items;
        for (std::size_t i = num_items; i-- > 0;) {
            while (block_weight(prefix, i, j) > bound) {
                --j;
            }
            farthest[i] = j;
            needed[i] = 1 + needed[j];
        }

        // Cut each block in turn as near as possible to an even share of the remaining weight,
        // among the ends that leave the remaining items coverable by the remaining blocks within
        // bound, and leave at least one item for each remaining block
        offsets[0] = 0;
        offsets[parts] = num_items;
        std::size_t start = 0;
        for (std::size_t p = 0; p + 1 < parts; ++p) {
            std::size_t const remaining_parts = parts - p;
            std::size_t const last = std::min(farthest[start], num_items - (remaining_parts - 1));
            std::size_t first = start + 1;
            while (needed[first] > remaining_parts - 1) {
                ++first;
            }

            double const share = block_weight(prefix, start, num_items) / remaining_parts;

            // The first end reaching the share, and the one before it, are the nearest to it
            std::size_t const reaching =
                std::partition_point(prefix.begin() + first, prefix.begin() + last + 1,
                                     [&](double p) { return p - prefix[start] < share; }) - prefix.begin();
            std::size_t end;
            if (reaching > last) {
                end = last;
            }
            else if (reaching == first) {
                end = first;
            }
            else {
                double const over = block_weight(prefix, start, reaching) - share;
                double const under = share - block_weight(prefix, start, reaching - 1);
                end = (under < over) ? reaching - 1 : reaching;
            }

            offsets[p + 1] = end;
            start = end;
        }
        return offsets;
    }

} // namespace

std::vector<std::size_t> cut_evenly_by_weight(const std::vector<double>& weights, int num_parts)
{
    if (num_parts < 1) {
        throw std::invalid_argument("Cannot cut into " + std::to_string(num_parts) + " parts");
    }
    std::size_t const parts = static_cast<std::size_t>(num_parts);

    // Cut the items of positive weight; each of zero weight goes with the item before it
    std::vector<std::size_t> positions;
    std::vector<double> positive;
    for (std::size_t i = 0; i < weights.size(); ++i) {
        double const w = weights[i];
        if (!std::isfinite(w) || w < 0.0) {
            throw std::invalid_argument("Item " + std::to_string(i) + " has weight " + std::to_string(w)
                                        + "; weights must be finite and nonnegative");
        }
        if (w > 0.0) {
            positions.push_back(i);
            positive.push_back(w);
        }
    }
    std::vector<std::size_t> const cut = cut_positive_weights(positive, parts);

    // Each block after the first starts at its first item of positive weight, or is empty
    std::vector<std::size_t> offsets(parts + 1, weights.size());
    offsets[0] = 0;
    for (std::size_t p = 1; p < parts; ++p) {
        if (cut[p] < positions.size()) {
            offsets[p] = positions[cut[p]];
        }
    }
    return offsets;
}

std::vector<double> contributing_catchment_counts(const NexusContributors& nexuses)
{
    std::vector<double> counts;
    counts.reserve(nexuses.catchment_ids.size());
    for (const auto& catchments : nexuses.catchment_ids) {
        counts.push_back(static_cast<double>(catchments.size()));
    }
    return counts;
}

std::vector<PartitionData> partition_by_nexus(const NexusContributors& nexuses,
                                              const std::vector<double>& weights,
                                              int num_parts)
{
    if (weights.size() != nexuses.nexus_ids.size()) {
        throw std::invalid_argument("Given " + std::to_string(weights.size()) + " weights for "
                                    + std::to_string(nexuses.nexus_ids.size()) + " nexuses");
    }
    std::vector<std::size_t> const offsets = cut_evenly_by_weight(weights, num_parts);

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

std::vector<PartitionData> partition_by_nexus(const NexusContributors& nexuses, int num_parts)
{
    return partition_by_nexus(nexuses, contributing_catchment_counts(nexuses), num_parts);
}

} // namespace partitioning
} // namespace ngen
