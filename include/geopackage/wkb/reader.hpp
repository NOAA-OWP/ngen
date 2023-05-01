#ifndef NGEN_GEOPACKAGE_WKB_POD_H
#define NGEN_GEOPACKAGE_WKB_POD_H

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

struct point           { double                  x, y;     };
struct linestring      { std::vector<point>      points;   };
struct polygon         { std::vector<linestring> rings;    };
struct multipoint      { std::vector<point>      points;   };
struct multilinestring { std::vector<linestring> lines;    };
struct multipolygon    { std::vector<polygon>    polygons; };

struct geometry {
    using gtype = boost::variant<
        point, linestring, polygon,
        multipoint, multilinestring, multipolygon
    >;

    uint32_t type;
    gtype data;
};

class wkt : public boost::static_visitor<std::string>
{
  public:

    /**
    * @brief Get the WKT form from WKB structs
    * 
    * @tparam T WKB geometry struct type
    * @param g geometry object
    * @return std::string @param{g} in WKT form
    */
    template<typename T>
    std::string operator()(T& g) const
    {
        return this->wkt_type(g) + " " + this->wkt_coords(g);
    }

  private:
    std::string wkt_type(const point&)           const { return "POINT";           }
    std::string wkt_type(const linestring&)      const { return "LINESTRING";      }
    std::string wkt_type(const polygon&)         const { return "POLYGON";         }
    std::string wkt_type(const multipoint&)      const { return "MULTIPOINT";      }
    std::string wkt_type(const multilinestring&) const { return "MULTILINESTRING"; }
    std::string wkt_type(const multipolygon&)    const { return "MULTIPOLYGON";    }

    std::string wkt_coords(const point& g) const
    {
        std::ostringstream out;
        out.precision(3);
        out << std::fixed << g.x << " " << g.y;
        return std::move(out).str();
    } 

    std::string wkt_coords(const linestring& g) const
    {
        return "(" + std::accumulate(
            std::next(g.points.begin()),
            g.points.end(),
            wkt_coords(g.points[0]),
            [this](const std::string& a, const point b) { return a + "," + this->wkt_coords(b); }
        ) + ")";
    }

    std::string wkt_coords(const multipoint& g) const
    {
        return "(" + std::accumulate(
            std::next(g.points.begin()),
            g.points.end(),
            wkt_coords(g.points[0]),
            [this](const std::string& a, const point b) { return a + "," + this->wkt_coords(b); }
        ) + ")";
    }

    std::string wkt_coords(const polygon& g) const
    {
        std::string output;
        for (const auto& gg : g.rings) {
            output += "(" + wkt_coords(gg) + ")";
        }
        return output;
    }

    std::string wkt_coords(const multilinestring& g) const
    {
        std::string output;
        for (const auto& gg : g.lines) {
            output += "(" + wkt_coords(gg) + ")";
        }
        return output;
    }

    std::string wkt_coords(const multipolygon& g) const
    {
        std::string output;
        for (const auto& gg : g.polygons) {
            output += "(" + wkt_coords(gg) + ")";
        }
        return output;
    }
};

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

template<>                                                        
inline linestring read_wkb_internal<linestring>(const byte_vector& buffer, int& index, uint8_t order)
{
    uint32_t count;
    copy_from(buffer, index, count, order);
    std::vector<point> children(count);
    for (auto& child : children) {
        child = read_wkb_internal<point>(buffer, index, order);
    }
    return linestring{children};
}

template<>                                                        
inline polygon read_wkb_internal<polygon>(const byte_vector& buffer, int& index, uint8_t order)
{
    uint32_t count;
    copy_from(buffer, index, count, order);
    std::vector<linestring> children(count);
    for (auto& child : children) {
        child = read_wkb_internal<linestring>(buffer, index, order);
    }
    
    return polygon{children};
}

template<>                                                        
inline multipoint read_wkb_internal<multipoint>(const byte_vector& buffer, int& index, uint8_t order)
{
    uint32_t count;
    copy_from(buffer, index, count, order);
    std::vector<point> children(count);
    for (auto& child : children) {
        child = read_wkb_internal<point>(buffer, index, order);
    }
    return multipoint{children};
}

template<>                                                        
inline multilinestring read_wkb_internal<multilinestring>(const byte_vector& buffer, int& index, uint8_t order)
{
    uint32_t count;
    copy_from(buffer, index, count, order);
    std::vector<linestring> children(count);
    for (auto& child : children) {
        child = read_wkb_internal<linestring>(buffer, index, order);
    }
    return multilinestring{children};
}

template<>                                                        
inline multipolygon read_wkb_internal<multipolygon>(const byte_vector& buffer, int& index, uint8_t order)
{
    uint32_t count;
    copy_from(buffer, index, count, order);
    std::vector<polygon> children(count);
    for (auto& child : children) {
        child = read_wkb_internal<polygon>(buffer, index, order);
    }
    return multipolygon{children};
}

/**
 * @brief Read WKB into a naive geometry struct
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

static inline wkb::geometry read_wkb(const byte_vector&buffer)
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

    wkb::geometry g;
    g.type = type;
    switch(type) {
        case 1:
            g.data = read_wkb_internal<point>(buffer, index, order);
            break;
        case 2:
            g.data = read_wkb_internal<linestring>(buffer, index, order);
            break;
        case 3:
            g.data = read_wkb_internal<polygon>(buffer, index, order);
            break;
        case 4:
            g.data = read_wkb_internal<multipoint>(buffer, index, order);
            break;
        case 5:
            g.data = read_wkb_internal<multilinestring>(buffer, index, order);
            break;
        case 6:
            g.data = read_wkb_internal<multipolygon>(buffer, index, order);
            break;
        default:
            g.data = point{std::nan("0"), std::nan("0")};
            break;
    }

    return std::move(g);
}

} // namespace wkb
} // namespace geopackage

#endif // NGEN_GEOPACKAGE_WKB_POD_H