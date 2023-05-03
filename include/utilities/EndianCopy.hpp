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
 * @tparam S a primitive type
 * @param src a vector of bytes
 * @param index an integral type tracking the starting position of @param{dst}'s memory
 * @param dst output primitive
 * @param order endianness value (0x01 == Little; 0x00 == Big)
 */
template<typename S>
void copy_from(const std::vector<uint8_t>& src, int& index, S& dst, uint8_t order)
{
    std::memcpy(&dst, &src[index], sizeof(S));

    if (order == 0x01) {
        boost::endian::little_to_native_inplace(dst);
    } else {
        boost::endian::big_to_native_inplace(dst);
    }

    index += sizeof(S);
}

// boost::endian doesn't support using primitive doubles
// see: https://github.com/boostorg/endian/issues/36
template<>
inline void copy_from<double>(const std::vector<uint8_t>& src, int& index, double& dst, uint8_t order)
{
    static_assert(sizeof(uint64_t) == sizeof(double), "sizeof(uint64_t) is not the same as sizeof(double)!");

    uint64_t tmp;

    // copy into uint64_t
    copy_from(src, index, tmp, order); 

    // copy resolved endianness into double
    std::memcpy(&dst, &tmp, sizeof(double));

    // above call to copy_from handles index
    // incrementing, so we don't need to.
}

}

#endif // NGEN_ENDIANCOPY_H