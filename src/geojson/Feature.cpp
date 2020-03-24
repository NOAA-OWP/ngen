#include "Feature.hpp"

Feature::Feature(std::vector<JSONGeometry> new_geometry_collection,
        double *new_bounding_box,
        property_map new_properties
    ) {
    type = FeatureType::GeometryCollection;
    geometry_collection = new_geometry_collection;
    properties = new_properties;   
}

JSONProperty Feature::get_property(std::string key) const {
    return properties.at(key);
}