#ifndef NGEN_GEOPACKAGE_WKB_VISITOR_WKT_H
#define NGEN_GEOPACKAGE_WKB_VISITOR_WKT_H

#include "reader.hpp"

namespace geopackage {
namespace wkb {

class wkt_visitor : public boost::static_visitor<std::string>
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
        return std::string(this->wkt_type(g)) + " " + this->wkt_coords(g);
    }

  private:
    constexpr const char* wkt_type(const wkb_point&)           const { return "POINT";           }
    constexpr const char* wkt_type(const wkb_linestring&)      const { return "LINESTRING";      }
    constexpr const char* wkt_type(const wkb_polygon&)         const { return "POLYGON";         }
    constexpr const char* wkt_type(const wkb_multipoint&)      const { return "MULTIPOINT";      }
    constexpr const char* wkt_type(const wkb_multilinestring&) const { return "MULTILINESTRING"; }
    constexpr const char* wkt_type(const wkb_multipolygon&)    const { return "MULTIPOLYGON";    }

    std::string wkt_coords(const wkb_point& g) const
    {
        std::ostringstream out;
        out.precision(3);
        out << std::fixed << g.x << " " << g.y;
        return std::move(out).str();
    } 
    
    std::string wkt_coords(const wkb_linestring& g) const
    {
        return "(" + std::accumulate(
            std::next(g.points.begin()),
            g.points.end(),
            wkt_coords(g.points[0]),
            [this](const std::string& a, const wkb_point b) { return a + "," + this->wkt_coords(b); }
        ) + ")";
    }

    std::string wkt_coords(const wkb_multipoint& g) const
    {
        return "(" + std::accumulate(
            std::next(g.points.begin()),
            g.points.end(),
            wkt_coords(g.points[0]),
            [this](const std::string& a, const wkb_point b) { return a + "," + this->wkt_coords(b); }
        ) + ")";
    }

    std::string wkt_coords(const wkb_polygon& g) const
    {
        std::string output;
        for (const auto& gg : g.rings) {
            output += "(" + wkt_coords(gg) + ")";
        }
        return output;
    }

    std::string wkt_coords(const wkb_multilinestring& g) const
    {
        std::string output;
        for (const auto& gg : g.lines) {
            output += "(" + wkt_coords(gg) + ")";
        }
        return output;
    }

    std::string wkt_coords(const wkb_multipolygon& g) const
    {
        std::string output;
        for (const auto& gg : g.polygons) {
            output += "(" + wkt_coords(gg) + ")";
        }
        return output;
    }
};

} // namespace wkb
} // namespace geopackage

#endif // NGEN_GEOPACKAGE_WKB_VISITOR_WKT_H