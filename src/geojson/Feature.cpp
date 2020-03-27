#include "Feature.hpp"

#include <utility>

using geojson::Feature;
using geojson::JSONGeometry;
using geojson::JSONProperty;

Feature::Feature(std::vector<geojson::JSONGeometry> new_geometry_collection,
                 std::vector<double> new_bounding_box,
                 property_map new_properties) {
    type = geojson::FeatureType::GeometryCollection;
    geometry_collection = std::move(new_geometry_collection);
    properties = std::move(new_properties);
    bounding_box = std::move(new_bounding_box);
}

JSONProperty Feature::get_property(const std::string& key) const {
    return properties.at(key);
}