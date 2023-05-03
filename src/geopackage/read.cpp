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
        throw std::runtime_error("table " + layer + " does not exist");
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

    // Get number of features
    sqlite_iter query_get_layer_count = db.query("SELECT COUNT(*) FROM " + layer);
    query_get_layer_count.next();
    const int layer_feature_count = query_get_layer_count.get<int>(0);

    // Get layer feature metadata (geometry column name + type)
    sqlite_iter query_get_layer_geom_meta = db.query("SELECT column_name FROM gpkg_geometry_columns WHERE table_name = ?", layer);
    query_get_layer_geom_meta.next();
    const std::string layer_geometry_column = query_get_layer_geom_meta.get<std::string>(0);

    // Get layer
    sqlite_iter query_get_layer = db.query("SELECT * FROM " + layer);
    query_get_layer.next();

    std::vector<geojson::Feature> features;
    features.reserve(layer_feature_count);
    while(!query_get_layer.done()) {
        geojson::Feature feature = build_feature(
            query_get_layer,
            layer_geometry_column
        );

        features.emplace_back(feature);
        query_get_layer.next();
    }

    // get layer bounding box
    double min_x = std::numeric_limits<double>::infinity();
    double min_y = std::numeric_limits<double>::infinity();
    double max_x = -std::numeric_limits<double>::infinity();
    double max_y = -std::numeric_limits<double>::infinity();
    for (const auto& feature : features) {
        const auto& bbox = feature->get_bounding_box();
        min_x = bbox[0] < min_x ? bbox[0] : min_x;
        min_y = bbox[1] < min_y ? bbox[1] : min_y;
        max_x = bbox[2] > max_x ? bbox[2] : max_x;
        max_y = bbox[3] > max_y ? bbox[3] : max_y;
    }

    const auto fc = geojson::FeatureCollection(std::move(features), {min_x, min_y, max_x, max_y});
    return std::make_shared<geojson::FeatureCollection>(fc);
}