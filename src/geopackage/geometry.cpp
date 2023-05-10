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
    
    const auto epsg = wkb::get_prj(srs_id);
    const bg::srs::transformation<> prj{epsg, wkb::get_prj(4326)};
    wkb::wgs84 pvisitor{srs_id, prj};
    
    if (indicator > 0 & indicator < 5) {
        // not an empty envelope

        double min_x = 0, max_x = 0, min_y = 0, max_y = 0;
        utils::copy_from(geometry_blob, index, min_x, endian); // min_x
        utils::copy_from(geometry_blob, index, max_x, endian); // max_x
        utils::copy_from(geometry_blob, index, min_y, endian); // min_y
        utils::copy_from(geometry_blob, index, max_y, endian); // max_y

        // we need to transform the bounding box from its initial SRS
        // to EPSG: 4326 -- so, we construct a temporary WKB linestring_t type
        // which will get projected to a geojson::geometry (aka geojson::linestring_t) type.

        // create a wkb::linestring_t bbox object
        wkb::point_t max{max_x, max_y};
        wkb::point_t min{min_x, min_y};
        geojson::coordinate_t max_prj{};
        geojson::coordinate_t min_prj{};

        // project the raw bounding box
        if (srs_id == 4326) {
            max_prj = geojson::coordinate_t{max.get<0>(), max.get<1>()};
            min_prj = geojson::coordinate_t{min.get<0>(), min.get<1>()};
        } else {
            prj.forward(max, max_prj);
            prj.forward(min, min_prj);
        }

        // assign the projected values to the bounding_box parameter
        bounding_box.clear();
        bounding_box.resize(4); // only 4, not supporting Z or M dims
        bounding_box[0] = min_prj.get<0>(); // min_x
        bounding_box[1] = min_prj.get<1>(); // min_y
        bounding_box[2] = max_prj.get<0>(); // max_x
        bounding_box[3] = max_prj.get<1>(); // max_y

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
        geojson::geometry geometry = boost::apply_visitor(pvisitor, wkb_geometry);
        return geometry;
        return geojson::geometry{};
    } else {
        return geojson::geometry{};
    }
}