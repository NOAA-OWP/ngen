#ifndef GEOJSON_MULTILINESTRING_FEATURE_H
#define GEOJSON_MULTILINESTRING_FEATURE_H

#include "FeatureBase.hpp"
#include <FeatureVisitor.hpp>
#include <JSONGeometry.hpp>

#include <string>
#include <vector>
#include <map>
#include <exception>

namespace geojson {
    class MultiLineStringFeature : public FeatureBase {
        public:
            MultiLineStringFeature(const FeatureBase& feature) : FeatureBase(feature) {}

            MultiLineStringFeature(const MultiLineStringFeature& feature) : FeatureBase(feature) {
                this->geom = feature.geometry();
                this->type = feature.get_type();
            }

            MultiLineStringFeature(
                multilinestring_t multilinestring,
                std::string new_id = "",
                PropertyMap new_properties = PropertyMap(),
                std::vector<double> new_bounding_box = std::vector<double>(),
                std::vector<FeatureBase*> upstream_features = std::vector<FeatureBase*>(),
                std::vector<FeatureBase*> downstream_features = std::vector<FeatureBase*>(),
                std::map<std::string, JSONProperty> members = std::map<std::string, JSONProperty>()
            ) : FeatureBase(new_id, new_properties, new_bounding_box, upstream_features, downstream_features, members) {
                this->geom = multilinestring;
                this->type = geojson::FeatureType::MultiLineString;
            }

            multilinestring_t geometry() const {
                return boost::get<multilinestring_t>(this->geom);
            }

            void visit(FeatureVisitor& visitor) override {
                visitor.visit(this);
            }
    };
}

#endif // GEOJSON_MULTILINESTRING_FEATURE_H
