#ifndef NGEN_GEOPACKAGE_H
#define NGEN_GEOPACKAGE_H

#include "FeatureCollection.hpp"
#include "SQLite.hpp"

namespace geopackage {

geojson::geometry build_geometry(
    const sqlite_iter& row,
    const std::string& geom_col,
    std::vector<double>& bounding_box
);

geojson::PropertyMap build_properties(
    const sqlite_iter& row,
    const std::string& geom_col
);

geojson::Feature build_feature(
    const sqlite_iter& row,
    const std::string& geom_col
);

std::shared_ptr<geojson::FeatureCollection> read(
    const std::string& gpkg_path,
    const std::string& layer,
    const std::vector<std::string>& ids
);

} // namespace geopackage
#endif // NGEN_GEOPACKAGE_H