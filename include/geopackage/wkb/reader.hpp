#ifndef NGEN_GEOPACKAGE_WKB_READER_H
#define NGEN_GEOPACKAGE_WKB_READER_H

#include <cstring>
#include <sstream>
#include <numeric>
#include <vector>
#include <cmath>

#include <boost/endian.hpp>
#include <boost/variant.hpp>

namespace geopackage {
namespace wkb {

using byte_t = uint8_t;
using byte_vector = std::vector<byte_t>;
struct wkb_point           { double                      x, y;     };
struct wkb_linestring      { std::vector<wkb_point>      points;   };
struct wkb_polygon         { std::vector<wkb_linestring> rings;    };
struct wkb_multipoint      { std::vector<wkb_point>      points;   };
struct wkb_multilinestring { std::vector<wkb_linestring> lines;    };
struct wkb_multipolygon    { std::vector<wkb_polygon>    polygons; };
using wkb_geometry = boost::variant<
    wkb_point,
    wkb_linestring,
    wkb_polygon,
    wkb_multipoint,
    wkb_multilinestring,
    wkb_multipolygon
>;

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

namespace {
template<typename output_t, typename child_t>
inline output_t read_wkb_compound_internal(const byte_vector& buffer, int& index, uint8_t order)
{
    uint32_t count;
    copy_from(buffer, index, count, order);
    std::vector<child_t> children(count);
    for (auto& child : children) {
        child = read_wkb_internal<child_t>(buffer, index, order);
    }
    return output_t{children};
}
} // anonymous namespace

#define READ_WKB_INTERNAL_SIG(output_t) output_t read_wkb_internal<output_t>(const byte_vector& buffer, int& index, uint8_t order)

template<>
inline READ_WKB_INTERNAL_SIG(wkb_point)
{
    double x, y;
    copy_from(buffer, index, x, order);
    copy_from(buffer, index, y, order);
    return wkb_point{x, y};
};

template<>
inline READ_WKB_INTERNAL_SIG(wkb_linestring)
{
    return read_wkb_compound_internal<wkb_linestring, wkb_point>(buffer, index, order);
}

template<>
inline READ_WKB_INTERNAL_SIG(wkb_polygon)
{
    return read_wkb_compound_internal<wkb_polygon, wkb_linestring>(buffer, index, order);
}

template<>
inline READ_WKB_INTERNAL_SIG(wkb_multipoint)
{
    return read_wkb_compound_internal<wkb_multipoint, wkb_point>(buffer, index, order);
}

template<>
inline READ_WKB_INTERNAL_SIG(wkb_multilinestring)
{
    return read_wkb_compound_internal<wkb_multilinestring, wkb_linestring>(buffer, index, order);
}

template<>
inline READ_WKB_INTERNAL_SIG(wkb_multipolygon)
{
    return read_wkb_compound_internal<wkb_multipolygon, wkb_polygon>(buffer, index, order);
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
static inline wkb::wkb_geometry read_wkb(const byte_vector&buffer)
{
    if (buffer.size() < 5) {
        throw std::runtime_error("buffer reached end before encountering WKB");
    }

    int index = 0;
    const byte_t order = buffer[index];
    index++;

    uint32_t type;
    copy_from(buffer, index, type, order);

    wkb_geometry g;
    switch(type) {
        case 1:  g = read_wkb_internal<wkb_point>(buffer, index, order); break;
        case 2:  g = read_wkb_internal<wkb_linestring>(buffer, index, order); break;
        case 3:  g = read_wkb_internal<wkb_polygon>(buffer, index, order); break;
        case 4:  g = read_wkb_internal<wkb_multipoint>(buffer, index, order); break;
        case 5:  g = read_wkb_internal<wkb_multilinestring>(buffer, index, order); break;
        case 6:  g = read_wkb_internal<wkb_multipolygon>(buffer, index, order); break;
        default: g = wkb_point{std::nan("0"), std::nan("0")}; break;
    }

    return g;
}

} // namespace wkb
} // namespace geopackage

#endif // NGEN_GEOPACKAGE_WKB_READER_H