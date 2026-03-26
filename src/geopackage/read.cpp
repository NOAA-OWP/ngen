#include "geopackage.hpp"

#include <numeric>
#include <regex>
#include <stdexcept>
#include "ewts_ngen/logger.hpp"

std::stringstream read_ss("");

void check_table_name(const std::string& table)
{
    if (boost::algorithm::starts_with(table, "sqlite_")) {
        std::string msg = "table `" + table + "` is not queryable";
        LOG(LogLevel::FATAL, msg);
        throw std::runtime_error(msg);
    }

    std::regex allowed("[^-A-Za-z0-9_ ]+");
    if (std::regex_match(table, allowed)) {
        std::string msg = "table `" + table + "` contains invalid characters";
        LOG(LogLevel::FATAL, msg);
        throw std::runtime_error(msg);
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
    std::vector<geojson::Feature> features;

    LOG(LogLevel::DEBUG, "Establishing connection to geopackage %s.", gpkg_path.c_str());
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
        LOG(LogLevel::FATAL, errmsg);
        throw std::runtime_error(errmsg);
    }

    std::string id_column;
    std::string feature_query;
    if (layer == "divides") {
        id_column = "div_id";
        feature_query = 
            "SELECT "
                "('cat-' || divides.div_id) AS id, "
                "('nex-' || flowpaths.dn_nex_id) AS toid, "
                "flowpaths.slope AS So, "
                "divides.area_sqkm AS areasqkm, " // faster for later code to rename the field here
                "divides.geom AS geom "
            "FROM divides "
            "LEFT JOIN flowpaths "
                "ON divides.div_id = flowpaths.div_id";
    } else if (layer == "nexus") {
        id_column = "nex_id";
        feature_query = 
            "SELECT "
                "('nex-' || nexus.nex_id) AS id, "
                "CASE "
                    "WHEN flowpaths.div_id IS NULL THEN 'terminal' "
                    "ELSE ('cat-' || flowpaths.div_id) "
                "END AS toid, "
                "CASE "
                    "WHEN flowpaths.slope IS NULL THEN 0.0 "
                    "ELSE flowpaths.slope "
                "END AS So, "
                "nexus.geom AS geom "
            "FROM nexus "
            "LEFT JOIN flowpaths "
                "ON nexus.dn_fp_id = flowpaths.fp_id";
    } else {
        Logger::LogAndThrow("Geopackage read only accepts layers `divides` and `nexus`. The layer entered was " + layer);
    }

    std::string joined_ids = "";
    if (!ids.empty()) {
        std::stringstream filter;
        filter << " WHERE " << layer << '.' << id_column << " IN (";
        for (size_t i = 0; i < ids.size(); ++i) {
            if (i != 0)
                filter << ',';
            auto &filter_id = ids[i];
            size_t sep_index = filter_id.find('-');
            if (sep_index == std::string::npos) {
                sep_index = 0;
            } else {
                sep_index++;
            }
            int id_num = std::atoi(filter_id.c_str() + sep_index);
            if (id_num <= 0)
                Logger::LogAndThrow("Could not convert input " + layer + " ID into a number: " + filter_id);
            filter << id_num;
        }
        filter << ')';
        joined_ids = filter.str();
    }

    // Get number of features
    auto query_get_layer_count = db.query("SELECT COUNT(*) FROM " + layer + joined_ids);
    query_get_layer_count.next();
    const int layer_feature_count = query_get_layer_count.get<int>(0);
    features.reserve(layer_feature_count);
    if (!ids.empty() && ids.size() != layer_feature_count) {
        LOG(LogLevel::WARNING, "The number of input IDs (%d) does not equal the number of features with those IDs in the geopackage (%d) for layer %s.",
            ids.size(), layer_feature_count, layer.c_str());
    }

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

    // Get layer
    LOG(LogLevel::DEBUG, "Reading %d features from layer %s.", layer_feature_count, layer.c_str());
    auto query_get_layer = db.query(feature_query + joined_ids);
    query_get_layer.next();

    // build features out of layer query
    while(!query_get_layer.done()) {
        geojson::Feature feature = build_feature(
            query_get_layer,
            "id",
            "geom"
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
