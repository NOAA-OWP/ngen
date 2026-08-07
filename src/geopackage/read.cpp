#include "geopackage.hpp"
#include "HydrofabricVersion.hpp"
#include "JSONProperty.hpp"

#include <numeric>
#include <regex>
#include <unordered_map>

void check_table_name(const std::string& table)
{
    if (boost::algorithm::starts_with(table, "sqlite_")) {
        throw std::runtime_error("table `" + table + "` is not queryable");
    }

    std::regex allowed("[^-A-Za-z0-9_ ]+");
    if (std::regex_match(table, allowed)) {
        throw std::runtime_error("table `" + table + "` contains invalid characters");
    }
}

// Hydrofabric tables touched: nexus and divides always; flowpaths only for
// v4.0beta1 divides->toid synthesis (never for v4.0). Auxiliary tables
// (network, flowlines, pois, lakes, attribute tables, etc.) are left alone.
std::shared_ptr<geojson::FeatureCollection> ngen::geopackage::read(
    const std::string& gpkg_path,
    const std::string& layer = "",
    const std::vector<std::string>& ids = {}
)
{
    // Check for malicious/invalid layer input
    check_table_name(layer);

    ngen::sqlite::database db{gpkg_path};

    // Detected once per load and reused for every per-row decision below;
    // falls back to V2_2 for non-hydrofabric input (e.g. synthetic fixtures).
    HydrofabricVersion version = guaranteed_get_hydrofabric_version(db);

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

        throw std::runtime_error(errmsg);
    }

    // Introspect if the layer is divides to see which ID field is in use
    std::string id_column = get_layer_id_column(version, layer, db);

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
    std::cout << "Reading " << layer_feature_count << " features from layer " << layer << " using ID column `"
              << id_column << "`";
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
    auto query_get_layer_geom_meta = db.query("SELECT column_name FROM gpkg_geometry_columns WHERE table_name = ?", layer);
    query_get_layer_geom_meta.next();
    const std::string layer_geometry_column = query_get_layer_geom_meta.get<std::string>(0);

    // Precomputed once so the per-row loop can attribute "toid" via a
    // hashtable lookup instead of N queries; empty outside (V4_0_BETA1, "divides").
    std::unordered_map<std::string, std::string> divide_toid_lookup = build_divide_toid_lookup(version, layer, db);

    // Get layer
    auto query_get_layer = db.query("SELECT * FROM " + layer + joined_ids, ids);
    query_get_layer.next();

    // build features out of layer query
    std::vector<geojson::Feature> features;
    features.reserve(layer_feature_count);
    while(!query_get_layer.done()) {
        std::string id = query_get_layer.get<std::string>(id_column);
        geojson::PropertyMap properties = build_properties(query_get_layer, layer_geometry_column);

        // No-op for v2.2; for v4.0, aliases nexus_id/nexus_toid to id/toid and
        // injects the synthesized "toid" on divides rows from divide_toid_lookup.
        update_property_map_for_version(properties, version, layer, id, divide_toid_lookup);

        features.push_back(build_feature(
            query_get_layer,
            id,
            layer_geometry_column,
            std::move(properties)
        ));
        query_get_layer.next();
    }

    // Aggregate WARN for divides whose toid could not be synthesized, rather
    // than one line per row.
    if (is_v4(version) && layer == "divides") {
        std::size_t unlinked = 0;
        for (const auto& f : features) {
            if (!f->has_property("toid")) {
                ++unlinked;
            }
        }
        #ifndef NGEN_QUIET
        if (unlinked > 0) {
            std::cout << "WARN: " << unlinked
                      << " divide(s) have no toid (null downstream reference"
                      << " or divides -> flowpaths join miss); treated as"
                      << " terminal." << std::endl;
        }
        #endif
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
