#ifndef NGEN_GEOPACKAGE_H
#define NGEN_GEOPACKAGE_H

#include <boost/geometry/srs/projections/epsg_params.hpp>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/geometry/srs/transformation.hpp>
#include <boost/geometry/srs/epsg.hpp>

#include "FeatureCollection.hpp"

#include "JSONGeometry.hpp"
#include "SQLite.hpp"
#include "features/CollectionFeature.hpp"
#include "wkb/reader.hpp"

namespace bsrs = boost::geometry::srs;

namespace geopackage {

namespace {
inline const geojson::FeatureType feature_type_map(const std::string& g)
{
    if (g == "POINT") return geojson::FeatureType::Point;
    if (g == "LINESTRING") return geojson::FeatureType::LineString;
    if (g == "POLYGON") return geojson::FeatureType::Polygon;
    if (g == "MULTIPOINT") return geojson::FeatureType::MultiPoint;
    if (g == "MULTILINESTRING") return geojson::FeatureType::MultiLineString;
    if (g == "MULTIPOLYGON") return geojson::FeatureType::MultiPolygon;
    return geojson::FeatureType::GeometryCollection;
}
} // anonymous namespace

inline geojson::geometry build_geometry(const sqlite_iter& row, const geojson::FeatureType geom_type, const std::string& geom_col)
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
    wkb::copy_from(geometry_blob, index, srs_id, endian);

    std::vector<double> envelope; // may be unused
    if (indicator > 0 & indicator < 5) {
        // not an empty envelope

        envelope.resize(4); // only 4, not supporting Z or M dims
        wkb::copy_from(geometry_blob, index, envelope[0], endian);
        wkb::copy_from(geometry_blob, index, envelope[1], endian);
        wkb::copy_from(geometry_blob, index, envelope[2], endian);
        wkb::copy_from(geometry_blob, index, envelope[3], endian);

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

inline geojson::PropertyMap build_properties(const sqlite_iter& row, const std::string& geom_col)
{
    geojson::PropertyMap properties;

    std::map<std::string, int> property_types;
    const auto data_cols = row.columns();
    const auto data_types = row.types();
    std::transform(
        data_cols.begin(),
        data_cols.end(),
        data_types.begin(),
        std::inserter(property_types, property_types.end()), [](const std::string& name, int type) {
            return std::make_pair(name, type);
        }
    );

    for (auto& col : property_types) {
        const auto name = col.first;
        const auto type = col.second;
        if (name == geom_col) {
            continue;
        }

        geojson::JSONProperty* property = nullptr;
        switch(type) {
            case SQLITE_INTEGER:
                *property = geojson::JSONProperty(name, row.get<int>(name));
                break;
            case SQLITE_FLOAT:
                *property = geojson::JSONProperty(name, row.get<double>(name));
                break;
            case SQLITE_TEXT:
                *property = geojson::JSONProperty(name, row.get<std::string>(name));
                break;
            default:
                *property = geojson::JSONProperty(name, "null");
                break;
        }

        properties.emplace(col, std::move(*property));
    }

    return properties;
}

inline geojson::Feature build_feature(
  const sqlite_iter& row,
  const std::string& geom_type,
  const std::string& geom_col
)
{
    const auto type                  = feature_type_map(geom_type);
    const auto id                    = row.get<std::string>("id");
    std::vector<double> bounding_box = {0, 0, 0, 0}; // TODO
    geojson::PropertyMap properties  = std::move(build_properties(row, geom_col));
    geojson::geometry geometry       = std::move(build_geometry(row, type, geom_col));

    switch(type) {
        case geojson::FeatureType::Point:
            return std::make_shared<geojson::PointFeature>(geojson::PointFeature(
                boost::get<geojson::coordinate_t>(geometry),
                id,
                properties,
                bounding_box,
                std::vector<geojson::FeatureBase*>(),
                std::vector<geojson::FeatureBase*>(),
                {}
            ));
        case geojson::FeatureType::LineString:
            return std::make_shared<geojson::LineStringFeature>(geojson::LineStringFeature(
                boost::get<geojson::linestring_t>(geometry),
                id,
                properties,
                bounding_box,
                std::vector<geojson::FeatureBase*>(),
                std::vector<geojson::FeatureBase*>(),
                {}
            ));
        case geojson::FeatureType::Polygon:
            return std::make_shared<geojson::PolygonFeature>(geojson::PolygonFeature(
                boost::get<geojson::polygon_t>(geometry),
                id,
                properties,
                bounding_box,
                std::vector<geojson::FeatureBase*>(),
                std::vector<geojson::FeatureBase*>(),
                {}
            ));
        case geojson::FeatureType::MultiPoint:
            return std::make_shared<geojson::MultiPointFeature>(geojson::MultiPointFeature(
                boost::get<geojson::multipoint_t>(geometry),
                id,
                properties,
                bounding_box,
                std::vector<geojson::FeatureBase*>(),
                std::vector<geojson::FeatureBase*>(),
                {}
            ));
        case geojson::FeatureType::MultiLineString:
            return std::make_shared<geojson::MultiLineStringFeature>(geojson::MultiLineStringFeature(
                boost::get<geojson::multilinestring_t>(geometry),
                id,
                properties,
                bounding_box,
                std::vector<geojson::FeatureBase*>(),
                std::vector<geojson::FeatureBase*>(),
                {}
            ));
        case geojson::FeatureType::MultiPolygon:
            return std::make_shared<geojson::MultiPolygonFeature>(geojson::MultiPolygonFeature(
                boost::get<geojson::multipolygon_t>(geometry),
                id,
                properties,
                bounding_box,
                std::vector<geojson::FeatureBase*>(),
                std::vector<geojson::FeatureBase*>(),
                {}
            ));
        default:
            return std::make_shared<geojson::CollectionFeature>(geojson::CollectionFeature(
                std::vector<geojson::geometry>{geometry},
                id,
                properties,
                bounding_box,
                std::vector<geojson::FeatureBase*>(),
                std::vector<geojson::FeatureBase*>(),
                {}
            ));
    }
};

inline std::shared_ptr<geojson::FeatureCollection> read(const std::string& gpkg_path, const std::string& layer = "", const std::vector<std::string>& ids = {})
{
    sqlite db(gpkg_path);
    // Check if layer exists
    if (!db.has_table(layer)) {
        throw std::runtime_error("");
    }

    // Layer exists, getting statement for it
    std::string joined_ids = "";
    if (!ids.empty()) {
        std::accumulate(
            ids.begin(),
            ids.end(),
            joined_ids,
            [](const std::string& origin, const std::string& append) {
                return origin.empty() ? append : origin + "," + append;
            }
        );

        joined_ids = " WHERE id (" + joined_ids + ")";
    }
  
    // Get layer bounding box
    sqlite_iter query_get_layer_bbox = db.query("SELECT min_x, min_y, max_x, max_y FROM gpkg_contents WHERE table_name = ?", layer);
    query_get_layer_bbox.next();
    const double min_x = query_get_layer_bbox.get<double>(0);
    const double min_y = query_get_layer_bbox.get<double>(1);
    const double max_x = query_get_layer_bbox.get<double>(2);
    const double max_y = query_get_layer_bbox.get<double>(3);

    // Get number of features
    sqlite_iter query_get_layer_count = db.query("SELECT COUNT(*) FROM " + layer);
    query_get_layer_count.next();
    const int layer_feature_count = query_get_layer_count.get<int>(0);

    // Get layer feature metadata (geometry column name + type)
    sqlite_iter query_get_layer_geom_meta = db.query("SELECT column_name, geometry_type_name FROM gpkg_geometry_columns WHERE table_name = ?", layer);
    const std::string layer_geometry_column = query_get_layer_geom_meta.get<std::string>(0);
    const std::string layer_geometry_type   = query_get_layer_geom_meta.get<std::string>(1);

    sqlite_iter query_get_layer_data_meta = db.query("SELECT column_name FROM gpkg_data_columns WHERE table_name = ?", layer);
    std::vector<std::string> layer_data_columns;
    query_get_layer_data_meta.next();
    while (!query_get_layer_data_meta.done()) {
        layer_data_columns.push_back(query_get_layer_data_meta.get<std::string>(0));
        query_get_layer_data_meta.next();
    }

    // Get layer
    sqlite_iter query_get_layer = db.query("SELECT * FROM " + layer + joined_ids + " ORDER BY id");
    std::vector<geojson::Feature> features;
    features.reserve(layer_feature_count);
    query_get_layer.next();
    while(!query_get_layer.done()) {
        features.push_back(build_feature(
            query_get_layer,
            layer_geometry_type,
            layer_geometry_column
        ));
    }

    const auto fc = geojson::FeatureCollection(
        std::move(features),
        {min_x, min_y, max_x, max_y}
    );

    return std::make_shared<geojson::FeatureCollection>(fc);
}

} // namespace geopackage
#endif // NGEN_GEOPACKAGE_H