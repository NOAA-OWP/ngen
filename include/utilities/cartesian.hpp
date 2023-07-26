#ifndef NGEN_UTILITIES_CARTESIAN_HPP
#define NGEN_UTILITIES_CARTESIAN_HPP

#include <vector>

#include "span.hpp"

namespace ngen {

/**
 * @brief 
 * 
 * @param shape 
 * @param index 
 * @param dimension 
 * @param output 
 */
inline void cartesian_indices(
    boost::span<const std::size_t>         shape,
    std::vector<std::size_t>&              index,
    std::size_t                            dimension,
    std::vector<std::vector<std::size_t>>& output
)
{
    if (dimension == shape.size()) {
        output.push_back(index);
        return;
    }

    const auto& size = shape[dimension];
    for (std::size_t i = 0; i < size; i++) {
        index[dimension] = i;
        cartesian_indices(shape, index, dimension + 1, output);
    }
}

}

#endif // NGEN_UTILITIES_CARTESIAN_HPP
