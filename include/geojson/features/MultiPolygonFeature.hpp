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
            ) : FeatureBase(std::move(new_id), std::move(new_properties), std::move(new_bounding_box), std::move(upstream_features), std::move(downstream_features), std::move(members)) {
                this->geom = std::move(multipolygon);
                this->type = geojson::FeatureType::MultiPolygon;
            }

            multipolygon_t geometry() const {
                return boost::get<multipolygon_t>(this->geom);
            }

            void visit(FeatureVisitor& visitor) override {
                visitor.visit(this);
            }
    };
}

#endif // GEOJSON_MULTIPOLYGON_FEATURE_H
