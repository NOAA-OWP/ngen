#include "HydrofabricVersion.hpp"

#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace ngen {
namespace geopackage {

HydrofabricVersion detect_version(const std::vector<std::string>& nexus_columns)
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

HydrofabricVersion detect_version(sqlite3* db)
{
    if (db == nullptr) {
        throw std::runtime_error(
            "hydrofabric detect_version: null sqlite3 handle"
        );
    }

    std::vector<std::string> columns;

    sqlite3_stmt* stmt = nullptr;
    const int prep_code = sqlite3_prepare_v2(
        db, "PRAGMA table_info(nexus)", -1, &stmt, nullptr
    );
    if (prep_code != SQLITE_OK) {
        const std::string err = sqlite3_errmsg(db);
        if (stmt != nullptr) {
            sqlite3_finalize(stmt);
        }
        throw std::runtime_error(
            "hydrofabric detect_version: failed to prepare "
            "PRAGMA table_info(nexus): " + err
        );
    }

    // PRAGMA table_info yields one row per column; the second column (index 1)
    // is the column's name. An empty result set means the table does not exist.
    while (true) {
        const int step_code = sqlite3_step(stmt);
        if (step_code == SQLITE_DONE) {
            break;
        }
        if (step_code != SQLITE_ROW) {
            const std::string err = sqlite3_errmsg(db);
            sqlite3_finalize(stmt);
            throw std::runtime_error(
                "hydrofabric detect_version: failed to step "
                "PRAGMA table_info(nexus): " + err
            );
        }

        const unsigned char* name = sqlite3_column_text(stmt, 1);
        if (name != nullptr) {
            columns.emplace_back(reinterpret_cast<const char*>(name));
        }
    }

    sqlite3_finalize(stmt);

    return detect_version(columns);
}

} // namespace geopackage
} // namespace ngen
