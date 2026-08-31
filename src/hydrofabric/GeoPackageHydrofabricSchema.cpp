#include "GeoPackageHydrofabricSchema.hpp"
#include "ngen_sqlite.hpp"

#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>

namespace ngen {
namespace hydrofabric {

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

    std::ostringstream msg;
    msg << "hydrofabric detect_version: nexus table has neither 'nexus_id' "
        << "(v4) nor 'id' (v2.2). Observed nexus columns: [";
    bool first = true;
    for (const std::string& column : nexus_columns) {
        if (!first) msg << ", ";
        msg << column;
        first = false;
    }
    msg << "]";
    throw std::runtime_error(msg.str());
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
