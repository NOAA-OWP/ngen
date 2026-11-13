#ifndef NGEN_PARTITIONING_STRATEGY_CONTRIBUTING_CATCHMENTS_HPP
#define NGEN_PARTITIONING_STRATEGY_CONTRIBUTING_CATCHMENTS_HPP

#include <cstddef>
#include <vector>

#include "partitioning/Nexus_Contributors.hpp"

namespace ngen {
namespace partitioning {

    /**
     * Cut a sequence of weighted items into @p num_parts contiguous blocks of nearly equal weight.
     *
     * The weight of the heaviest block is the least that any contiguous cut can achieve. Within
     * that bound, each block in turn is cut as close as possible to an even share of the weight
     * not yet assigned, preferring the heavier of two equally close cuts, so the blocks other than
     * the heaviest are balanced as well. That balance is greedy: a block's share does not
     * anticipate a heavier block later on.
     *
     * Items of positive weight are what is cut. With at least as many of them as blocks, every
     * block holds at least one; with fewer, each of the first blocks holds one, and the rest are
     * empty. Each item of zero weight goes in the block of the item before it, or in the first
     * block if no item comes before it.
     *
     * A block's weight is taken as the difference of the prefix sums of the weights, computed in
     * order, and the heaviest block is least among cuts weighed that way. Integer weights with a
     * sum below 2^53 are summed exactly.
     *
     * With equal weights, block sizes differ by at most one, as from cut_evenly(), though the
     * larger blocks need not come first.
     *
     * @param weights Weight of each item, each finite and nonnegative
     * @return `num_parts + 1` offsets; block `p` holds items `[offsets[p], offsets[p + 1])`
     * @throws std::invalid_argument if @p num_parts is less than 1 or any weight is negative or
     *         not finite
     * @throws std::overflow_error if the weights sum to more than a double holds
     */
    std::vector<std::size_t> cut_evenly_by_weight(const std::vector<double>& weights, int num_parts);

    /** The number of catchments in each group of @p nexuses. */
    std::vector<double> contributing_catchment_counts(const NexusContributors& nexuses);

    /**
     * Strategy::contributing_catchments: cut @p nexuses, in the order given, into @p num_parts
     * blocks of nearly equal numbers of catchments, weighting each nexus by the catchments that
     * drain to it, per cut_evenly_by_weight().
     *
     * @return `num_parts + 1` offsets; block `p` holds nexuses `[offsets[p], offsets[p + 1])`
     * @throws std::invalid_argument if @p num_parts is less than 1
     */
    std::vector<std::size_t> cut_by_contributing_catchments(const NexusContributors& nexuses, int num_parts);

} // namespace partitioning
} // namespace ngen

#endif // NGEN_PARTITIONING_STRATEGY_CONTRIBUTING_CATCHMENTS_HPP
