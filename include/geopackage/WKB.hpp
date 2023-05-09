#ifndef NGEN_GEOPACKAGE_WKB_H
#define NGEN_GEOPACKAGE_WKB_H

#include "EndianCopy.hpp"
#include "JSONGeometry.hpp"
#include <boost/geometry/srs/epsg.hpp>
#include <boost/geometry/srs/transformation.hpp>

namespace bg = boost::geometry;

namespace geopackage {

/**
 * A recursive WKB reader
 */
struct wkb {
    using point_t = bg::model::point<double, 2, bg::cs::cartesian>;
    using linestring_t = bg::model::linestring<point_t>;
    using polygon_t = bg::model::polygon<point_t>;
    using multipoint_t = bg::model::multi_point<point_t>;
    using multilinestring_t = bg::model::multi_linestring<linestring_t>;
    using multipolygon_t = bg::model::multi_polygon<polygon_t>;
    using geometry = boost::variant<
        point_t,
        linestring_t,
        polygon_t,
        multipoint_t,
        multilinestring_t,
        multipolygon_t
    >;

    using byte_t = uint8_t;
    using byte_vector = std::vector<byte_t>;

    /**
     * projection visitor. applied with boost to project from
     * cartesian coordinates to WGS84.
     */
    struct wgs84;

    // prevent instatiation of this struct
    wkb() = delete;

    /**
     * Read WKB from a given buffer
     * @param[in] buffer byte vector buffer
     * @return geometry wkb::geometry struct containing the geometry data from the buffer
     */
    static geometry read(const byte_vector& buffer);

    static bg::srs::dpar::parameters<> get_prj(uint32_t srid);

  private:
    /**
     * Read a WKB point into a cartesian model.
     * @return point_t 
     */
    static point_t read_point(const byte_vector&, int&, uint8_t);

    /**
     * Read a WKB linestring into a cartesian model.
     * @return linestring_t 
     */
    static linestring_t read_linestring(const byte_vector&, int&, uint8_t);

    /**
     * Read a WKB polygon into a cartesian model.
     * @return polygon_t 
     */
    static polygon_t read_polygon(const byte_vector&, int&, uint8_t);

    /**
     * Read a WKB multipoint into a cartesian model.
     * @return multipoint_t 
     */
    static multipoint_t read_multipoint(const byte_vector&, int&, uint8_t);

    /**
     * Read a WKB multilinestring into a cartesian model.
     * @return multilinestring_t 
     */
    static multilinestring_t read_multilinestring(const byte_vector&, int&, uint8_t);

    /**
     * Read a WKB multipolygon into a cartesian model.
     * @return multipolygon_t 
     */
    static multipolygon_t read_multipolygon(const byte_vector&, int&, uint8_t);
};

inline typename wkb::point_t wkb::read_point(const byte_vector& buffer, int& index, uint8_t order)
{
    double x, y;
    utils::copy_from(buffer, index, x, order);
    utils::copy_from(buffer, index, y, order);
    return point_t{x, y};
}

inline typename wkb::linestring_t wkb::read_linestring(const byte_vector& buffer, int& index, uint8_t order)
{
    uint32_t count;
    utils::copy_from(buffer, index, count, order);

    linestring_t linestring;
    linestring.resize(count);
    for (auto& child : linestring) {
        child = read_point(buffer, index, order);
    }

    return linestring;
}

inline typename wkb::polygon_t wkb::read_polygon(const byte_vector& buffer, int& index, uint8_t order)
{
    uint32_t count;
    utils::copy_from(buffer, index, count, order);

    polygon_t polygon;
    
    if (count > 1) {
        // polygons only have 1 outer ring,
        // so any extra vectors are considered to be
        // inner rings.
        polygon.inners().resize(count);
    }

    auto outer = read_linestring(buffer, index, order);
    polygon.outer().reserve(outer.size());
    for (auto& p : outer) {
        polygon.outer().push_back(p);
    }

    for (uint32_t i = 1; i < count; i++) {
        auto inner = read_linestring(buffer, index, order);
        polygon.inners().at(i).reserve(inner.size());
        for (auto& p : inner) {
            polygon.inners().at(i).push_back(p);
        }
    }

    return polygon;
}

inline typename wkb::multipoint_t wkb::read_multipoint(const byte_vector& buffer, int& index, uint8_t order)
{
    uint32_t count;
    utils::copy_from(buffer, index, count, order);

    multipoint_t mp;
    mp.resize(count);

    for (auto& point : mp) {
        const byte_t new_order = buffer[index];
        index++;

        uint32_t type;
        utils::copy_from(buffer, index, type, new_order);

        point = read_point(buffer, index, new_order);
    }

    return mp;
}

inline typename wkb::multilinestring_t wkb::read_multilinestring(const byte_vector& buffer, int& index, uint8_t order)
{
    uint32_t count;
    utils::copy_from(buffer, index, count, order);

    multilinestring_t ml;
    ml.resize(count);
    for (auto& line : ml) {
        const byte_t new_order = buffer[index];
        index++;

        uint32_t type;
        utils::copy_from(buffer, index, type, new_order);

        line = read_linestring(buffer, index, new_order);
    }

    return ml;
}

inline typename wkb::multipolygon_t wkb::read_multipolygon(const byte_vector& buffer, int& index, uint8_t order)
{
    uint32_t count;
    utils::copy_from(buffer, index, count, order);

    multipolygon_t mpl;
    mpl.resize(count);
    for (auto& polygon : mpl) {
        const byte_t new_order = buffer[index];
        index++;

        uint32_t type;
        utils::copy_from(buffer, index, type, new_order);

        polygon = read_polygon(buffer, index, new_order);
    }

    return mpl;
}


inline typename wkb::geometry wkb::read(const byte_vector& buffer)
{
    if (buffer.size() < 5) {
        throw std::runtime_error("buffer reached end before encountering WKB");
    }

    int index = 0;
    const byte_t order = buffer[index];
    index++;

    uint32_t type;
    utils::copy_from(buffer, index, type, order);

    geometry g;
    switch(type) {
        case 1:  g = read_point(buffer, index, order); break;
        case 2:  g = read_linestring(buffer, index, order); break;
        case 3:  g = read_polygon(buffer, index, order); break;
        case 4:  g = read_multipoint(buffer, index, order); break;
        case 5:  g = read_multilinestring(buffer, index, order); break;
        case 6:  g = read_multipolygon(buffer, index, order); break;
        default: g = point_t{std::nan("0"), std::nan("0")}; break;
    }

    return g;
}

/**
 * EPSG 5070 projection definition for use with boost::geometry.
 * 
 * @note this is required because boost 1.72.0 does not
 *       have an EPSG definition for 5070 in boost::srs::epsg.
 */
const auto epsg5070 = bg::srs::dpar::parameters<>(bg::srs::dpar::proj_aea)
                                                 (bg::srs::dpar::ellps_grs80)
                                                 (bg::srs::dpar::towgs84, {0,0,0,0,0,0,0})
                                                 (bg::srs::dpar::lat_0, 23)
                                                 (bg::srs::dpar::lon_0, -96)
                                                 (bg::srs::dpar::lat_1, 29.5)
                                                 (bg::srs::dpar::lat_2, 45.5)
                                                 (bg::srs::dpar::x_0, 0)
                                                 (bg::srs::dpar::y_0, 0);

const auto epsg3857 = bg::srs::dpar::parameters<>(bg::srs::dpar::proj_merc)
                                                 (bg::srs::dpar::units_m)
                                                 (bg::srs::dpar::no_defs)
                                                 (bg::srs::dpar::a, 6378137)
                                                 (bg::srs::dpar::b, 6378137)
                                                 (bg::srs::dpar::lat_ts, 0)
                                                 (bg::srs::dpar::lon_0, 0)
                                                 (bg::srs::dpar::x_0, 0)
                                                 (bg::srs::dpar::y_0, 0)
                                                 (bg::srs::dpar::k, 1);

inline bg::srs::dpar::parameters<> wkb::get_prj(uint32_t srid) {
    switch(srid) {
        case 5070:
            return epsg5070;
        case 3857:
            return epsg3857;
        default:
            return bg::projections::detail::epsg_to_parameters(srid);
    }
}

struct wkb::wgs84 : public boost::static_visitor<geojson::geometry>
{
    wgs84(uint32_t srs, const bg::srs::transformation<>& tr)
        : srs(srs)
        , tr(tr) {};

    geojson::geometry operator()(point_t& g)
    {
        if (this->srs == 4326) {
            return geojson::coordinate_t(g.get<0>(), g.get<1>());
        }

        geojson::coordinate_t h;
        this->tr.forward(g, h);
        return h;
    }

    geojson::geometry operator()(linestring_t& g)
    {
        geojson::linestring_t h;

        if (this->srs == 4326) {
            h.reserve(g.size());
            for (auto&& gg : g) {
                h.emplace_back(
                    std::move(gg.get<0>()),
                    std::move(gg.get<1>())
                );
            }
        } else {
            this->tr.forward(g, h);
        }
        return h;
    }

    geojson::geometry operator()(polygon_t& g)
    {
        geojson::polygon_t h;

        if(this->srs == 4326) {
            h.outer().reserve(g.outer().size());
            for (auto&& gg : g.outer()) {
                h.outer().emplace_back(
                    std::move(gg.get<0>()),
                    std::move(gg.get<1>())
                );
            }

            h.inners().resize(g.inners().size());
            auto&& inner_g = g.inners().begin();
            auto&& inner_h = h.inners().begin();
            for (; inner_g != g.inners().end(); inner_g++, inner_h++) {
                inner_h->reserve(inner_g->size());
                for (auto&& gg : *inner_g) {
                    inner_h->emplace_back(
                        std::move(gg.get<0>()),
                        std::move(gg.get<1>())
                    );
                }
            }
        } else {
            this->tr.forward(g, h);
        }
        return h;
    }

    geojson::geometry operator()(multipoint_t& g)
    {
        geojson::multipoint_t h;

        if (this->srs == 4326) {
            h.reserve(g.size());
            for (auto&& gg : g) {
                h.emplace_back(
                    std::move(gg.get<0>()),
                    std::move(gg.get<1>())
                );
            }
        } else {
            this->tr.forward(g, h);
        }

        return h;
    }

    geojson::geometry operator()(multilinestring_t& g)
    {
        geojson::multilinestring_t h;

        if (this->srs == 4326) {
            h.resize(g.size());
            auto&& line_g = g.begin();
            auto&& line_h = h.begin();
            for(; line_g != g.end(); line_g++, line_h++) {
                *line_h = std::move(
                    boost::get<geojson::linestring_t>(this->operator()(*line_g))
                );
            }
        } else {
            this->tr.forward(g, h);
        }

        return h;
    }

    geojson::geometry operator()(multipolygon_t& g)
    {
        geojson::multipolygon_t h;

        if (this->srs == 4326) {
            h.resize(g.size());
            auto&& polygon_g = g.begin();
            auto&& polygon_h = h.begin();
            for (; polygon_g != g.end(); polygon_g++, polygon_h++) {
                *polygon_h = std::move(
                    boost::get<geojson::polygon_t>(this->operator()(*polygon_g))
                );
            }
        } else {
            this->tr.forward(g, h);
        }

        return h;
    }

    private:
        uint32_t srs;
        const bg::srs::transformation<>& tr;
};

} // namespace geopackage

#endif // NGEN_GEOPACKAGE_WKB_H
