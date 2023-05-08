#include "GeoPackage.hpp"
#include "EndianCopy.hpp"
#include "WKB.hpp"

geojson::geometry geopackage::build_geometry(
    const sqlite_iter& row,
    const std::string& geom_col,
    std::vector<double>& bounding_box
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

    if (indicator > 0 & indicator < 5) {
        // not an empty envelope
        bounding_box.clear();
        bounding_box.resize(4); // only 4, not supporting Z or M dims
        utils::copy_from(geometry_blob, index, bounding_box[0], endian); // min_x
        utils::copy_from(geometry_blob, index, bounding_box[2], endian); // max_x
        utils::copy_from(geometry_blob, index, bounding_box[1], endian); // min_y
        utils::copy_from(geometry_blob, index, bounding_box[3], endian); // max_y

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