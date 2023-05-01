#include "GeoPackage.hpp"

#include <boost/geometry/srs/transformation.hpp>
#include <boost/geometry/srs/epsg.hpp>

#include "WKB.hpp"

namespace bsrs = boost::geometry::srs;

geojson::geometry geopackage::build_geometry(
    const sqlite_iter& row,
    const geojson::FeatureType geom_type,
    const std::string& geom_col
)
{
    const std::vector<uint8_t> geometry_blob = row.get<std::vector<uint8_t>>(geom_col);
    int index = 0;
    if (geometry_blob[0] != 'G' && geometry_blob[1] != 'P') {
        throw std::runtime_error("expected geopackage WKB, but found invalid format instead");
    }
    index += 2;
    
    // skip version
    index++;

    // flags
    const bool is_extended  =  geometry_blob[index] & 0x00100000;
    const bool is_empty     =  geometry_blob[index] & 0x00010000;
    const uint8_t indicator = (geometry_blob[index] & 0x00001110) >> 1;
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

    const std::vector<uint8_t> geometry_data(geometry_blob.begin() + index, geometry_blob.end());
    geojson::geometry geometry = wkb::read_wkb(geometry_data);

    if (srs_id != 4326) {
        return geometry;
    } else {
        geojson::geometry projected;

        // project coordinates from whatever they are to 4326
        boost::geometry::srs::transformation<> tr{
            bsrs::epsg(srs_id),
            bsrs::epsg(4326)
        };

        tr.forward(geometry, projected);

        return projected;
    }
}