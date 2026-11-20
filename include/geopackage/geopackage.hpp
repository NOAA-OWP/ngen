#ifndef NGEN_GEOPACKAGE_H
#define NGEN_GEOPACKAGE_H

#include <string>

#include "FeatureCollection.hpp"
#include "ngen_sqlite.hpp"

namespace ngen {
namespace geopackage {

/**
 * Validate a GeoPackage table name and return it as a quoted SQL identifier.
 *
 * Table names cannot be bound as statement parameters, so every path that interpolates one into a
 * statement funnels through here. SQLite's own internal tables are refused outright, as are names
 * made up entirely of characters outside the GeoPackage conventions; quoting then keeps the
 * statement well-formed, and the name inert, for everything that remains.
 *
 * @param[in] table Table name taken from a configuration file or the command line
 * @return std::string @p table wrapped in double quotes, ready to interpolate into a statement
 * @throw std::runtime_error if @p table names a SQLite internal table or holds no usable characters
 */
std::string quote_table_name(const std::string& table);

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
 * Convert one column of a GeoPackage table row into a JSON property.
 *
 * @param[in] row SQLite iterator at the row containing the column
 * @param[in] name Name of the column to read, which is also the key of the returned property
 * @param[in] type SQLite type of this row's value in that column
 * @return geojson::JSONProperty Property holding the column's value
 */
geojson::JSONProperty get_property(
    const ngen::sqlite::database::iterator& row,
    const std::string& name,
    int type
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
