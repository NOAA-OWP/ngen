#include "geopackage.hpp"

#include <numeric>
#include <regex>
#include <Logger.hpp>

std::stringstream read_ss("");

void check_table_name(const std::string& table)
{
    if (boost::algorithm::starts_with(table, "sqlite_")) {
        Logger::logMsgAndThrowError("table `" + table + "` is not queryable");
    }

    std::regex allowed("[^-A-Za-z0-9_ ]+");
    if (std::regex_match(table, allowed)) {
        Logger::logMsgAndThrowError("table `" + table + "` contains invalid characters");
    }
}

std::shared_ptr<geojson::FeatureCollection> ngen::geopackage::read(
    const std::string& gpkg_path,
    const std::string& layer = "",
    const std::vector<std::string>& ids = {}
)
{
    // Check for malicious/invalid layer input
    check_table_name(layer);

    ngen::sqlite::database db{gpkg_path};

    // Check if layer exists
    if (!db.contains(layer)) {
        // Since the layer doesn't exist, we need to output some additional
        // debug information with the error. In this case, we add ALL the tables
        // available in the GPKG, so that if the user sees this error, then it
        // might've been either a typo or a bad data input, and they can correct.
        std::string errmsg = "[" + std::string(sqlite3_errmsg(db.connection())) + "] " +
                             "table " + layer + " does not exist.\n\tTables: ";

        auto errquery = db.query("SELECT name FROM sqlite_master WHERE type='table'");
        errquery.next();
        while(!errquery.done()) {
            errmsg += errquery.get<std::string>(0);
            errmsg += ", ";
            errquery.next();
        }

        Logger::logMsgAndThrowError(errmsg);
    }

    // Introspect if the layer is divides to see which ID field is in use
    std::string id_column = "id";
    if(layer == "divides"){
        try {
            //TODO: A bit primitive. Actually introspect the schema somehow? https://www.sqlite.org/c3ref/funclist.html
            auto query_get_first_row = db.query("SELECT divide_id FROM " + layer + " LIMIT 1");
            id_column = "divide_id";
        }
        catch (const std::exception& e){
            #ifndef NGEN_QUIET
            // output debug info on what is read exactly
            read_ss << "WARN: Using legacy ID column \"id\" in layer " << layer << " is DEPRECATED and may stop working at any time." << std::endl;
            LOG(read_ss.str(), LogLevel::WARNING); read_ss.str("");
            #endif
        }
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
    auto query_get_layer_count = db.query("SELECT COUNT(*) FROM " + layer + joined_ids, ids);
    query_get_layer_count.next();
    const int layer_feature_count = query_get_layer_count.get<int>(0);

    #ifndef NGEN_QUIET
    // output debug info on what is read exactly
    read_ss << "Reading " << layer_feature_count << " features from layer " << layer << " using ID column `"<< id_column << "`";
    if (!ids.empty()) {
        read_ss << " (id subset:";
        for (auto& id : ids) {
            read_ss << " " << id;
        }
        read_ss << ")";
    }
    read_ss << std::endl;
    LOG(read_ss.str(), LogLevel::DEBUG); read_ss.str("");
    #endif

    // Get layer feature metadata (geometry column name + type)
    auto query_get_layer_geom_meta = db.query("SELECT column_name FROM gpkg_geometry_columns WHERE table_name = ?", layer);
    query_get_layer_geom_meta.next();
    const std::string layer_geometry_column = query_get_layer_geom_meta.get<std::string>(0);

    // Get layer
    auto query_get_layer = db.query("SELECT * FROM " + layer + joined_ids, ids);
    query_get_layer.next();

    // build features out of layer query
    std::vector<geojson::Feature> features;
    features.reserve(layer_feature_count);
    while(!query_get_layer.done()) {
        geojson::Feature feature = build_feature(
            query_get_layer,
            id_column,
            layer_geometry_column
        );

        features.push_back(feature);
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
    for (const auto& feature : features) {
        const auto& bbox = feature->get_bounding_box();
        min_x = bbox[0] < min_x ? bbox[0] : min_x;
        min_y = bbox[1] < min_y ? bbox[1] : min_y;
        max_x = bbox[2] > max_x ? bbox[2] : max_x;
        max_y = bbox[3] > max_y ? bbox[3] : max_y;
    }

    auto fc = std::make_shared<geojson::FeatureCollection>(
        std::move(features),
        std::vector<double>({min_x, min_y, max_x, max_y})
    );

    fc->update_ids();

    return fc;
}
