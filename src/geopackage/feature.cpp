#include "geopackage.hpp"

// Points don't have a bounding box, so we can say its bbox is itself
inline void build_point_bbox(const geojson::geometry& geom, std::vector<double>& bbox)
{
    const auto& pt = std::get<geojson::coordinate_t>(geom);
    bbox[0] = pt.get<0>();
    bbox[1] = pt.get<1>();
    bbox[2] = pt.get<0>();
    bbox[3] = pt.get<1>();
}

geojson::Feature ngen::geopackage::build_feature(
  const ngen::sqlite::database::iterator& row,
  const std::string& id,
  const std::string& geom_col,
  geojson::PropertyMap properties
)
{
    std::vector<double> bounding_box(4);
    geojson::geometry geometry = build_geometry(row, geom_col, bounding_box);

    // Convert variant type (0-based) to FeatureType
    const auto wkb_type = static_cast<geojson::FeatureType>(geometry.index() + 1);

    switch(wkb_type) {
        case geojson::FeatureType::Point:
            build_point_bbox(geometry, bounding_box);
            return std::make_shared<geojson::PointFeature>(
                std::get<geojson::coordinate_t>(geometry),
                id,
                properties,
                bounding_box
            );
        case geojson::FeatureType::LineString:
            return std::make_shared<geojson::LineStringFeature>(
                std::get<geojson::linestring_t>(geometry),
                id,
                properties,
                bounding_box
            );
        case geojson::FeatureType::Polygon:
            return std::make_shared<geojson::PolygonFeature>(
                std::get<geojson::polygon_t>(geometry),
                id,
                properties,
                bounding_box
            );
        case geojson::FeatureType::MultiPoint:
            return std::make_shared<geojson::MultiPointFeature>(
                std::get<geojson::multipoint_t>(geometry),
                id,
                properties,
                bounding_box
            );
        case geojson::FeatureType::MultiLineString:
            return std::make_shared<geojson::MultiLineStringFeature>(
                std::get<geojson::multilinestring_t>(geometry),
                id,
                properties,
                bounding_box
            );
        case geojson::FeatureType::MultiPolygon:
            return std::make_shared<geojson::MultiPolygonFeature>(
                std::get<geojson::multipolygon_t>(geometry),
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
            throw std::runtime_error("invalid WKB feature type. Received: " + std::to_string(geometry.index() + 1));
    }
}

geojson::Feature ngen::geopackage::build_feature_without_geometry(
  const std::string& id,
  geojson::PropertyMap properties
)
{
    return std::make_shared<geojson::CollectionFeature>(
        std::vector<geojson::geometry>{},
        id,
        std::move(properties),
        std::vector<double>{}
    );
}
