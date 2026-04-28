#include "geopackage.hpp"
#include "JSONProperty.hpp"

#include <stdexcept>

// Points don't have a bounding box, so we can say its bbox is itself
inline void build_point_bbox(const geojson::geometry& geom, std::vector<double>& bbox)
{
    const auto& pt = boost::get<geojson::coordinate_t>(geom);
    bbox[0] = pt.get<0>();
    bbox[1] = pt.get<1>();
    bbox[2] = pt.get<0>();
    bbox[3] = pt.get<1>();
}

geojson::Feature ngen::geopackage::build_feature(
  const ngen::sqlite::database::iterator& row,
  const std::string& id_col,
  const std::string& geom_col,
  ngen::geopackage::HydrofabricVersion version,
  const std::unordered_map<std::string, std::string>* divide_toid_lookup
)
{
    std::vector<double> bounding_box(4);
    std::string id                   = row.get<std::string>(id_col);
    geojson::PropertyMap properties  = build_properties(row, geom_col);
    geojson::geometry geometry       = build_geometry(row, geom_col, bounding_box);

    // v3.0 renamed nexus.id -> nexus.nexus_id and nexus.toid -> nexus.nexus_toid.
    // Downstream consumers still key on "id" / "toid", so alias the v3.0 columns
    // into those names. The originals remain in the map (additive) so any future
    // consumer that prefers the schema names keeps working.
    if (version == ngen::geopackage::HydrofabricVersion::V3_0 && id_col == "nexus_id") {
        auto it_nid = properties.find("nexus_id");
        if (it_nid != properties.end()) {
            properties.emplace("id", geojson::JSONProperty("id", it_nid->second));
        }
        auto it_ntoid = properties.find("nexus_toid");
        if (it_ntoid != properties.end()) {
            properties.emplace("toid", geojson::JSONProperty("toid", it_ntoid->second));
        }
        if (properties.count("id") == 0) {
            throw std::runtime_error(
                "v3.0 nexus row missing required 'nexus_id' column"
            );
        }
        if (properties.count("toid") == 0) {
            throw std::runtime_error(
                "v3.0 nexus row missing required 'nexus_toid' column"
            );
        }
        if (id.empty()) {
            throw std::runtime_error("v3.0 nexus row has empty 'id' value");
        }
    }

    // v3.0 divides carry flowpath_id as the foreign key into flowpaths.
    // build_properties already copies it verbatim (it is a non-geometry
    // column); guard that invariant here so the toid-synthesis lookup
    // below can rely on it. v2.2 divides are intentionally not asserted:
    // flowpath_id is a v3.0 column.
    if (version == ngen::geopackage::HydrofabricVersion::V3_0 && id_col == "divide_id") {
        if (properties.count("flowpath_id") == 0) {
            throw std::runtime_error(
                "v3.0 divides row missing required 'flowpath_id' column"
            );
        }
        if (id.empty()) {
            throw std::runtime_error("v3.0 divides row has empty 'id' value");
        }

        // v3.0 divides have no native toid column: synthesize it by looking
        // up this divide's id in the precomputed divide_id -> flowpath_toid
        // cache (built in read.cpp from divides JOIN flowpaths). If the
        // cache is null or the lookup misses (e.g., flowpath_id was null
        // or the join did not resolve), leave "toid" unset — that matches
        // v2.2 terminal-divide semantics and lets read.cpp count the
        // unlinked divides once, after the full feature build.
        if (divide_toid_lookup != nullptr) {
            auto it = divide_toid_lookup->find(id);
            if (it != divide_toid_lookup->end()) {
                properties.emplace(
                    "toid",
                    geojson::JSONProperty("toid", it->second)
                );
            }
        }
    }

    // Convert variant type (0-based) to FeatureType
    const auto wkb_type = static_cast<geojson::FeatureType>(geometry.which() + 1);

    switch(wkb_type) {
        case geojson::FeatureType::Point:
            build_point_bbox(geometry, bounding_box);
            return std::make_shared<geojson::PointFeature>(
                boost::get<geojson::coordinate_t>(geometry),
                id,
                properties,
                bounding_box
            );
        case geojson::FeatureType::LineString:
            return std::make_shared<geojson::LineStringFeature>(
                boost::get<geojson::linestring_t>(geometry),
                id,
                properties,
                bounding_box
            );
        case geojson::FeatureType::Polygon:
            return std::make_shared<geojson::PolygonFeature>(
                boost::get<geojson::polygon_t>(geometry),
                id,
                properties,
                bounding_box
            );
        case geojson::FeatureType::MultiPoint:
            return std::make_shared<geojson::MultiPointFeature>(
                boost::get<geojson::multipoint_t>(geometry),
                id,
                properties,
                bounding_box
            );
        case geojson::FeatureType::MultiLineString:
            return std::make_shared<geojson::MultiLineStringFeature>(
                boost::get<geojson::multilinestring_t>(geometry),
                id,
                properties,
                bounding_box
            );
        case geojson::FeatureType::MultiPolygon:
            return std::make_shared<geojson::MultiPolygonFeature>(
                boost::get<geojson::multipolygon_t>(geometry),
                id,
                properties,
                bounding_box
            );
        case geojson::FeatureType::GeometryCollection:
            return std::make_shared<geojson::CollectionFeature>(
                std::vector<geojson::geometry>{geometry},
                id,
                properties,
                bounding_box
            );
        default:
            throw std::runtime_error("invalid WKB feature type. Received: " + std::to_string(geometry.which() + 1));
    }
}
