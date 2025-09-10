#ifndef GEOJSON_SENTINEL_FEATURE_H
#define GEOJSON_SENTINEL_FEATURE_H

#include "FeatureBase.hpp"
#include <FeatureVisitor.hpp>
#include <JSONGeometry.hpp>

#include <string>
#include <vector>
#include <map>
#include <exception>

namespace geojson {

    class SentinelFeature : public FeatureBase {
        public:
            SentinelFeature(
                std::string new_id
            ) : FeatureBase(std::move(new_id)) {
                this->type = geojson::FeatureType::Sentinel;
            }

            void visit(FeatureVisitor& visitor) override {
                visitor.visit(this);
            }
    };
}

#endif // GEOJSON_SENTINEL_FEATURE_H
