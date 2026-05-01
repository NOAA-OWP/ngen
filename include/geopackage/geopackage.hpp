#ifndef NGEN_GEOPACKAGE_H
#define NGEN_GEOPACKAGE_H

#include <string>

#include "FeatureCollection.hpp"
#include "ngen_sqlite.hpp"

namespace ngen {
namespace geopackage {

/**
 * Build a geometry object from GeoPackage WKB.
 * 
 * @param[in] row SQLite iterator at the row containing a geometry column
 * @param[in] geom_col Name of geometry column containing GPKG WKB
 * @param[out] bounding_box Bounding box of the geometry to output
 * @return geojson::geometry GPKG WKB converted and projected to a boost geometry model
 */
geojson::geometry build_geometry(
    const ngen::sqlite::database::iterator& row,
    const std::string& geom_col,
    std::vector<double>& bounding_box
);

/**
 * Build properties from GeoPackage table columns.
 * 
 * @param[in] row SQLite iterator at the row containing the data columns
 * @param[in] geom_col Name of geometry column containing GPKG WKB to ignore
 * @return geojson::PropertyMap PropertyMap of properties from the given row
 */
geojson::PropertyMap build_properties(
    const ngen::sqlite::database::iterator& row,
    const std::string& geom_col
);

/**
 * Build a feature from a GPKG table row.
 *
 * This function is intentionally schema-agnostic: it reads only the
 * geometry from `row` and constructs the appropriate geojson::*Feature
 * subclass around the supplied `id` and `properties`. All
 * schema-specific concerns (resolving which column holds the id,
 * aliasing renamed columns, synthesizing fields that have no native
 * column) are the caller's responsibility and must be applied to
 * `id` and `properties` before this function is called.
 *
 * The `properties` map should hold the row's non-geometry columns; the
 * geometry column is read separately from `row` and must not appear in
 * the map. Downstream geojson consumers commonly key on the property
 * map's "id" entry rather than the Feature's id field, so the map
 * should contain an "id" entry whose value matches the `id` parameter.
 * When the source id column is not named "id" (e.g. hydrofabric v3.0
 * nexus uses "nexus_id"), the caller is responsible for adding the
 * canonical "id" alias — and any related aliases such as "toid" —
 * before calling. Schema-specific field synthesis (e.g. v3.0 divides
 * "toid" derived from a divides→flowpaths join) must likewise be
 * performed by the caller.
 *
 * @param[in] row SQLite iterator at the row to build a feature from
 * @param[in] id Resolved feature id; stored on the returned Feature
 * @param[in] geom_col Name of geometry column containing GPKG WKB
 * @param[in] properties Pre-built property map for the feature
 * @return geojson::Feature Feature containing geometry and properties from the given row
 */
geojson::Feature build_feature(
    const ngen::sqlite::database::iterator& row,
    const std::string& id,
    const std::string& geom_col,
    geojson::PropertyMap properties
);

/**
 * Build a feature collection from a GPKG layer
 *
 * @param[in] gpkg_path Path to GPKG file
 * @param[in] layer Layer name within GPKG file to create a collection from
 * @param[in] ids optional subset of feature IDs to capture (if empty, the entire layer is converted)
 * @return std::shared_ptr<geojson::FeatureCollection> 
 */
std::shared_ptr<geojson::FeatureCollection> read(
    const std::string& gpkg_path,
    const std::string& layer,
    const std::vector<std::string>& ids
);

} // namespace geopackage
} // namespace ngen
#endif // NGEN_GEOPACKAGE_H
