#include "GeoPackageReader.hpp"
#include "geopackage.hpp"

#include <iostream>
#include <limits>
#include <regex>
#include <stdexcept>

#include <boost/algorithm/string/predicate.hpp>

namespace ngen {
namespace geopackage {

/**
 * Reject table names that are unsafe to interpolate into a query.
 *
 * Layer names reach the query text directly (SQLite cannot bind an
 * identifier), so anything but a plain table name is refused up front.
 *
 * @param[in] table Table name to validate
 * @throws std::runtime_error if the name is a SQLite internal table or
 *         contains characters outside the allowed set
 */
static void check_table_name(const std::string& table)
{
    if (boost::algorithm::starts_with(table, "sqlite_")) {
        throw std::runtime_error("table `" + table + "` is not queryable");
    }

    std::regex allowed("[^-A-Za-z0-9_ ]+");
    if (std::regex_match(table, allowed)) {
        throw std::runtime_error("table `" + table + "` contains invalid characters");
    }
}

GeoPackageReader::GeoPackageReader(sqlite::database db)
  : db_(std::move(db))
{}

const sqlite::database& GeoPackageReader::db() const noexcept
{
    return db_;
}

std::shared_ptr<geojson::FeatureCollection> GeoPackageReader::read(
    const std::string& layer,
    const std::vector<std::string>& ids,
    const std::string& id_column
) const
{
    // Check for malicious/invalid layer input
    check_table_name(layer);

    // Check if layer exists
    if (!db_.contains(layer)) {
        // Since the layer doesn't exist, we need to output some additional
        // debug information with the error. In this case, we add ALL the tables
        // available in the GPKG, so that if the user sees this error, then it
        // might've been either a typo or a bad data input, and they can correct.
        std::string errmsg = "[" + std::string(sqlite3_errmsg(db_.connection())) + "] " +
                             "table " + layer + " does not exist.\n\tTables: ";

        sqlite::database::iterator errquery = db_.query("SELECT name FROM sqlite_master WHERE type='table'");
        errquery.next();
        while(!errquery.done()) {
            errmsg += errquery.get<std::string>(0);
            errmsg += ", ";
            errquery.next();
        }

        throw std::runtime_error(errmsg);
    }

    // Layer exists, getting statement for it
    //
    // this creates a string in the form:
    //     WHERE id IN (?, ?, ?, ...)
    // so that it can be bound by SQLite.
    // This is safer than trying to concatenate
    // the IDs together.
    std::string joined_ids = "";
    if (!ids.empty()) {
        joined_ids = " WHERE "+id_column+" IN (?";
        for (size_t i = 1; i < ids.size(); i++) {
            joined_ids += ", ?";
        }
        joined_ids += ")";
    }

    // Get number of features
    sqlite::database::iterator query_get_layer_count =
        db_.query("SELECT COUNT(*) FROM " + layer + joined_ids, ids);
    query_get_layer_count.next();
    const int layer_feature_count = query_get_layer_count.get<int>(0);

    #ifndef NGEN_QUIET
    // output debug info on what is read exactly
    std::cout << "Reading " << layer_feature_count << " features from layer " << layer << " using ID column `"
              << id_column << "`";
    if (!ids.empty()) {
        std::cout << " (id subset:";
        for (const std::string& id : ids) {
            std::cout << " " << id;
        }
        std::cout << ")";
    }
    std::cout << std::endl;
    #endif

    // Get layer feature metadata (geometry column name + type)
    sqlite::database::iterator query_get_layer_geom_meta = db_.query(
        "SELECT column_name FROM gpkg_geometry_columns WHERE table_name = ?", layer
    );
    query_get_layer_geom_meta.next();
    const std::string layer_geometry_column = query_get_layer_geom_meta.get<std::string>(0);

    // Get layer
    sqlite::database::iterator query_get_layer = db_.query("SELECT * FROM " + layer + joined_ids, ids);
    query_get_layer.next();

    // build features out of layer query
    std::vector<geojson::Feature> features;
    features.reserve(layer_feature_count);
    while(!query_get_layer.done()) {
        std::string id = query_get_layer.get<std::string>(id_column);
        geojson::PropertyMap properties = build_properties(query_get_layer, layer_geometry_column);

        features.push_back(build_feature(
            query_get_layer,
            id,
            layer_geometry_column,
            std::move(properties)
        ));
        query_get_layer.next();
    }

    // get layer bounding box from features
    //
    // GeoPackage contains a bounding box in the SQLite DB,
    // however, it is in the SRS of the GPKG. By creating
    // the bbox after the features are built, the projection
    // is already done. This also should be fairly cheap to do.
    double min_x = std::numeric_limits<double>::infinity();
    double min_y = std::numeric_limits<double>::infinity();
    double max_x = -std::numeric_limits<double>::infinity();
    double max_y = -std::numeric_limits<double>::infinity();
    for (const geojson::Feature& feature : features) {
        const std::vector<double>& bbox = feature->get_bounding_box();
        min_x = bbox[0] < min_x ? bbox[0] : min_x;
        min_y = bbox[1] < min_y ? bbox[1] : min_y;
        max_x = bbox[2] > max_x ? bbox[2] : max_x;
        max_y = bbox[3] > max_y ? bbox[3] : max_y;
    }

    std::shared_ptr<geojson::FeatureCollection> fc = std::make_shared<geojson::FeatureCollection>(
        std::move(features),
        std::vector<double>({min_x, min_y, max_x, max_y})
    );

    fc->update_ids();

    return fc;
}

GeoPackageReader make_reader(const std::string& gpkg_path)
{
    return GeoPackageReader{sqlite::database{gpkg_path}};
}

} // namespace geopackage
} // namespace ngen
