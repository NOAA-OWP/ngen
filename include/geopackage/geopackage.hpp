#ifndef NGEN_GEOPACKAGE_H
#define NGEN_GEOPACKAGE_H

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
 * Build a feature from a GPKG table row
 * 
 * @param[in] row SQLite iterator at the row to build a feature from
 * @param[in] geom_col Name of geometry column containing GPKG WKB
 * @return geojson::Feature Feature containing geometry and properties from the given row
 */
geojson::Feature build_feature(
    const ngen::sqlite::database::iterator& row,
    const std::string& id_col,
    const std::string& geom_col
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

/**
 * Join the columns of a GeoPackage attribute table onto the features of a collection.
 *
 * Rows are matched to features by comparing @p key_column against feature IDs, and each matched
 * feature gains a property `<prefix>.<column>` per non-key column holding an integer, real or text
 * value. Cells of any other type, SQL NULL among them, yield no property. Rows keyed to a feature
 * the collection does not hold are ignored, since under partitioning most of a table's rows belong
 * to other ranks.
 *
 * @param[in,out] collection Features to join onto, mutated in place
 * @param[in] gpkg_path Path to the GPKG file holding the attribute table
 * @param[in] table Name of the attribute table within the GPKG file
 * @param[in] key_column Column of @p table whose values are matched against feature IDs
 * @param[in] prefix Namespace the joined columns are published under
 * @param[in] required When true, a feature with no matching row is an error rather than a warning
 * @throw std::runtime_error if @p table or @p key_column does not exist, if two rows of @p table
 *        are keyed to the same feature, if a composed property name is already held by a feature,
 *        or if a feature has no matching row while @p required is true
 */
void join_attributes(
    geojson::FeatureCollection& collection,
    const std::string& gpkg_path,
    const std::string& table,
    const std::string& key_column,
    const std::string& prefix,
    bool required
);

} // namespace geopackage
} // namespace ngen
#endif // NGEN_GEOPACKAGE_H
