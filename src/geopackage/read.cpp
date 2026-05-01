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

// Required tables for hydrofabric GeoPackages:
//   nexus      — read for v2.2 and v3.0 (id/toid vs nexus_id/nexus_toid)
//   divides    — read for v2.2 and v3.0 (divide_id / id fallback)
//   flowpaths  — read only for v3.0 divides toid synthesis (JOIN); optional
//
// Metadata tables always read by SQLite / geopackage infrastructure:
//   gpkg_geometry_columns — geometry column name lookup
//   sqlite_master         — table-existence checks (db.contains / PRAGMA table_info)
//
// Auxiliary tables that MUST NOT be touched by this loader:
//   network, flowlines, flowline_endpoints, hydrolocations, pois,
//   incremental_areas, lakes, divide-attributes, flowpath-attributes,
//   flowpath-attributes-ml
std::shared_ptr<geojson::FeatureCollection> ngen::geopackage::read(
    const std::string& gpkg_path,
    const std::string& layer = "",
    const std::vector<std::string>& ids = {}
)
{
    // Check for malicious/invalid layer input
    check_table_name(layer);

    ngen::sqlite::database db{gpkg_path};

    // Detect the hydrofabric schema version once per file load and reuse the
    // result for every per-row decision below. The "guaranteed" variant
    // collapses the not-a-hydrofabric case (no `nexus` table, e.g. synthetic
    // test fixtures) into HydrofabricVersion::V2_2 so the pre-v3.0 legacy
    // code paths remain intact.
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

    // Precompute the v3.0 divide_id -> flowpath_toid map once before the
    // per-row loop, so update_property_map_for_version can attribute "toid"
    // via a hashtable lookup. Empty map for any (version, layer) other than
    // (V3_0, "divides"), or when the GPKG has no flowpaths table.
    std::unordered_map<std::string, std::string> divide_toid_lookup = build_divide_toid_lookup(version, layer, db);

    // Get layer
    auto query_get_layer = db.query("SELECT * FROM " + layer + joined_ids, ids);
    query_get_layer.next();

    // build features out of layer query
    //
    // All schema-specific work happens here so that build_feature() can
    // remain version-agnostic: the per-row body resolves the id, builds
    // the property map, applies any v3.0 column aliasing or synthesis,
    // and only then hands the prepared inputs to build_feature().
    std::vector<geojson::Feature> features;
    features.reserve(layer_feature_count);
    while(!query_get_layer.done()) {
        std::string id = query_get_layer.get<std::string>(id_column);
        geojson::PropertyMap properties = build_properties(query_get_layer, layer_geometry_column);

        // No-op for v2.2; for v3.0, aliases nexus_id/nexus_toid to id/toid and
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

    // Summary WARN for v3.0 divides whose toid could not be synthesized via
    // the divides -> flowpaths join (null flowpath_id, or join miss). A
    // single aggregate line avoids flooding logs when a large subset is
    // terminal.
    if (version == HydrofabricVersion::V3_0 && layer == "divides") {
        std::size_t unlinked = 0;
        for (const auto& f : features) {
            if (!f->has_property("toid")) {
                ++unlinked;
            }
        }
        #ifndef NGEN_QUIET
        if (unlinked > 0) {
            std::cout << "WARN: " << unlinked
                      << " v3.0 divide(s) have no toid (flowpath_id null or"
                      << " join miss on divides -> flowpaths); treated as"
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
