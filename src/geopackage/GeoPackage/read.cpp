#include "GeoPackage.hpp"

std::shared_ptr<geojson::FeatureCollection> geopackage::read(
    const std::string& gpkg_path,
    const std::string& layer = "",
    const std::vector<std::string>& ids = {}
)
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