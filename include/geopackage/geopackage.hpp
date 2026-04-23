#ifndef NGEN_GEOPACKAGE_H
#define NGEN_GEOPACKAGE_H

#include <string>
#include <unordered_map>

#include "FeatureCollection.hpp"
#include "HydrofabricVersion.hpp"
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
 * Build a feature from a GPKG table row
 *
 * @param[in] row SQLite iterator at the row to build a feature from
 * @param[in] id_col Name of the column to use as the feature id
 * @param[in] geom_col Name of geometry column containing GPKG WKB
 * @param[in] version Detected hydrofabric version (drives v3.0-specific
 *            property aliasing for the nexus/divides layers; ignored for
 *            non-hydrofabric GPKGs, where V2_2 is passed as a benign default)
 * @param[in] divide_toid_lookup Optional cache mapping v3.0 divide_id ->
 *            flowpath_toid. When non-null and the row is a v3.0 divide, the
 *            built feature's "toid" property is populated from the cache.
 *            Missing keys leave "toid" unset, matching v2.2 terminal-divide
 *            behavior. Pass nullptr for all non-v3.0-divides code paths.
 * @return geojson::Feature Feature containing geometry and properties from the given row
 */
geojson::Feature build_feature(
    const ngen::sqlite::database::iterator& row,
    const std::string& id_col,
    const std::string& geom_col,
    HydrofabricVersion version,
    const std::unordered_map<std::string, std::string>* divide_toid_lookup = nullptr
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
