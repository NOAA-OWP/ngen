#ifndef NGEN_GEOPACKAGE_WKB_H
#define NGEN_GEOPACKAGE_WKB_H

#include "EndianCopy.hpp"
#include "JSONGeometry.hpp"
#include <boost/geometry/srs/epsg.hpp>
#include <boost/geometry/srs/projections/epsg_params.hpp>
#include <boost/geometry/srs/projections/spar.hpp>
#include <boost/geometry/srs/transformation.hpp>

namespace bg = boost::geometry;

namespace geopackage {

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

    struct wgs84;

    wkb() = delete;
    static geometry read(const byte_vector&);

  private:
    static point_t read_point(const byte_vector&, int&, uint8_t);
    static linestring_t read_linestring(const byte_vector&, int&, uint8_t);
    static polygon_t read_polygon(const byte_vector&, int&, uint8_t);
    static multipoint_t read_multipoint(const byte_vector&, int&, uint8_t);
    static multilinestring_t read_multilinestring(const byte_vector&, int&, uint8_t);
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
        polygon.inners().resize(count - 1);
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


/**
 * @brief Read WKB into a variant geometry struct
 * 
 * @tparam CoordinateSystem boost coordinate system (i.e. boost::geometry::cs::cartesian)
 * @param buffer buffer vector of bytes
 * @return g_geometry_t<CoordinateSystem> Variant geometry struct
 */
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

    geometry g = point_t{std::nan("0"), std::nan("0")};
    switch(type) {
        case 1:  g = read_point(buffer, index, order); break;
        case 2:  g = read_linestring(buffer, index, order); break;
        case 3:  g = read_polygon(buffer, index, order); break;
        case 4:  g = read_multipoint(buffer, index, order); break;
        case 5:  g = read_multilinestring(buffer, index, order); break;
        case 6:  g = read_multipolygon(buffer, index, order); break;
    }
    return g;
}

using namespace bg::srs;
const auto epsg5070 = bg::srs::dpar::parameters<>(
    dpar::proj_aea
)(
    bg::srs::dpar::ellps_grs80
)(
    bg::srs::dpar::towgs84, {0,0,0,0,0,0,0}
)(
    bg::srs::dpar::lat_0, 23
)(
    bg::srs::dpar::lon_0, -96
)(
    bg::srs::dpar::lat_1, 29.5
)(
    bg::srs::dpar::lat_2, 45.5
)(
    bg::srs::dpar::x_0, 0
)(
    bg::srs::dpar::y_0, 0
);

struct wkb::wgs84 : public boost::static_visitor<geojson::geometry>
{
    wgs84(uint32_t srs) : srs(srs)
    {
        if (srs == 5070) {
            this->tr = std::make_unique<bg::srs::transformation<bg::srs::dynamic, bg::srs::dynamic>>(
                bg::srs::transformation<bg::srs::dynamic, bg::srs::dynamic>{
                    epsg5070,
                    bg::srs::dpar::parameters<>(bg::srs::dpar::proj_longlat)(bg::srs::dpar::datum_wgs84)(bg::srs::dpar::no_defs)
                }
            );
        } else {
            this->tr = std::make_unique<bg::srs::transformation<bg::srs::dynamic, bg::srs::dynamic>>(
                bg::srs::transformation<bg::srs::dynamic, bg::srs::dynamic>{
                    bg::srs::epsg(srs),
                    bg::srs::epsg(4326)
                }
            );
        }
    };

    geojson::geometry operator()(point_t& g)
    {
        geojson::coordinate_t h;
        this->tr->forward(g, h);
        return h;
    }

    geojson::geometry operator()(linestring_t& g)
    {
        geojson::linestring_t h;
        this->tr->forward(g, h);
        return h;
    }

    geojson::geometry operator()(polygon_t& g)
    {
        geojson::polygon_t h;
        this->tr->forward(g, h);
        return h;
    }

    geojson::geometry operator()(multipoint_t& g)
    {
        geojson::multipoint_t h;
        this->tr->forward(g, h);
        return h;
    }

    geojson::geometry operator()(multilinestring_t& g)
    {
        geojson::multilinestring_t h;
        this->tr->forward(g, h);
        return h;
    }

    geojson::geometry operator()(multipolygon_t& g)
    {
        geojson::multipolygon_t h;
        this->tr->forward(g, h);
        return h;
    }

    private:
        uint32_t srs;
        std::unique_ptr<bg::srs::transformation<bg::srs::dynamic, bg::srs::dynamic>> tr;
};

} // namespace geopackage

#endif // NGEN_GEOPACKAGE_WKB_H
