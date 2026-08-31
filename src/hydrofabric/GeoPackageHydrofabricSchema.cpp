#include "GeoPackageHydrofabricSchema.hpp"
#include "ngen_sqlite.hpp"

#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>

namespace ngen {
namespace hydrofabric {

namespace {

/**
 * Render a column set as a bracketed, comma-separated list for error messages.
 *
 * @param[in] columns Column names to render
 * @return String of the form "[a, b, c]"
 */
std::string join_columns(const std::set<std::string>& columns)
{
    std::ostringstream joined;
    joined << "[";
    bool first = true;
    for (const std::string& column : columns) {
        if (!first) joined << ", ";
        joined << column;
        first = false;
    }
    joined << "]";
    return joined.str();
}

} // namespace

HydrofabricVersion detect_version(
    const std::set<std::string>& nexus_columns,
    const std::set<std::string>& divides_columns)
{
    if (nexus_columns.empty()) {
        throw std::runtime_error(
            "hydrofabric detect_version: no nexus table"
        );
    }

    if (nexus_columns.count("nexus_id") > 0) {
        // Both v4 variants rename nexus.id -> nexus.nexus_id; the divides
        // layer's flowpath_toid column tells them apart (present == V4_0).
        return divides_columns.count("flowpath_toid") > 0 ? HydrofabricVersion::V4_0
                                                          : HydrofabricVersion::V4_0_BETA1;
    }

    if (nexus_columns.count("id") > 0) {
        return HydrofabricVersion::V2_2;
    }

    throw std::runtime_error(
        "hydrofabric detect_version: nexus table has neither 'nexus_id' (v4) nor 'id' (v2.2). "
        "Observed nexus columns: " + join_columns(nexus_columns)
    );
}

HydrofabricVersion detect_version(const sqlite::database& nexus_db, const sqlite::database& divides_db) {
    return detect_version(nexus_db.columns("nexus"), divides_db.columns("divides"));
}

HydrofabricVersion detect_version(const sqlite::database& db) {
    return detect_version(db, db);
}

HydrofabricVersion detect_hydrofabric(
    const sqlite::database& nexus_db,
    const sqlite::database& divides_db) {

    if (!nexus_db.contains("nexus")) {
        return HydrofabricVersion::UNRECOGNIZED;
    }

    // A `nexus` table is present, so this is a hydrofabric; let detect_version()
    // throw its detailed error if the schema doesn't match any known version,
    // rather than reporting it as some version it isn't.
    return detect_version(nexus_db, divides_db);
}

HydrofabricVersion detect_hydrofabric(const sqlite::database& db) {
    return detect_hydrofabric(db, db);
}

std::string get_layer_id_column(const HydrofabricVersion version, const std::string& layer,
                                const sqlite::database& db) {

    const std::set<std::string> columns = db.columns(layer);
    if (columns.empty()) {
        // No columns means no such table, so there is no id column to name: return a value that
        // cannot be confused with a real column rather than a plausible-looking guess. Reporting
        // the missing table itself is left to the read that follows, which names it with more
        // context (the tables the file does have) than a column check can.
        return "<unset>";
    }

    std::string id_column = "id";
    if (layer == "divides" && is_v4(version)) {
        // Every v4 variant exposes divides.divide_id; verified below like everything else.
        id_column = "divide_id";
    }
    else if (layer == "divides" && version == HydrofabricVersion::V2_2) {
        if (columns.count("divide_id") > 0) {
            id_column = "divide_id";
        }
        #ifndef NGEN_QUIET
        else if (columns.count("id") > 0) {
            // output debug info on what is read exactly
            std::cout << "WARN: Using legacy ID column \"id\" in layer " << layer
                      << " is DEPRECATED and may stop working at any time."
                      << std::endl;
        }
        #endif
    }
    else if (layer == "nexus" && is_v4(version)) {
        // v4 renames nexus.id -> nexus.nexus_id.
        id_column = "nexus_id";
    }
    // v2.2 nexus reads its native "id", as does any other layer.

    if (columns.count(id_column) == 0) {
        throw std::runtime_error(
            "hydrofabric " + std::string(version_label(version)) + " " + layer +
            " layer has no '" + id_column + "' id column. Observed columns: " +
            join_columns(columns)
        );
    }
    return id_column;
}

std::unordered_map<std::string, std::string> build_divide_toid_lookup(
    HydrofabricVersion version,
    const std::string& layer,
    const sqlite::database& db)
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

} // namespace hydrofabric
} // namespace ngen
