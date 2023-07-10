#ifndef NGEN_GEOPACKAGE_H
#define NGEN_GEOPACKAGE_H

#include "FeatureCollection.hpp"
#include "NGen_SQLite.hpp"

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
    const sqlite_iter& row,
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
    const sqlite_iter& row,
    const std::string& geom_col
);

/**
 * Build a feature from a GPKG table row
 * 
 * @param[in] row SQLite iterator at the row to build a feature from
 * @param[in] geom_col Name of geometry column containing GPKG WKB
 * @return geojson::Feature Feature containing geometry and properties from the given row
 */
geojson::Feature build_feature(
    const sqlite_iter& row,
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

} // namespace geopackage
#endif // NGEN_GEOPACKAGE_H
