#ifndef GEOJSON_LINESTRING_FEATURE_H
#define GEOJSON_LINESTRING_FEATURE_H

#include "FeatureBase.hpp"
#include <FeatureVisitor.hpp>
#include <JSONGeometry.hpp>

#include <string>
#include <vector>
#include <map>
#include <exception>

namespace geojson {
    class LineStringFeature : public FeatureBase {
        public:
            LineStringFeature(const FeatureBase& feature) : FeatureBase(feature) {}

            LineStringFeature(const LineStringFeature& feature) : FeatureBase(feature) {
                this->geom = feature.geometry();
                this->type = feature.get_type();
            }

            LineStringFeature(
                linestring_t linestring,
                std::string new_id = "",
                PropertyMap new_properties = PropertyMap(),
                std::vector<double> new_bounding_box = std::vector<double>(),
                std::vector<FeatureBase*> upstream_features = std::vector<FeatureBase*>(),
                std::vector<FeatureBase*> downstream_features = std::vector<FeatureBase*>(),
                std::map<std::string, JSONProperty> members = std::map<std::string, JSONProperty>()
            ) : FeatureBase(new_id, new_properties, new_bounding_box, upstream_features, downstream_features, members) {
                this->geom = linestring;
              this->type = geojson::FeatureType::LineString;
            }

            linestring_t geometry()  const {
                return boost::get<linestring_t>(this->geom);
            }

            void visit(FeatureVisitor& visitor) override {
                visitor.visit(this);
            }
    };
}

#endif // GEOJSON_LINESTRING_FEATURE_H
