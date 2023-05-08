#include "GeoPackage.hpp"

#include <numeric>

std::shared_ptr<geojson::FeatureCollection> geopackage::read(
    const std::string& gpkg_path,
    const std::string& layer = "",
    const std::vector<std::string>& ids = {}
)
{
    sqlite db(gpkg_path);

    // Check if layer exists
    if (!db.has_table(layer)) {
        std::string errmsg = "[" + std::string(sqlite3_errmsg(db.connection())) + "] " +
                             "table " + layer + " does not exist.\n\tTables: ";

        auto errquery = db.query("SELECT name FROM sqlite_master WHERE type='table'").next();
        while(!errquery.done()) {
            errmsg += errquery.get<std::string>(0);
            errmsg += ", ";
            errquery.next();
        }

        throw std::runtime_error(errmsg);
    }

    // Layer exists, getting statement for it
    std::string joined_ids = "";
    if (!ids.empty()) {
        joined_ids = " WHERE id IN (?";
        for (size_t i = 1; i < ids.size(); i++) {
            joined_ids += ", ?";
        }
        joined_ids += ")";
    }

    // Get number of features
    sqlite_iter query_get_layer_count = db.query("SELECT COUNT(*) FROM " + layer + joined_ids, ids);
    query_get_layer_count.next();
    const int layer_feature_count = query_get_layer_count.get<int>(0);

    #ifndef NGEN_QUIET
    std::cout << "Reading " << layer_feature_count << " features in layer " << layer;
    if (!ids.empty()) {
        std::cout << " (id subset:";
        for (auto& id : ids) {
            std::cout << " " << id;
        }
        std::cout << ")";
    }
    std::cout << std::endl;
    #endif

    // Get layer feature metadata (geometry column name + type)
    sqlite_iter query_get_layer_geom_meta = db.query("SELECT column_name FROM gpkg_geometry_columns WHERE table_name = ?", layer);
    query_get_layer_geom_meta.next();
    const std::string layer_geometry_column = query_get_layer_geom_meta.get<std::string>(0);

    // Get layer
    sqlite_iter query_get_layer = db.query("SELECT * FROM " + layer + joined_ids, ids);
    query_get_layer.next();

    std::vector<geojson::Feature> features;
    features.reserve(layer_feature_count);
    while(!query_get_layer.done()) {
        geojson::Feature feature = build_feature(
            query_get_layer,
            layer_geometry_column
        );

        features.push_back(feature);
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