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
 * v2.2 identifies nexus rows via `id`; v4 renamed it to `nexus_id`
 * (and `toid` to `nexus_toid`). The v4 family has two variants for how
 * a divide reaches its downstream nexus: V4_0_BETA1 has no
 * `flowpath_toid` column and synthesizes "toid" by joining `divides`
 * to `flowpaths` on `flowpath_id`; V4_0 exposes `flowpath_toid`
 * natively and never consults `flowpaths`. Use is_v4() for checks
 * that apply to the whole family.
 */
enum class HydrofabricVersion {
    V2_2,
    V4_0_BETA1,
    V4_0
};

/**
 * Whether @p version belongs to the v4 family (either variant).
 *
 * @param[in] version Detected hydrofabric schema version
 * @return true for V4_0_BETA1 and V4_0, false for V2_2
 */
inline bool is_v4(const HydrofabricVersion version)
{
    return version == HydrofabricVersion::V4_0_BETA1 || version == HydrofabricVersion::V4_0;
}

/**
 * Detect the hydrofabric schema version of an open GeoPackage database.
 *
 * Inspects `nexus` via `PRAGMA table_info(nexus)`: `nexus_id` present
 * means v4, `id` means v2.2; neither (or no `nexus` table) throws
 * std::runtime_error listing the observed columns. For v4, `divides`
 * is then checked for `flowpath_toid` to pick V4_0 vs. V4_0_BETA1; a
 * missing `divides` table also yields V4_0_BETA1.
 *
 * @param[in] db Open GeoPackage database
 * @return HydrofabricVersion detected for the database
 */
HydrofabricVersion detect_version(sqlite::database& db);

/**
 * Test-friendly overload operating on already-materialized `nexus`
 * and `divides` column-name lists, for unit tests that don't want to
 * construct an on-disk SQLite database.
 *
 * @param[in] nexus_columns Column names observed on the `nexus` table;
 *            empty is treated as "no nexus table" and throws
 * @param[in] divides_columns Column names observed on the `divides`
 *            table; only consulted for v4 input, empty resolves to
 *            V4_0_BETA1
 * @return HydrofabricVersion detected from the column lists
 */
HydrofabricVersion detect_version(
    const std::vector<std::string>& nexus_columns,
    const std::vector<std::string>& divides_columns = {}
);

/**
 * Detect the hydrofabric schema version, falling back to V2_2 when
 * detect_version() throws (typically no `nexus` table, e.g. a
 * synthetic fixture or non-hydrofabric GeoPackage), preserving the
 * pre-v4 legacy code paths for such input. Logs the detected version
 * to stdout on success, unless NGEN_QUIET is defined.
 *
 * @param[in] db Open GeoPackage database
 * @return Detected hydrofabric version, or V2_2 if the GPKG has no nexus table
 */
HydrofabricVersion guaranteed_get_hydrofabric_version(sqlite::database& db);

/**
 * Resolve the column name to use as the feature id when reading rows
 * from a hydrofabric layer, centralizing the version/layer mapping so
 * read() doesn't need to scatter version checks.
 *
 *   - divides, v4: "divide_id" (always present).
 *   - divides, v2.2: "divide_id" if present, else legacy "id" (warns
 *     unless NGEN_QUIET).
 *   - nexus, v4: "nexus_id"; nexus, v2.2: "id".
 *   - anything else: "id".
 *
 * @param[in] version Hydrofabric schema version detected for this load
 * @param[in] layer Layer name being read (e.g., "divides", "nexus")
 * @param[in] db Open GeoPackage database; consulted only for v2.2
 *               divides introspection
 * @return Column name to use as the feature id
 */
std::string get_layer_id_column(HydrofabricVersion version, const std::string& layer, sqlite::database& db);

/**
 * Precompute the divide_id -> flowpath_toid map used to synthesize
 * "toid" on V4_0_BETA1 divides rows, which carry no native toid and
 * instead reference their downstream nexus indirectly via
 * flowpath_id -> flowpaths.flowpath_toid. Built once up front so the
 * per-row loop can do a hashtable lookup instead of N queries.
 *
 * Returns an empty map outside of (V4_0_BETA1, "divides") — V4_0 reads
 * flowpath_toid directly and needs no synthesis — or when the GPKG has
 * no `flowpaths` table (logs a WARN unless NGEN_QUIET). Rows with a
 * NULL flowpath_toid are excluded from the join, so unlinked divides
 * simply miss the lookup and leave "toid" unset.
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
 * Apply hydrofabric-version-specific fixups to a row's property map
 * before it is handed to build_feature(). No-op for V2_2.
 *
 * For v4:
 *   - "nexus" (both variants): aliases nexus_id/nexus_toid to id/toid
 *     (additive — originals remain). Throws if either source column
 *     is missing or `id` is empty.
 *   - "divides", V4_0: requires "flowpath_toid" present and `id`
 *     non-empty, then aliases flowpath_toid -> toid. A NULL value
 *     leaves "toid" unset rather than propagating a placeholder.
 *   - "divides", V4_0_BETA1: requires "flowpath_id" present and `id`
 *     non-empty, then synthesizes "toid" via divide_toid_lookup. A
 *     miss leaves "toid" unset, matching v2.2 terminal-divide
 *     semantics so a post-loop summary can count unlinked divides.
 *
 * @param[in,out] properties Property map to update in place
 * @param[in] version Detected hydrofabric schema version
 * @param[in] layer Layer name being read (e.g., "nexus", "divides")
 * @param[in] id Resolved feature id for the row; used for
 *               empty-string validation and as the lookup key into
 *               divide_toid_lookup
 * @param[in] divide_toid_lookup Precomputed divide_id ->
 *               flowpath_toid map; consulted only for
 *               (V4_0_BETA1, "divides") rows. Pass an empty map
 *               otherwise.
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
