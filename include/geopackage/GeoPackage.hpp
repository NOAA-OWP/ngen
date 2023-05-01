#ifndef NGEN_GEOPACKAGE_H
#define NGEN_GEOPACKAGE_H

#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>
#include <sqlite3.h>

#include "SQLite.hpp"
#include "FeatureCollection.hpp"

namespace geopackage {

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

inline geojson::geometry build_geometry(const sqlite_iter& row, const geojson::FeatureType geom_type, const std::string& geom_col)
{
    const std::vector<uint8_t> geometry_blob = row.get<std::vector<uint8_t>>(geom_col);

    geojson::geometry geometry;

    switch(geom_type) {
        case geojson::FeatureType::Point:
            break;
        case geojson::FeatureType::LineString:
            break;
        case geojson::FeatureType::Polygon:
            break;
        case geojson::FeatureType::MultiPoint:
            break;
        case geojson::FeatureType::MultiLineString:
            break;
        case geojson::FeatureType::MultiPolygon:
            break;
        case geojson::FeatureType::GeometryCollection:
            break;
        default:
            break;
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
    const auto type = feature_type_map(geom_type);
    const geojson::PropertyMap properties = build_properties(row, geom_col);
    const geojson::geometry geometry = build_geometry(row, type, geom_col);
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