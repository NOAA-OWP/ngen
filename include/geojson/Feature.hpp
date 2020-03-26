#ifndef GEOJSON_FEATURE_H
#define GEOJSON_FEATURE_H

#include "JSONGeometry.hpp"
#include "JSONProperty.hpp"

#include <memory>
#include <ostream>
#include <exception>
#include <string>

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

    typedef std::map<std::string, JSONProperty> property_map;

    class Feature {
        public:
            Feature(
                JSONGeometry &new_geometry,
                std::vector<double> new_bounding_box = std::vector<double>(),
                property_map new_properties = property_map()
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

                geometry = new_geometry;
                bounding_box = new_bounding_box;
                properties = new_properties;
            }

            Feature(std::vector<JSONGeometry> new_geometry_collection,
                std::vector<double> new_bounding_box = std::vector<double>(),
                property_map new_properties = property_map()
            )  {
                type = FeatureType::GeometryCollection;
                geometry_collection = new_geometry_collection;
                properties = new_properties;   
            }

            Feature(boost::property_tree::ptree &tree) {
                if (tree.count("geometry") > 0) {
                    boost::property_tree::ptree geom = tree.get_child("geometry");
                    geometry = JSONGeometry::from_ptree(geom);

                    switch(geometry.get_type()) {
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
                }
                else if (tree.count("geometries") > 0) {
                    type = FeatureType::GeometryCollection;
                    for (auto &geom : tree.get_child("geometries")) {
                        geometry_collection.push_back(JSONGeometry::from_ptree(geom.second));
                    }
                }

                if (tree.count("bbox") > 0) {
                    for (auto &value : tree.get_child("bbox")) {
                        bounding_box.push_back(std::stod(value.second.data()));
                    }
                } 
                
                if (tree.count("properties") > 0) {
                    for (auto &property : tree.get_child("properties")) {
                        // TODO: Add handling for nested objects by determining if property.second is another ptree
                        std::string key = property.first.data();
                        std::string value = property.second.data();

                        bool is_numeric = true;
                        bool is_real = true;
                        bool decimal_already_hit = false;

                        if (property.second.data() == "true") {
                            properties.emplace(key, JSONProperty(key, true));
                            continue;
                        }
                        else if (property.second.data() == "false") {
                            properties.emplace(key, JSONProperty(key, false));
                            continue;
                        }

                        for(int character_index = 0; character_index < value.length(); character_index++) {
                            char character = value[character_index];

                            // If the first character is a '0' or isn't a digit, the whole value cannot be a number
                            if (character_index == 0 && character == '0') {
                                is_numeric = false;
                            }
                            else if (character != '.' && !std::isdigit(character)) {
                                // If this character isn't a decimal point and isn't a digit, the whole value cannot be a number
                                is_numeric = false;
                                is_real = false;
                                break;
                            }
                            else if (character == '.' && decimal_already_hit) {
                                // If this character is a decimal point, but we've already seen one, the whole value cannot be a number
                                is_real = false;
                                break;
                            }
                            else if (character == '.') {
                                // If a decimal point is seen, the whole value cannot be an integer
                                is_numeric = false;
                                decimal_already_hit = true;
                            }
                        }

                        // If the value can be represented as a whole number, we want to go with that
                        if (is_numeric) {
                            properties.emplace(key, JSONProperty(key, std::stol(property.second.data())));
                        }
                        else if (is_real) {
                            // If this can be a floating point number, we want to use a floating point value
                            properties.emplace(key, JSONProperty(key, std::stod(property.second.data())));
                        }
                        else {
                            // Otherwise we'll store everything as a raw string
                            properties.emplace(key, JSONProperty(key, value));
                        }
                    }
                }
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

            JSONProperty get_property(std::string key) const {
                return properties.at(key);
            }

            std::vector<std::string> keys() {
                std::vector<std::string> property_keys;

                for (auto &pair : properties) {
                    property_keys.push_back(pair.first());
                }

                return property_keys;
            }

            std::vector<JSONGeometry> get_geometry_collection() const {
                if (type == FeatureType::GeometryCollection) {
                    return geometry_collection;
                }

                std::string message = "This feature is not a collection of geometry objects.";
                throw std::runtime_error(message);
            }

            JSONGeometry get_geometry() const {
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
        private:
            FeatureType type;
            JSONGeometry geometry;
            std::vector<JSONGeometry> geometry_collection;

            property_map properties;
            std::vector<double> bounding_box;
    };
}
#endif // GEOJSON_FEATURE_H