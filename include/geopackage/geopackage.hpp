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
 * Schema-agnostic: reads only the geometry from `row` and wraps the
 * given `id` and `properties` in the appropriate geojson::*Feature
 * subclass. The id is taken from `id` alone -- nothing here reads it
 * back out of `properties` -- so resolving which column an id came
 * from, and publishing any derived property, is the caller's business,
 * before or after. `properties` must not contain the geometry column.
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

} // namespace geopackage
} // namespace ngen
#endif // NGEN_GEOPACKAGE_H
