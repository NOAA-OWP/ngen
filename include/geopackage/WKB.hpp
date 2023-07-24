#ifndef NGEN_GEOPACKAGE_WKB_H
#define NGEN_GEOPACKAGE_WKB_H

#include "EndianCopy.hpp"
#include "JSONGeometry.hpp"
#include <boost/geometry/srs/epsg.hpp>
#include <boost/geometry/srs/transformation.hpp>

namespace bg = boost::geometry;

namespace geopackage {

/**
 * A recursive WKB reader.
 *
 * @note This WKB implementation follows a subset of the
 *       OGC Specification found in https://www.ogc.org/standard/sfa/.
 *       This is a strict WKB implementation and does not support
 *       Extended WKB (EWKB) or Tiny WKB (TWKB).
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
    static geometry read(const boost::span<const uint8_t> buffer);

    static bg::srs::dpar::parameters<> get_prj(uint32_t srid);

  private:
    /**
     * Read a WKB point into a cartesian model.
     * @return point_t 
     */
    static point_t read_point(const boost::span<const uint8_t>, int&, uint8_t);

    /**
     * Read a WKB linestring into a cartesian model.
     * @return linestring_t 
     */
    static linestring_t read_linestring(const boost::span<const uint8_t>, int&, uint8_t);

    /**
     * Read a WKB polygon into a cartesian model.
     * @return polygon_t 
     */
    static polygon_t read_polygon(const boost::span<const uint8_t>, int&, uint8_t);

    /**
     * Read a WKB multipoint into a cartesian model.
     * @return multipoint_t 
     */
    static multipoint_t read_multipoint(const boost::span<const uint8_t>, int&, uint8_t);

    /**
     * Read a WKB multilinestring into a cartesian model.
     * @return multilinestring_t 
     */
    static multilinestring_t read_multilinestring(const boost::span<const uint8_t>, int&, uint8_t);

    /**
     * Read a WKB multipolygon into a cartesian model.
     * @return multipolygon_t 
     */
    static multipolygon_t read_multipolygon(const boost::span<const uint8_t>, int&, uint8_t);
};

struct wkb::wgs84 : public boost::static_visitor<geojson::geometry>
{
    wgs84(uint32_t srs, const bg::srs::transformation<>& tr)
        : srs(srs)
        , tr(tr) {};

    geojson::geometry operator()(point_t& g);
    geojson::geometry operator()(linestring_t& g);
    geojson::geometry operator()(polygon_t& g);
    geojson::geometry operator()(multipoint_t& g);
    geojson::geometry operator()(multilinestring_t& g);
    geojson::geometry operator()(multipolygon_t& g);

    private:
        uint32_t srs;
        const bg::srs::transformation<>& tr;
};

} // namespace geopackage

#endif // NGEN_GEOPACKAGE_WKB_H
