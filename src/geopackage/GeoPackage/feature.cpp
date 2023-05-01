#include "GeoPackage.hpp"

const geojson::FeatureType feature_type_map(const std::string& g)
{
    if (g == "POINT") return geojson::FeatureType::Point;
    if (g == "LINESTRING") return geojson::FeatureType::LineString;
    if (g == "POLYGON") return geojson::FeatureType::Polygon;
    if (g == "MULTIPOINT") return geojson::FeatureType::MultiPoint;
    if (g == "MULTILINESTRING") return geojson::FeatureType::MultiLineString;
    if (g == "MULTIPOLYGON") return geojson::FeatureType::MultiPolygon;
    return geojson::FeatureType::GeometryCollection;
}

geojson::Feature geopackage::build_feature(
  const sqlite_iter& row,
  const std::string& geom_type,
  const std::string& geom_col
)
{
    const auto type                  = feature_type_map(geom_type);
    const auto id                    = row.get<std::string>("id");
    geojson::PropertyMap properties  = std::move(build_properties(row, geom_col));
    geojson::geometry geometry       = std::move(build_geometry(row, type, geom_col));
    const auto geometry_bbox = bg::return_envelope<bg::model::box<geojson::coordinate_t>>(geometry);
    const std::vector<double> bounding_box = {
        geometry_bbox.min_corner().get<0>(),
        geometry_bbox.min_corner().get<1>(),
        geometry_bbox.max_corner().get<0>(),
        geometry_bbox.max_corner().get<1>()
    };

    switch(type) {
        case geojson::FeatureType::Point:
            return std::make_shared<geojson::PointFeature>(geojson::PointFeature(
                boost::get<geojson::coordinate_t>(geometry),
                id,
                properties,
                bounding_box,
                std::vector<geojson::FeatureBase*>(),
                std::vector<geojson::FeatureBase*>(),
                {}
            ));
        case geojson::FeatureType::LineString:
            return std::make_shared<geojson::LineStringFeature>(geojson::LineStringFeature(
                boost::get<geojson::linestring_t>(geometry),
                id,
                properties,
                bounding_box,
                std::vector<geojson::FeatureBase*>(),
                std::vector<geojson::FeatureBase*>(),
                {}
            ));
        case geojson::FeatureType::Polygon:
            return std::make_shared<geojson::PolygonFeature>(geojson::PolygonFeature(
                boost::get<geojson::polygon_t>(geometry),
                id,
                properties,
                bounding_box,
                std::vector<geojson::FeatureBase*>(),
                std::vector<geojson::FeatureBase*>(),
                {}
            ));
        case geojson::FeatureType::MultiPoint:
            return std::make_shared<geojson::MultiPointFeature>(geojson::MultiPointFeature(
                boost::get<geojson::multipoint_t>(geometry),
                id,
                properties,
                bounding_box,
                std::vector<geojson::FeatureBase*>(),
                std::vector<geojson::FeatureBase*>(),
                {}
            ));
        case geojson::FeatureType::MultiLineString:
            return std::make_shared<geojson::MultiLineStringFeature>(geojson::MultiLineStringFeature(
                boost::get<geojson::multilinestring_t>(geometry),
                id,
                properties,
                bounding_box,
                std::vector<geojson::FeatureBase*>(),
                std::vector<geojson::FeatureBase*>(),
                {}
            ));
        case geojson::FeatureType::MultiPolygon:
            return std::make_shared<geojson::MultiPolygonFeature>(geojson::MultiPolygonFeature(
                boost::get<geojson::multipolygon_t>(geometry),
                id,
                properties,
                bounding_box,
                std::vector<geojson::FeatureBase*>(),
                std::vector<geojson::FeatureBase*>(),
                {}
            ));
        default:
            return std::make_shared<geojson::CollectionFeature>(geojson::CollectionFeature(
                std::vector<geojson::geometry>{geometry},
                id,
                properties,
                bounding_box,
                std::vector<geojson::FeatureBase*>(),
                std::vector<geojson::FeatureBase*>(),
                {}
            ));
    }
}