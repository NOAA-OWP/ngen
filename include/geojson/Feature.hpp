#ifndef GEOJSON_FEATURE_H
#define GEOJSON_FEATURE_H

#include "JSONGeometry.hpp"
#include "JSONProperty.hpp"

#include <ostream>
#include <exception>

#include <boost/property_tree/ptree.hpp>

namespace geojson {
    enum class FeatureType {
        Point,
        LineString,
        Polygon,
        MultiPoint,
        MultiLineString,
        MultiPolygon,
        GeometryCollection
    };

    std::string get_featuretype_name(FeatureType &feature) {
        switch(feature) {
            case FeatureType::Point:
                return "Point";
            case FeatureType::LineString:
                return "LineString";
            case FeatureType::Polygon:
                return "Polygon";
            case FeatureType::MultiPoint:
                return "MultiPoint";
            case FeatureType::MultiLineString:
                return "MultiLineString";
            case FeatureType::MultiPolygon:
                return "MultiPolygon";
            case FeatureType::GeometryCollection:
                return "GeometryCollection";
            default:
                throw std::logic_error("The passed in FeatureType was invalid");
        }
    }

    typedef std::map<std::string, JSONProperty> property_map;

    class Feature {
        public:
            Feature(
                JSONGeometry new_geometry,
                std::vector<double> new_bounding_box,
                property_map new_properties
            ) {
                switch(new_geometry.get_type()) {
                    case JSONGeometryType::Point:
                        type = FeatureType::Point;
                        break;
                    case JSONGeometryType::LineString:
                        type = FeatureType::LineString;
                        break;
                    case JSONGeometryType::Polygon:
                        type = FeatureType::Polygon;
                        break;
                    case JSONGeometryType::MultiPoint:
                        type = FeatureType::MultiPoint;
                        break;
                    case JSONGeometryType::MultiLineString:
                        type = FeatureType::MultiLineString;
                        break;
                    case JSONGeometryType::MultiPolygon:
                        type = FeatureType::MultiPolygon;
                        break;
                }

                bounding_box = new_bounding_box;
                properties = new_properties;
            }

            Feature(std::vector<JSONGeometry> new_geometry_collection,
                std::vector<double> new_bounding_box,
                property_map new_properties = property_map()
            );

            Feature(boost::property_tree::ptree &tree) {
                boost::property_tree::ptree geom = tree.get_child("geometry");
                std::string geometry_type = geom.get<std::string>("type");
                
            }

            static Feature of_point(
                double x,
                double y,
                std::vector<double> new_bounding_box,
                property_map new_properties = property_map()
            ) {
                JSONGeometry geometric_point = JSONGeometry::of_point(x, y);
                return Feature(geometric_point, new_bounding_box, new_properties);
            }

            static Feature of_linestring(
                std::vector<double> coordinates,
                std::vector<double> new_bounding_box,
                property_map new_properties = property_map()
            ) {
                
            }

            Feature(const Feature &feature) {
                properties = feature.get_properties();

                type = feature.get_type();
                bounding_box = feature.get_bounding_box();

                if (feature.get_type() == FeatureType::GeometryCollection) {
                    for(JSONGeometry geometry : feature.get_geometry_collection()) {
                        geometry_collection.push_back(geometry);
                    }
                }
                else {
                    geometry = feature.get_geometry();
                }
            }

            virtual ~Feature(){};

            FeatureType get_type() const {
                return type;
            }

            JSONProperty get_property(const std::string& key) const;

            std::vector<JSONGeometry> get_geometry_collection() const {
                if (type == FeatureType::GeometryCollection) {
                    return geometry_collection;
                }

                std::string message = "This feature is not a collection of geometry objects.";
                throw std::runtime_error(message);
            }

            JSONGeometry *get_geometry() const {
                if (type == FeatureType::GeometryCollection) {
                    std::string message = "This feature does not represent an individual geometry.";
                    throw std::runtime_error(message);
                }

                return geometry;
            }

            std::vector<double> get_bounding_box() const {
                return bounding_box;
            }

            property_map get_properties() const {
                return properties;
            }

        protected:
            FeatureType type;
            std::vector<JSONGeometry> geometry_collection;
            std::vector<double> bounding_box;

        private:
            JSONGeometry *geometry;
            property_map properties;

    };
}
#endif // GEOJSON_FEATURE_H