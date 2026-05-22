#ifndef GEOJSON_MULTIPOINT_FEATURE_H
#define GEOJSON_MULTIPOINT_FEATURE_H

#include "FeatureBase.hpp"
#include <FeatureVisitor.hpp>
#include <JSONGeometry.hpp>

#include <string>
#include <vector>
#include <map>
#include <exception>

namespace geojson {
    class MultiPointFeature : public FeatureBase {
        public:
            MultiPointFeature(const FeatureBase& feature) : FeatureBase(feature) {}

            MultiPointFeature(const MultiPointFeature& feature) : FeatureBase(feature) {
                this->geom = feature.geometry();
                this->type = feature.get_type();
            }

            MultiPointFeature(
                multipoint_t multipoint,
                std::string new_id = "",
                PropertyMap new_properties = PropertyMap(),
                std::vector<double> new_bounding_box = std::vector<double>(),
                std::vector<FeatureBase*> upstream_features = std::vector<FeatureBase*>(),
                std::vector<FeatureBase*> downstream_features = std::vector<FeatureBase*>(),
                std::map<std::string, JSONProperty> members = std::map<std::string, JSONProperty>()
            ) : FeatureBase(new_id, new_properties, new_bounding_box, upstream_features, downstream_features, members) {
                this->geom = multipoint;
                this->type = geojson::FeatureType::MultiPoint;
            }

            multipoint_t geometry() const  {
                return boost::get<multipoint_t>(this->geom);
            }

            void visit(FeatureVisitor& visitor) override {
                visitor.visit(this);
            }
    };
}

#endif // GEOJSON_MULTIPOINT_FEATURE_H
