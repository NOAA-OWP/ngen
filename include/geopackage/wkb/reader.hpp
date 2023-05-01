#ifndef NGEN_GEOPACKAGE_WKB_POD_H
#define NGEN_GEOPACKAGE_WKB_POD_H

#include <boost/endian/conversion.hpp>
#include <cstring>
#include <sstream>
#include <numeric>
#include <vector>

#include <boost/endian.hpp>

namespace geopackage {
namespace wkb {

using byte_t = uint8_t;
using byte_vector = std::vector<byte_t>;

struct point           { double                  x, y;     };
struct linestring      { std::vector<point>      points;   };
struct polygon         { std::vector<linestring> rings;    };
struct multipoint      { std::vector<point>      points;   };
struct multilinestring { std::vector<linestring> lines;    };
struct multipolygon    { std::vector<polygon>    polygons; };

namespace {

inline std::string wkt_type(const point&)           { return "POINT";           }
inline std::string wkt_type(const linestring&)      { return "LINESTRING";      }
inline std::string wkt_type(const polygon&)         { return "POLYGON";         }
inline std::string wkt_type(const multipoint&)      { return "MULTIPOINT";      }
inline std::string wkt_type(const multilinestring&) { return "MULTILINESTRING"; }
inline std::string wkt_type(const multipolygon&)    { return "MULTIPOLYGON";    }

inline std::string wkt_coords(const point& g)
{
    std::ostringstream out;
    out.precision(3);
    out << std::fixed << g.x << " " << g.y;
    return std::move(out).str();
} 

inline std::string wkt_coords(const linestring& g)
{
    return "(" + std::accumulate(
        std::next(g.points.begin()),
        g.points.end(),
        wkt_coords(g.points[0]),
        [](const std::string& a, const point b) { return a + "," + wkt_coords(b); }
    ) + ")";
}

inline std::string wkt_coords(const multipoint& g)
{
    return "(" + std::accumulate(
        std::next(g.points.begin()),
        g.points.end(),
        wkt_coords(g.points[0]),
        [](const std::string& a, const point b) { return a + "," + wkt_coords(b); }
    ) + ")";
}

inline std::string wkt_coords(const polygon& g)
{
    std::string output;
    for (const auto& gg : g.rings) {
        output += "(" + wkt_coords(gg) + ")";
    }
    return output;
}

inline std::string wkt_coords(const multilinestring& g)
{
    std::string output;
    for (const auto& gg : g.lines) {
        output += "(" + wkt_coords(gg) + ")";
    }
    return output;
}

inline std::string wkt_coords(const multipolygon& g)
{
    std::string output;
    for (const auto& gg : g.polygons) {
        output += "(" + wkt_coords(gg) + ")";
    }
    return output;
}

} // anonymous namespace

/**
 * @brief Get the WKT form from WKB structs
 * 
 * @tparam T WKB geometry struct type
 * @param g geometry object
 * @return std::string @param{g} in WKT form
 */
template<typename T>
inline std::string as_wkt(const T& g)
{
    return wkb::wkt_type(g) + " " + wkb::wkt_coords(g);
}

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
static inline T read_wkb_internal(const byte_vector& buffer, int& index, uint8_t order);

template<>
inline point read_wkb_internal<point>(const byte_vector& buffer, int& index, uint8_t order)
{
    double x, y;
    copy_from(buffer, index, x, order);
    copy_from(buffer, index, y, order);
    return point{x, y};
};

#define INTERNAL_WKB_DEF(output_t, child_t)                                                           \
    template<>                                                                                        \
    inline output_t read_wkb_internal<output_t>(                      \
        const byte_vector& buffer,                                    \
        int& index,                                                   \
        uint8_t order                                                 \
    )                                                                 \
    {                                                                 \
        uint32_t count;                                               \
        copy_from(buffer, index, count, order);                       \
        std::vector<child_t> children(count);                         \
        for (auto& child : children) {                                \
            child = read_wkb_internal<child_t>(buffer, index, order); \
        }                                                             \
        return output_t{children};                                    \
    }

INTERNAL_WKB_DEF(linestring, point)

INTERNAL_WKB_DEF(polygon, linestring)

INTERNAL_WKB_DEF(multipoint, point)

INTERNAL_WKB_DEF(multilinestring, linestring)

INTERNAL_WKB_DEF(multipolygon, polygon)

#undef INTERNAL_WKB_DEF

/**
 * @brief Read WKB into a naive geometry struct
 * 
 * @tparam T geometry struct type (i.e. @code{wkb::point}, @code{wkb::polygon}, etc.)
 * @param buffer vector of bytes
 * @return T geometry struct containing the parsed WKB values.
 */
template<typename T>
static inline T read_wkb(const byte_vector& buffer)
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
    copy_from(buffer, index, type, 0x00);

    return read_wkb_internal<T>(buffer, index, order);
};

} // namespace wkb
} // namespace geopackage

#endif // NGEN_GEOPACKAGE_WKB_POD_H