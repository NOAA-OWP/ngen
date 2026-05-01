#ifndef HYDROFABRIC_VERSION_H
#define HYDROFABRIC_VERSION_H

#include <string>
#include <unordered_map>
#include <vector>

#include "JSONProperty.hpp"

namespace ngen {
    // Forward declare for efficiency
    namespace sqlite {
        class database;
    }
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
 * @param[in] db Open GeoPackage database
 * @return HydrofabricVersion detected for the database
 */
HydrofabricVersion detect_version(sqlite::database& db);

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

/**
 * Detect the hydrofabric schema version, falling back to V2_2 for
 * input that is not detectable as a hydrofabric.
 *
 * Calls detect_version() and, on success, logs the detected version
 * to stdout (unless NGEN_QUIET is defined). If detect_version() throws
 * — typically because the GPKG has no `nexus` table (e.g., a synthetic
 * test fixture or a non-hydrofabric GeoPackage) — the exception is
 * swallowed and HydrofabricVersion::V2_2 is returned. The default
 * preserves the pre-v3.0 legacy code paths for non-hydrofabric inputs,
 * which is why this entry point is "guaranteed" to return a value
 * rather than propagate the missing-nexus error.
 *
 * Use this when the caller is hydrofabric-aware but must tolerate
 * non-hydrofabric inputs without aborting; use detect_version()
 * directly when a missing nexus table must be reported as an error.
 *
 * @param[in] db Open GeoPackage database
 * @return Detected hydrofabric version, or V2_2 if the GPKG has no nexus table
 */
HydrofabricVersion guaranteed_get_hydrofabric_version(sqlite::database& db);

/**
 * Resolve the column name to use as the feature id when reading rows
 * from a hydrofabric layer.
 *
 * Different hydrofabric versions and layers expose different
 * primary-id column names; this helper centralizes the mapping so the
 * loader can route SQL subset filters and per-row id lookups against
 * the correct column without scattering version checks throughout
 * read().
 *
 * Resolution rules:
 *   - layer == "divides", v3.0: "divide_id" (always exposed in v3.0).
 *   - layer == "divides", v2.2: probes the database with
 *     `SELECT divide_id FROM divides LIMIT 1`. If the column exists,
 *     "divide_id" is returned; otherwise the function falls back to
 *     "id" and emits a deprecation warning to stdout (suppressed
 *     when NGEN_QUIET is defined).
 *   - layer == "nexus",   v3.0: "nexus_id" (renamed from "id" in v3.0).
 *   - layer == "nexus",   v2.2: "id".
 *   - All other layer/version combinations: "id" (the historical
 *     default for non-hydrofabric or otherwise unhandled layers).
 *
 * @param[in] version Hydrofabric schema version detected for this load
 * @param[in] layer Layer name being read (e.g., "divides", "nexus")
 * @param[in] db Open GeoPackage database; consulted only for v2.2
 *               divides introspection
 * @return Column name to use as the feature id
 */
std::string get_layer_id_column(HydrofabricVersion version, const std::string& layer, sqlite::database& db);

/**
 * Precompute the divide_id -> flowpath_toid map used to synthesize a
 * "toid" property on v3.0 divides rows.
 *
 * v3.0 divides carry no native "toid" column; instead, every divide
 * points at a flowpath via flowpath_id, and that flowpath's
 * flowpath_toid is the downstream nexus. Building the map once
 * up-front lets the per-row loop attribute toid via a hashtable
 * lookup rather than N database queries. The underlying SQL joins
 * divides and flowpaths and drops rows whose flowpath_toid is NULL,
 * so the cache only contains well-formed links; unlinked divides
 * (null flowpath_id, or flowpath present but with a null toid)
 * naturally miss the lookup and leave "toid" unset on the resulting
 * feature.
 *
 * Returns an empty map (no synthesis) for any (version, layer)
 * combination outside of (V3_0, "divides"), and also when the GPKG
 * does not contain a "flowpaths" table — in the latter case a one-line
 * WARN is logged to stdout (suppressed when NGEN_QUIET is defined)
 * since v3.0 divides without flowpaths means every divide will be
 * treated as terminal.
 *
 * @param[in] version Detected hydrofabric schema version
 * @param[in] layer Layer name being read
 * @param[in] db Open GeoPackage database
 * @return Map from divide_id to flowpath_toid; empty when synthesis is
 *         not applicable or not possible for this (version, layer)
 */
std::unordered_map<std::string, std::string> build_divide_toid_lookup(
    HydrofabricVersion version,
    const std::string& layer,
    sqlite::database& db
);

/**
 * Update a row's property map to satisfy hydrofabric-version-specific
 * conventions before the row is handed to build_feature().
 *
 * For HydrofabricVersion::V2_2 input, this is a no-op.
 *
 * For HydrofabricVersion::V3_0 input, the function dispatches on the
 * layer name and applies the v3.0 schema fixups:
 *   - layer == "nexus": v3.0 renamed nexus.id -> nexus.nexus_id and
 *     nexus.toid -> nexus.nexus_toid. Downstream consumers still key
 *     on "id" / "toid", so the v3.0 column values are aliased into
 *     those names. The original "nexus_id" / "nexus_toid" entries
 *     remain in the map (additive). Throws std::runtime_error if
 *     either source column is missing or if `id` is empty.
 *   - layer == "divides": validates that the v3.0 foreign-key column
 *     "flowpath_id" is present and that `id` is non-empty. v3.0
 *     divides have no native "toid" column, so this function
 *     synthesizes one by looking up `id` in the precomputed
 *     `divide_toid_lookup` (built upstream from a divides JOIN
 *     flowpaths query). A miss leaves "toid" unset, matching v2.2
 *     terminal-divide semantics and letting any post-loop summary
 *     count the unlinked divides.
 *
 * @param[in,out] properties Property map to update in place
 * @param[in] version Detected hydrofabric schema version
 * @param[in] layer Layer name being read (e.g., "nexus", "divides")
 * @param[in] id Resolved feature id for the row; used for
 *               empty-string validation and as the lookup key into
 *               divide_toid_lookup
 * @param[in] divide_toid_lookup Precomputed v3.0 divide_id ->
 *               flowpath_toid map; consulted only for
 *               (V3_0, "divides") rows. Pass an empty map when
 *               synthesis is unavailable (e.g., no flowpaths table)
 *               or when the layer is not "divides".
 */
void update_property_map_for_version(
    geojson::PropertyMap& properties,
    HydrofabricVersion version,
    const std::string& layer,
    const std::string& id,
    const std::unordered_map<std::string, std::string>& divide_toid_lookup
);

} // namespace geopackage
} // namespace ngen

#endif // HYDROFABRIC_VERSION_H
