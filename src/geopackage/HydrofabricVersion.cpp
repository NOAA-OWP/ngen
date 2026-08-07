#include "HydrofabricVersion.hpp"
#include "ngen_sqlite.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace ngen {
namespace geopackage {

HydrofabricVersion detect_version(
    const std::vector<std::string>& nexus_columns,
    const std::vector<std::string>& divides_columns)
{
    if (nexus_columns.empty()) {
        throw std::runtime_error(
            "hydrofabric detect_version: no nexus table"
        );
    }

    const bool has_nexus_id =
        std::find(nexus_columns.begin(), nexus_columns.end(), "nexus_id")
        != nexus_columns.end();
    if (has_nexus_id) {
        // Both v4 variants rename nexus.id -> nexus.nexus_id; the divides
        // layer's flowpath_toid column tells them apart (present == V4_0).
        const bool has_flowpath_toid =
            std::find(divides_columns.begin(), divides_columns.end(), "flowpath_toid")
            != divides_columns.end();
        return has_flowpath_toid ? HydrofabricVersion::V4_0
                                 : HydrofabricVersion::V4_0_BETA1;
    }

    const bool has_id =
        std::find(nexus_columns.begin(), nexus_columns.end(), "id")
        != nexus_columns.end();
    if (has_id) {
        return HydrofabricVersion::V2_2;
    }

    std::ostringstream msg;
    msg << "hydrofabric detect_version: nexus table has neither 'nexus_id' "
        << "(v4) nor 'id' (v2.2). Observed nexus columns: [";
    for (std::size_t i = 0; i < nexus_columns.size(); ++i) {
        if (i > 0) msg << ", ";
        msg << nexus_columns[i];
    }
    msg << "]";
    throw std::runtime_error(msg.str());
}

/**
 * Read the column names of @p table via PRAGMA table_info.
 *
 * PRAGMA table_info yields one row per column on the named table; column
 * index 1 is the column's name. An empty result set means the table does
 * not exist, which callers treat as "absent" rather than an error.
 *
 * @param[in] db Open GeoPackage database
 * @param[in] table Table whose columns should be listed
 * @return Observed column names, empty if the table does not exist
 */
static std::vector<std::string> read_table_columns(sqlite::database& db, const std::string& table)
{
    std::vector<std::string> columns;
    auto q = db.query("PRAGMA table_info(" + table + ")");
    q.next();
    while (!q.done()) {
        columns.emplace_back(q.get<std::string>(1));
        q.next();
    }
    return columns;
}

HydrofabricVersion detect_version(sqlite::database& db) {
    return detect_version(read_table_columns(db, "nexus"), read_table_columns(db, "divides"));
}

/**
 * Human-readable label for @p version, as used in log output.
 *
 * @param[in] version Detected hydrofabric schema version
 * @return Short version string, e.g. "v4.0beta1"
 */
static const char* version_label(const HydrofabricVersion version)
{
    switch (version) {
        case HydrofabricVersion::V4_0:       return "v4.0";
        case HydrofabricVersion::V4_0_BETA1: return "v4.0beta1";
        case HydrofabricVersion::V2_2:       return "v2.2";
    }
    return "unknown";
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
        std::cout << "INFO: hydrofabric detected: " << version_label(version) << std::endl;
    }
    #endif

    return version;
}

std::string get_layer_id_column(const HydrofabricVersion version, const std::string& layer, sqlite::database& db) {

    if (layer == "divides" && is_v4(version)) {
        // Every v4 variant always exposes divides.divide_id.
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

    if (layer == "nexus" && is_v4(version)) {
        // v4 renames nexus.id -> nexus.nexus_id.
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

    // Only v4.0beta1 needs synthesis. V4_0 reads flowpath_toid straight off
    // the divides row, so it never consults flowpaths and gets an empty map.
    if (version != HydrofabricVersion::V4_0_BETA1 || layer != "divides") {
        return divide_toid_lookup;
    }

    if (!db.contains("flowpaths")) {
#ifndef NGEN_QUIET
        std::cout << "WARN: v4.0beta1 divides loaded without a 'flowpaths' table; "
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
    if (!is_v4(version)) {
        throw std::runtime_error("Unexpected hydrofabric version " + std::to_string(static_cast<int>(version)));
    }

    if (layer == "nexus") {
        // v4 renamed nexus.id/toid -> nexus_id/nexus_toid; downstream
        // consumers still key on "id"/"toid", so alias them (additive).
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
                "v4 nexus row missing required 'nexus_id' column"
            );
        }
        if (properties.count("toid") == 0) {
            throw std::runtime_error(
                "v4 nexus row missing required 'nexus_toid' column"
            );
        }
        if (id.empty()) {
            throw std::runtime_error("v4 nexus row has empty 'id' value");
        }
    } else if (layer == "divides") {
        if (id.empty()) {
            throw std::runtime_error("v4 divides row has empty 'id' value");
        }

        if (version == HydrofabricVersion::V4_0) {
            // v4.0 divides carry flowpath_toid natively, aliased straight to
            // "toid". NULL arrives as the placeholder string "null" (see
            // build_properties); treat it as absent (terminal divide).
            auto it_toid = properties.find("flowpath_toid");
            if (it_toid == properties.end()) {
                throw std::runtime_error(
                    "v4.0 divides row missing required 'flowpath_toid' column"
                );
            }
            if (it_toid->second.as_string() != "null") {
                properties.emplace("toid", geojson::JSONProperty("toid", it_toid->second));
            }
        } else {
            if (properties.count("flowpath_id") == 0) {
                throw std::runtime_error(
                    "v4.0beta1 divides row missing required 'flowpath_id' column"
                );
            }

            // No native toid column: synthesize it from the precomputed
            // divide_id -> flowpath_toid cache. A miss (null flowpath_id,
            // join miss, or no flowpaths table) leaves "toid" unset,
            // matching v2.2 terminal-divide semantics.
            auto it = divide_toid_lookup.find(id);
            if (it != divide_toid_lookup.end()) {
                properties.emplace("toid", geojson::JSONProperty("toid", it->second));
            }
        }
    }
}

} // namespace geopackage
} // namespace ngen
