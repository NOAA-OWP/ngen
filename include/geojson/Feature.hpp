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
        None,
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
            );

            Feature(std::vector<JSONGeometry> new_geometry_collection,
                std::vector<double> new_bounding_box = std::vector<double>(),
                property_map new_properties = property_map()
            );

            Feature(boost::property_tree::ptree &tree);

            Feature(const Feature &feature);

            virtual ~Feature(){};

            std::vector<double> get_bounding_box() const;

            std::vector<JSONGeometry> get_geometry_collection() const;

            JSONGeometry get_geometry() const;

            property_map get_properties() const;

            JSONProperty get_property(const std::string& key) const;

            FeatureType get_type() const;

            std::vector<std::string> keys();

        protected:
            FeatureType type;
            std::vector<JSONGeometry> geometry_collection;
            std::vector<double> bounding_box;

        private:
            JSONGeometry geometry;
            property_map properties;

    };
}
#endif // GEOJSON_FEATURE_H