#include "GeoPackage.hpp"

#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/srs/transformation.hpp>
#include <boost/geometry/srs/epsg.hpp>
#include <boost/variant/detail/apply_visitor_delayed.hpp>

#include "EndianCopy.hpp"
#include "WKB.hpp"

geojson::geometry geopackage::build_geometry(
    const sqlite_iter& row,
    const geojson::FeatureType geom_type,
    const std::string& geom_col
)
{
    const std::vector<uint8_t> geometry_blob = row.get<std::vector<uint8_t>>(geom_col);
    if (geometry_blob[0] != 'G' || geometry_blob[1] != 'P') {
        throw std::runtime_error("expected geopackage WKB, but found invalid format instead");
    }

    int index = 3; // skip version

    // flags
    const bool is_extended  =  geometry_blob[index] & 0x00100000;
    const bool is_empty     =  geometry_blob[index] & 0x00010000;
    const uint8_t indicator = (geometry_blob[index] >> 1) & 0x00000111;
    const uint8_t endian    =  geometry_blob[index] & 0x00000001;
    index++;

    // Read srs_id
    uint32_t srs_id;
    utils::copy_from(geometry_blob, index, srs_id, endian);

    std::vector<double> envelope; // may be unused
    if (indicator > 0 & indicator < 5) {
        // not an empty envelope

        envelope.resize(4); // only 4, not supporting Z or M dims
        utils::copy_from(geometry_blob, index, envelope[0], endian);
        utils::copy_from(geometry_blob, index, envelope[1], endian);
        utils::copy_from(geometry_blob, index, envelope[2], endian);
        utils::copy_from(geometry_blob, index, envelope[3], endian);

        // ensure `index` is at beginning of data
        if (indicator == 2 || indicator == 3) {
            index += 2 * sizeof(double);
        } else if (indicator == 4) {
            index += 4 * sizeof(double);
        }
    }

    if (!is_empty) {
        const std::vector<uint8_t> geometry_data(geometry_blob.begin() + index, geometry_blob.end());
        auto wkb_geometry = wkb::read(geometry_data);
        wkb::wgs84 pvisitor{srs_id};
        geojson::geometry geometry = boost::apply_visitor(pvisitor, wkb_geometry);
        return geometry;
    } else {
        return geojson::geometry{};
    }
}