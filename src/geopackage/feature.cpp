#include "GeoPackage.hpp"

geojson::Feature geopackage::build_feature(
  const sqlite_iter& row,
  const std::string& geom_col
)
{
    std::vector<double> bounding_box(4);
    const auto id                    = row.get<std::string>("id");
    geojson::PropertyMap properties  = build_properties(row, geom_col);
    geojson::geometry geometry       = build_geometry(row, geom_col, bounding_box);

    // Convert variant type (0-based) to FeatureType
    const auto wkb_type = geojson::FeatureType(geometry.which() + 1);

    // Points don't have a bounding box, so we can say its bbox is itself
    if (wkb_type == geojson::FeatureType::Point) {
        const auto& pt = boost::get<geojson::coordinate_t>(geometry);
        bounding_box[0] = pt.get<0>();
        bounding_box[1] = pt.get<1>();
        bounding_box[2] = pt.get<0>();
        bounding_box[3] = pt.get<1>();
    }
    
    switch(wkb_type) {
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