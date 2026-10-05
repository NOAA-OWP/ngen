#ifndef NGEN_GEOPACKAGE_HYDROFABRIC_SCHEMA_H
#define NGEN_GEOPACKAGE_HYDROFABRIC_SCHEMA_H

#include <set>
#include <string>
#include <unordered_map>

#include "HydrofabricVersion.hpp"

namespace ngen {
    // Forward declare for efficiency
    namespace sqlite {
        class database;
    }
namespace hydrofabric {

//! GeoPackage hydrofabric layer (table) names, shared by the readers and the schema utilities.
constexpr const char* const DIVIDES_LAYER = "divides";
constexpr const char* const NEXUS_LAYER = "nexus";
constexpr const char* const FLOWPATHS_LAYER = "flowpaths";

//! Canonical feature property keys ngen consumes, which are also hydrofabric v2.2's own column
//! names -- the release the canonical vocabulary was taken from.
constexpr const char* const ID_KEY = "id";
constexpr const char* const TOID_KEY = "toid";

//! Version-specific column names, translated onto the canonical keys by the readers.
constexpr const char* const DIVIDE_ID_COLUMN = "divide_id";
constexpr const char* const NEXUS_ID_COLUMN = "nexus_id";
constexpr const char* const NEXUS_TOID_COLUMN = "nexus_toid";
constexpr const char* const FLOWPATH_ID_COLUMN = "flowpath_id";
constexpr const char* const FLOWPATH_TOID_COLUMN = "flowpath_toid";

/**
 * Identify which hydrofabric release a set of observed columns describes.
 *
 * `nexus_id` present means v4, `id` means v2.2; neither throws std::runtime_error listing the
 * observed columns. For v4, `divides` is then checked for `flowpath_toid` to pick V4_0 vs.
 * V4_0_BETA1.
 *
 * @param[in] nexus_columns Column names observed on the `nexus` table; empty is treated as "no
 *            nexus table" and throws
 * @param[in] divides_columns Column names observed on the `divides` table; only consulted for v4
 *            input, empty resolves to V4_0_BETA1
 * @return HydrofabricVersion identified from the column sets
 * @throws std::runtime_error if the column sets match no known release
 */
HydrofabricVersion detect_version(
    const std::set<std::string>& nexus_columns,
    const std::set<std::string>& divides_columns = {}
);

/**
 * Identify the release of a hydrofabric whose layers may live in two GeoPackages.
 *
 * Each column list is read from the file that holds its layer. This matters for more than
 * tidiness: v4.0 is only distinguishable from v4.0beta1 by `divides.flowpath_toid`, and an absent
 * table reads as an empty column list, so identifying a split v4.0 hydrofabric against its nexus
 * file alone would report V4_0_BETA1.
 *
 * @param[in] nexus_db Open database holding the `nexus` layer
 * @param[in] divides_db Open database holding the `divides` layer; may be the same database
 * @return HydrofabricVersion identified for the pair
 * @throws std::runtime_error if the observed schema matches no known release
 */
HydrofabricVersion detect_version(const sqlite::database& nexus_db, const sqlite::database& divides_db);

/**
 * Identify the release of a hydrofabric held entirely in one GeoPackage.
 *
 * @param[in] db Open GeoPackage database holding both layers
 * @return HydrofabricVersion identified for the database
 * @throws std::runtime_error if the observed schema matches no known release
 */
HydrofabricVersion detect_version(const sqlite::database& db);

/**
 * Test whether a GeoPackage is a hydrofabric, and identify it if so.
 *
 * The `nexus` layer is what marks a GeoPackage as a hydrofabric. Absent, the file is something
 * else entirely -- a plain GeoPackage of arbitrary tables -- and the result is UNRECOGNIZED;
 * ngen::geopackage::GeoPackageReader still reads it perfectly well, it is simply not a
 * hydrofabric. Present, the file is a hydrofabric and is identified, or else rejected: a `nexus`
 * layer whose schema matches no known release is an error, not a release.
 *
 * This differs from detect_version() only in what it does with a GeoPackage that is not a
 * hydrofabric at all: detect_version() presumes it is one and throws, while this reports
 * UNRECOGNIZED and leaves the judgment to the caller.
 *
 * @param[in] nexus_db Open database expected to hold the `nexus` layer
 * @param[in] divides_db Open database holding the `divides` layer; may be the same database
 * @return HydrofabricVersion identified for the pair, or UNRECOGNIZED if @p nexus_db has no
 *         `nexus` layer
 * @throws std::runtime_error if a `nexus` layer is present but matches no known release
 */
HydrofabricVersion detect_hydrofabric(
    const sqlite::database& nexus_db,
    const sqlite::database& divides_db
);

/**
 * Test whether a single-file GeoPackage is a hydrofabric, and identify it if so.
 *
 * @param[in] db Open GeoPackage database holding both layers
 * @return HydrofabricVersion identified, or UNRECOGNIZED if @p db has no `nexus` layer
 * @throws std::runtime_error if a `nexus` layer is present but matches no known release
 */
HydrofabricVersion detect_hydrofabric(const sqlite::database& db);

/**
 * Resolve the column name to use as the feature id when reading rows from a hydrofabric layer,
 * centralizing the release/layer mapping so no caller needs to scatter version checks.
 *
 *   - divides, v4: "divide_id".
 *   - divides, v2.2: "divide_id" if present, else legacy "id" (warns unless NGEN_QUIET).
 *   - nexus, v4: "nexus_id"; nexus, v2.2: "id".
 *   - any other layer: "id".
 *   - a layer absent from @p db entirely: the sentinel "<unset>", which is deliberately not a
 *     legal column name -- there is no id column to name, and a plausible-looking guess would
 *     only misdirect whoever reads it.
 *
 * The resolved column is verified to exist in @p layer, so a schema surprise is reported here as
 * the problem it is rather than as a confusing failure from the read that follows. An absent
 * @p layer is not reported here, though: the subsequent read names the missing table with more
 * context than a column check could.
 *
 * @param[in] version Hydrofabric release identified for this load
 * @param[in] layer Layer name being read (e.g., "divides", "nexus")
 * @param[in] db Open GeoPackage database holding @p layer
 * @return Column name to use as the feature id
 * @throws std::runtime_error if @p layer exists but lacks the resolved column
 */
std::string get_layer_id_column(HydrofabricVersion version, const std::string& layer, const sqlite::database& db);

/**
 * Precompute the divide_id -> flowpath_toid map used to synthesize "toid" on V4_0_BETA1 divides
 * rows, which carry no native toid and instead reference their downstream nexus indirectly via
 * flowpath_id -> flowpaths.flowpath_toid. Built once up front so the per-row loop can do a
 * hashtable lookup instead of N queries.
 *
 * Returns an empty map outside of (V4_0_BETA1, "divides") -- V4_0 reads flowpath_toid directly and
 * needs no synthesis -- or when the GPKG has no `flowpaths` table (warns unless NGEN_QUIET).
 * Rows with a NULL flowpath_toid are excluded from the join, so unlinked divides simply miss the
 * lookup and leave "toid" unset.
 *
 * @param[in] version Identified hydrofabric release
 * @param[in] layer Layer name being read
 * @param[in] db Open GeoPackage database
 * @return Map from divide_id to flowpath_toid; empty when synthesis is not applicable or not
 *         possible for this (version, layer)
 */
std::unordered_map<std::string, std::string> build_divide_toid_lookup(
    HydrofabricVersion version,
    const std::string& layer,
    const sqlite::database& db
);

} // namespace hydrofabric
} // namespace ngen

#endif // NGEN_GEOPACKAGE_HYDROFABRIC_SCHEMA_H
