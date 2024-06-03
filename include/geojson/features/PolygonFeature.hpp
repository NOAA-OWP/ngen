#ifndef GEOJSON_POLYGON_FEATURE_H
#define GEOJSON_POLYGON_FEATURE_H

#include "FeatureBase.hpp"
#include <FeatureVisitor.hpp>
#include <JSONGeometry.hpp>

#include <string>
#include <vector>
#include <map>
#include <exception>

namespace geojson {
    /**
     * Represents a feature based around a polygon defining an area
     */
    class PolygonFeature : public FeatureBase {
        public:
            /**
             * Copy Constructor
             */
            PolygonFeature(const PolygonFeature& feature) : FeatureBase(feature) {}

            /**
             * Verbose constructor
             * 
             * @p
             * @param polygon A boost geometry definition for a polygon
             * @param new_id The identifier that may be used to find this feature
             * @param new_properties A mapping of std::string to JSONProperties defining basic properties of the feature
             * @param new_bounding_box A definition for all of the point bounds for the shape of the feature
             * @param upstream_features A collection of features that appear upstream from this feature
             * @param downstream_features A collection of features that appear downstream from this feature
             * @param members A mapping of foreign member values between strings and JSONProperties
             */
            PolygonFeature(
                polygon_t polygon,
                std::string new_id = "",
                PropertyMap new_properties = PropertyMap(),
                std::vector<double> new_bounding_box = std::vector<double>(),
                std::vector<FeatureBase*> upstream_features = std::vector<FeatureBase*>(),
                std::vector<FeatureBase*> downstream_features = std::vector<FeatureBase*>(),
                PropertyMap members = std::map<std::string, JSONProperty>()
            ) : FeatureBase(std::move(new_id), std::move(new_properties), std::move(new_bounding_box), std::move(upstream_features), std::move(downstream_features), std::move(members)) {
                this->geom = std::move(polygon);
                this->type = geojson::FeatureType::Polygon;
            }

            /**
             * Returns a properly typed geometry for the feature
             * 
             * @return The underlying polygon for this feature
             */
            polygon_t geometry() const {
                return boost::get<polygon_t>(this->geom);
            }

            /**
             * Runs a visitor function on this feature
             */
            void visit(FeatureVisitor& visitor) override {
                visitor.visit(this);
            }
    };
}

#endif // GEOJSON_POLYGON_FEATURE_H
