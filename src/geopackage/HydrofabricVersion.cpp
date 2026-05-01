#include "HydrofabricVersion.hpp"
#include "ngen_sqlite.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace ngen {
namespace geopackage {

HydrofabricVersion detect_version(const std::vector<std::string>& nexus_columns) {
    if (nexus_columns.empty()) {
        throw std::runtime_error(
            "hydrofabric detect_version: no nexus table"
        );
    }

    const bool has_nexus_id =
        std::find(nexus_columns.begin(), nexus_columns.end(), "nexus_id")
        != nexus_columns.end();
    if (has_nexus_id) {
        return HydrofabricVersion::V3_0;
    }

    const bool has_id =
        std::find(nexus_columns.begin(), nexus_columns.end(), "id")
        != nexus_columns.end();
    if (has_id) {
        return HydrofabricVersion::V2_2;
    }

    std::ostringstream msg;
    msg << "hydrofabric detect_version: nexus table has neither 'nexus_id' "
        << "(v3.0) nor 'id' (v2.2). Observed nexus columns: [";
    for (std::size_t i = 0; i < nexus_columns.size(); ++i) {
        if (i > 0) msg << ", ";
        msg << nexus_columns[i];
    }
    msg << "]";
    throw std::runtime_error(msg.str());
}

HydrofabricVersion detect_version(sqlite::database& db) {
    // PRAGMA table_info yields one row per column on the named table; column
    // index 1 is the column's name. An empty result set means the table does
    // not exist.
    std::vector<std::string> columns;
    auto q = db.query("PRAGMA table_info(nexus)");
    q.next();
    while (!q.done()) {
        columns.emplace_back(q.get<std::string>(1));
        q.next();
    }

    return detect_version(columns);
}

HydrofabricVersion guaranteed_get_hydrofabric_version(sqlite::database& db) {
    HydrofabricVersion version = HydrofabricVersion::V2_2;
    bool version_detected = false;
    try {
        version = detect_version(db);
        version_detected = true;
    } catch (const std::runtime_error&) {
        // swallow: this GPKG does not carry a hydrofabric `nexus` table
    }

    #ifndef NGEN_QUIET
    if (version_detected) {
        std::cout << "INFO: hydrofabric detected: "
                  << (version == HydrofabricVersion::V3_0 ? "v3.0" : "v2.2")
                  << std::endl;
    }
    #endif

    return version;
}

std::string get_layer_id_column(const HydrofabricVersion version, const std::string& layer, sqlite::database& db) {

    if (layer == "divides" && version == HydrofabricVersion::V3_0) {
        // v3.0 always exposes divides.divide_id; no runtime introspection
        // needed. The flowpath_id column (used by the upcoming toid
        // synthesis step) is carried verbatim through build_properties.
        return "divide_id";
    }

    if (layer == "divides" && version == HydrofabricVersion::V2_2) {
        try {
            auto query_get_first_row = db.query("SELECT divide_id FROM " + layer + " LIMIT 1");
            return "divide_id";
        }
        catch (const std::exception& e){
            #ifndef NGEN_QUIET
            // output debug info on what is read exactly
            std::cout << "WARN: Using legacy ID column \"id\" in layer " << layer
                      << " is DEPRECATED and may stop working at any time."
                      << std::endl;
            #endif
            return "id";
        }
    }

    if (layer == "nexus" && version == HydrofabricVersion::V3_0) {
        // v3.0 renames nexus.id -> nexus.nexus_id; point id_column at the
        // new primary key so the WHERE-IN subset clause and the per-row id
        // lookup in build_feature both resolve against the right column.
        return "nexus_id";
    }

    if (layer == "nexus" && version == HydrofabricVersion::V2_2) {
        return "id";
    }

    // Default fallback
    return "id";
}

std::unordered_map<std::string, std::string> build_divide_toid_lookup(
    HydrofabricVersion version,
    const std::string& layer,
    sqlite::database& db)
{
    std::unordered_map<std::string, std::string> divide_toid_lookup;

    if (version != HydrofabricVersion::V3_0 || layer != "divides") {
        return divide_toid_lookup;
    }

    if (!db.contains("flowpaths")) {
#ifndef NGEN_QUIET
        std::cout << "WARN: v3.0 divides loaded without a 'flowpaths' table; "
                  << "all divides will be treated as terminal (no toid)." << std::endl;
#endif
        return divide_toid_lookup;
    }

    auto q = db.query(
        "SELECT d.divide_id, f.flowpath_toid "
        "FROM divides d "
        "JOIN flowpaths f ON d.flowpath_id = f.flowpath_id "
        "WHERE f.flowpath_toid IS NOT NULL"
    );
    q.next();
    while (!q.done()) {
        divide_toid_lookup.emplace(q.get<std::string>(0), q.get<std::string>(1));
        q.next();
    }
    return divide_toid_lookup;
}

void update_property_map_for_version(
    geojson::PropertyMap& properties,
    HydrofabricVersion version,
    const std::string& layer,
    const std::string& id,
    const std::unordered_map<std::string, std::string>& divide_toid_lookup)
{
    if (version == HydrofabricVersion::V2_2) {
        return;
    }
    if (version != HydrofabricVersion::V3_0) {
        throw std::runtime_error("Unexpected hydrofabric version " + std::to_string(static_cast<int>(version)));
    }

    if (layer == "nexus") {
        // v3.0 renamed nexus.id -> nexus.nexus_id and nexus.toid ->
        // nexus.nexus_toid. Downstream consumers still key on "id" /
        // "toid", so alias the v3.0 columns into those names. The
        // originals remain in the map (additive) so any future consumer
        // that prefers the schema names keeps working.
        auto it_nid = properties.find("nexus_id");
        if (it_nid != properties.end()) {
            properties.emplace("id", geojson::JSONProperty("id", it_nid->second));
        }
        auto it_ntoid = properties.find("nexus_toid");
        if (it_ntoid != properties.end()) {
            properties.emplace("toid", geojson::JSONProperty("toid", it_ntoid->second));
        }
        if (properties.count("id") == 0) {
            throw std::runtime_error(
                "v3.0 nexus row missing required 'nexus_id' column"
            );
        }
        if (properties.count("toid") == 0) {
            throw std::runtime_error(
                "v3.0 nexus row missing required 'nexus_toid' column"
            );
        }
        if (id.empty()) {
            throw std::runtime_error("v3.0 nexus row has empty 'id' value");
        }
    } else if (layer == "divides") {
        // v3.0 divides carry flowpath_id as the foreign key into
        // flowpaths. build_properties already copies it verbatim (it is
        // a non-geometry column); guard that invariant here so the
        // toid-synthesis lookup below can rely on it.
        if (properties.count("flowpath_id") == 0) {
            throw std::runtime_error(
                "v3.0 divides row missing required 'flowpath_id' column"
            );
        }
        if (id.empty()) {
            throw std::runtime_error("v3.0 divides row has empty 'id' value");
        }

        // v3.0 divides have no native toid column: synthesize it by
        // looking up this divide's id in the precomputed divide_id ->
        // flowpath_toid cache (built upstream from divides JOIN
        // flowpaths). If the lookup misses (e.g., flowpath_id was null,
        // the join did not resolve, or no flowpaths table exists and
        // the cache is empty), leave "toid" unset — that matches v2.2
        // terminal-divide semantics and lets any post-loop summary
        // count the unlinked divides.
        auto it = divide_toid_lookup.find(id);
        if (it != divide_toid_lookup.end()) {
            properties.emplace("toid", geojson::JSONProperty("toid", it->second));
        }
    }
}

} // namespace geopackage
} // namespace ngen
