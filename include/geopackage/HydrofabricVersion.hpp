#ifndef HYDROFABRIC_VERSION_H
#define HYDROFABRIC_VERSION_H

#include <string>
#include <vector>

#include <sqlite3.h>

namespace ngen {
namespace geopackage {

/**
 * Discrete hydrofabric schema versions supported by the loader.
 *
 * v2.2 identifies nexus rows via the `id` column; v3.0 renamed it
 * to `nexus_id` (and uses `nexus_toid` as the downstream reference).
 */
enum class HydrofabricVersion {
    V2_2,
    V3_0
};

/**
 * Detect the hydrofabric schema version of an open GeoPackage database.
 *
 * Inspects the `nexus` table via `PRAGMA table_info(nexus)`. If the
 * column `nexus_id` is present, this is a v3.0 hydrofabric. Otherwise,
 * if `id` is present, this is a v2.2 hydrofabric. If neither is
 * present (or if the `nexus` table is missing entirely), a
 * std::runtime_error is thrown with a message listing the observed
 * nexus columns (or stating that there is no nexus table).
 *
 * @param[in] db Open sqlite3 handle to a GeoPackage database
 * @return HydrofabricVersion detected for the database
 */
HydrofabricVersion detect_version(sqlite3* db);

/**
 * Test-friendly overload that operates on an already-materialized list
 * of column names taken from the `nexus` table. Intended for unit tests
 * that do not want to construct an on-disk SQLite database.
 *
 * An empty column list is treated as "no nexus table" and throws.
 *
 * @param[in] nexus_columns Column names observed on the `nexus` table
 * @return HydrofabricVersion detected from the column list
 */
HydrofabricVersion detect_version(const std::vector<std::string>& nexus_columns);

} // namespace geopackage
} // namespace ngen

#endif // HYDROFABRIC_VERSION_H
