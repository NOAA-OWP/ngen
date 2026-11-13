#ifndef NGEN_PARTITIONING_STRATEGY_NEXUS_COUNT_HPP
#define NGEN_PARTITIONING_STRATEGY_NEXUS_COUNT_HPP

#include <cstddef>
#include <vector>

#include "partitioning/Nexus_Contributors.hpp"

namespace ngen {
namespace partitioning {

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
     * Strategy::nexus_count: cut @p nexuses, in the order given, into @p num_parts blocks of
     * nearly equal numbers of nexuses, per cut_evenly().
     *
     * @return `num_parts + 1` offsets; block `p` holds nexuses `[offsets[p], offsets[p + 1])`
     * @throws std::invalid_argument if @p num_parts is less than 1
     */
    std::vector<std::size_t> cut_by_nexus_count(const NexusContributors& nexuses, int num_parts);

} // namespace partitioning
} // namespace ngen

#endif // NGEN_PARTITIONING_STRATEGY_NEXUS_COUNT_HPP
