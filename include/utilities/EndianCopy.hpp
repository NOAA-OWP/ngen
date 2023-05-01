#ifndef NGEN_ENDIANCOPY_H
#define NGEN_ENDIANCOPY_H

#include <vector>

#include <boost/endian.hpp>

namespace utils {

/**
 * @brief
 * Copies bytes from @param{src} to @param{dst},
 * converts the endianness to native,
 * and increments @param{index} by the number of bytes
 * used to store @tparam{S}
 * 
 * @tparam T an integral type
 * @tparam S a primitive type
 * @param src a vector of bytes
 * @param index an integral type tracking the starting position of @param{dst}'s memory
 * @param dst output primitive
 * @param order endianness value (0x01 == Little; 0x00 == Big)
 */
template<typename T, typename S>
void copy_from(std::vector<uint8_t> src, T& index, S& dst, uint8_t order)
{
    std::memcpy(&dst, &src[index], sizeof(S));

    if (order == 0x01) {
        boost::endian::little_to_native_inplace(dst);
    } else {
        boost::endian::big_to_native_inplace(dst);
    }

    index += sizeof(S);
}

}

#endif // NGEN_ENDIANCOPY_H