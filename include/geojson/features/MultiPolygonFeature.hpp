#ifndef GEOJSON_MULTIPOLYGON_FEATURE_H
#define GEOJSON_MULTIPOLYGON_FEATURE_H

#include "FeatureBase.hpp"
#include <FeatureVisitor.hpp>
#include <JSONGeometry.hpp>

#include <string>
#include <vector>
#include <map>
#include <exception>

namespace geojson {
    class MultiPolygonFeature : public FeatureBase {
        public:
            MultiPolygonFeature(const FeatureBase& feature) : FeatureBase(feature) {}

            MultiPolygonFeature(const MultiPolygonFeature& feature) : FeatureBase(feature) {
                this->geom = feature.geometry();
                this->type = geojson::FeatureType::MultiPolygon;
            }

            MultiPolygonFeature(
                multipolygon_t multipolygon,
                std::string new_id = "",
                PropertyMap new_properties = PropertyMap(),
                std::vector<double> new_bounding_box = std::vector<double>(),
                std::vector<FeatureBase*> upstream_features = std::vector<FeatureBase*>(),
                std::vector<FeatureBase*> downstream_features = std::vector<FeatureBase*>(),
                std::map<std::string, JSONProperty> members = std::map<std::string, JSONProperty>()
            ) : FeatureBase(new_id, new_properties, new_bounding_box, upstream_features, downstream_features, members) {
                this->geom = multipolygon;
                this->type = geojson::FeatureType::MultiPolygon;
            }

            multipolygon_t geometry() const {
                return boost::get<multipolygon_t>(this->geom);
            }

            void visit(FeatureVisitor& visitor) {
                visitor.visit(this);
            }
    };
}

#endif // GEOJSON_MULTIPOLYGON_FEATURE_H