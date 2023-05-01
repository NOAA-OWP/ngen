#ifndef NGEN_GEOPACKAGE_WKB_READER_H
#define NGEN_GEOPACKAGE_WKB_READER_H

#include <cstring>
#include <sstream>
#include <numeric>
#include <vector>
#include <cmath>

#include <boost/endian.hpp>

#include "JSONGeometry.hpp"

namespace geopackage {
namespace wkb {

using byte_t = uint8_t;
using byte_vector = std::vector<byte_t>;

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
inline void copy_from(byte_vector src, T& index, S& dst, uint8_t order)
{
    std::memcpy(&dst, &src[index], sizeof(S));

    if (order == 0x01) {
        boost::endian::little_to_native_inplace(dst);
    } else {
        boost::endian::big_to_native_inplace(dst);
    }

    index += sizeof(S);
}

/**
 * @brief Recursively read WKB data into known structs
 * 
 * @tparam T geometry struct type
 * @param buffer vector of bytes
 * @param index tracked buffer index
 * @param order endianness
 * @return T parsed WKB in geometry struct
 */
template<typename T>
inline T read_wkb_internal(const byte_vector& buffer, int& index, uint8_t order);

#define READ_WKB_INTERNAL_SIG(output_t) output_t read_wkb_internal<output_t>(const byte_vector& buffer, int& index, uint8_t order)

template<>
inline READ_WKB_INTERNAL_SIG(geojson::coordinate_t)
{
    double x, y;
    copy_from(buffer, index, x, order);
    copy_from(buffer, index, y, order);
    return geojson::coordinate_t{x, y};
};

template<>
inline READ_WKB_INTERNAL_SIG(geojson::linestring_t)
{
    uint32_t count;
    copy_from(buffer, index, count, order);

    geojson::linestring_t linestring;
    linestring.resize(count);

    for (auto& child : linestring) {
        child = read_wkb_internal<geojson::coordinate_t>(buffer, index, order);
    }

    return linestring;
}

template<>
inline READ_WKB_INTERNAL_SIG(geojson::polygon_t)
{
    uint32_t count;
    copy_from(buffer, index, count, order);

    geojson::polygon_t polygon;
    
    if (count > 1) {
        polygon.inners().resize(count - 1);
    }

    auto outer = read_wkb_internal<geojson::linestring_t>(buffer, index, order);
    polygon.outer().reserve(outer.size());
    for (auto& p : outer) {
        polygon.outer().push_back(p);
    }

    for (uint32_t i = 1; i < count; i++) {
        auto inner = read_wkb_internal<geojson::linestring_t>(buffer, index, order);
        polygon.inners().at(i).reserve(inner.size());
        for (auto& p : inner) {
            polygon.inners().at(i).push_back(p);
        }
    }

    return polygon;
}

template<typename T>
inline T read_multi_wkb_internal(const byte_vector& buffer, int& index, uint32_t expected_type) {
    const byte_t new_order = buffer[index];
    index++;

    uint32_t type;
    copy_from(buffer, index, type, new_order);

    if (type != expected_type) {
        throw std::runtime_error("expected type " + std::to_string(expected_type) + ", but found type: " + std::to_string(type));
    }

    return read_wkb_internal<T>(buffer, index, new_order);
};

template<>
inline READ_WKB_INTERNAL_SIG(geojson::multipoint_t)
{
    uint32_t count;
    copy_from(buffer, index, count, order);

    geojson::multipoint_t mp;
    mp.resize(count);

    for (auto& point : mp) {
        point = read_multi_wkb_internal<geojson::coordinate_t>(buffer, index, 1);
    }

    return mp;
}

template<>
inline READ_WKB_INTERNAL_SIG(geojson::multilinestring_t)
{
    uint32_t count;
    copy_from(buffer, index, count, order);

    geojson::multilinestring_t ml;
    ml.resize(count);
    for (auto& line : ml) {
        line = read_multi_wkb_internal<geojson::linestring_t>(buffer, index, 2);
    }

    return ml;
}

template<>
inline READ_WKB_INTERNAL_SIG(geojson::multipolygon_t)
{
    uint32_t count;
    copy_from(buffer, index, count, order);

    geojson::multipolygon_t mpl;
    mpl.resize(count);
    for (auto& polygon : mpl) {
        polygon = read_multi_wkb_internal<geojson::polygon_t>(buffer, index, 3);
    }

    return mpl;
}

#undef READ_WKB_INTERNAL_SIG

/**
 * @brief Read (known) WKB into a specific geometry struct
 * 
 * @tparam T geometry struct type (i.e. @code{wkb::point}, @code{wkb::polygon}, etc.)
 * @param buffer vector of bytes
 * @return T geometry struct containing the parsed WKB values.
 */
template<typename T>
static inline T read_known_wkb(const byte_vector& buffer)
{
    if (buffer.size() < 5) {
        
        throw std::runtime_error(
            "buffer reached end before encountering WKB\n\tdebug: [" +
            std::string(buffer.begin(), buffer.end()) + "]"
        );
    }

    int index = 0;
    const byte_t order = buffer[index];
    index++;

    uint32_t type;
    copy_from(buffer, index, type, order);

    return read_wkb_internal<T>(buffer, index, order);
};

/**
 * @brief Read WKB into a variant geometry struct
 * 
 * @param buffer vector of bytes
 * @return wkb::geometry geometry struct containing the parsed WKB values.
 */
static inline geojson::geometry read_wkb(const byte_vector&buffer)
{
    if (buffer.size() < 5) {
        throw std::runtime_error("buffer reached end before encountering WKB");
    }

    int index = 0;
    const byte_t order = buffer[index];
    index++;

    uint32_t type;
    copy_from(buffer, index, type, order);

    geojson::geometry g = geojson::coordinate_t{std::nan("0"), std::nan("0")};
    switch(type) {
        case 1:  g = read_wkb_internal<geojson::coordinate_t>(buffer, index, order); break;
        case 2:  g = read_wkb_internal<geojson::linestring_t>(buffer, index, order); break;
        case 3:  g = read_wkb_internal<geojson::polygon_t>(buffer, index, order); break;
        case 4:  g = read_wkb_internal<geojson::multipoint_t>(buffer, index, order); break;
        case 5:  g = read_wkb_internal<geojson::multilinestring_t>(buffer, index, order); break;
        case 6:  g = read_wkb_internal<geojson::multipolygon_t>(buffer, index, order); break;
    }

    return g;
}

} // namespace wkb
} // namespace geopackage

#endif // NGEN_GEOPACKAGE_WKB_READER_H
