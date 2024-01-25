#pragma once

#include "Driver.hpp"
#include <boost/json.hpp>
#include <utilities/Feature.hpp>
#include <utilities/FeatureCollection.hpp>
#include <utilities/traits.hpp>

namespace ngen {

auto tag_invoke(
    const boost::json::value_to_tag<ngen::FeatureCollection>& /*unused*/,
    const boost::json::value& value
) -> ngen::FeatureCollection
{
    const auto& object = value.as_object();

    if (!(object.contains("type") && object.contains("features"))) {
        throw std::runtime_error{"Input is not GeoJSON."};
    }

    if (object.at("type") != "FeatureCollection") {
        throw std::runtime_error{"Input does not contain a FeatureCollection"};
    }

    const boost::json::array& features = object.at("features").as_array();

    FeatureCollection collection;
    collection.reserve(features.size());
    for (const auto& feature : features) {
        collection.emplace_back(boost::json::value_to<ngen::Feature>(feature));
    }

    return collection;
}

auto tag_invoke(
    const boost::json::value_to_tag<ngen::Feature>& /*unused*/,
    const boost::json::value& value
) -> ngen::Feature
{
    const auto& object = value.as_object();

    if (!object.contains("geometry")) {
        throw std::runtime_error("Object does not have geometry");
    }

    Feature feature;
    feature.geometry() = boost::json::value_to<ngen::geometry>(object.at("geometry"));

    for (const auto& property : object.at("properties").as_object()) {
        feature[property.key()] = property.value();
    }

    return feature;
}

auto tag_invoke(
    const boost::json::value_to_tag<ngen::geometry>& /* unused */,
    const boost::json::value& value
) -> ngen::geometry
{
    const auto& object = value.as_object();

    if (!(object.contains("type") && object.contains("coordinates"))) {
        throw std::runtime_error("Object does not have either type, coordinates, or both");
    }

    const boost::json::string& type = object.at("type").as_string();
    const auto& coordinates = object.at("coordinates");

    if (type == "Point") {
        return boost::json::value_to<ngen::point>(coordinates);
    }

    if (type == "LineString") {
        return boost::json::value_to<ngen::linestring>(coordinates);
    }

    if (type == "Polygon") {
        return boost::json::value_to<ngen::polygon>(coordinates);
    }

    if (type == "MultiPoint") {
        return boost::json::value_to<ngen::multipoint>(coordinates);
    }

    if (type == "MultiLineString") {
        return boost::json::value_to<ngen::multilinestring>(coordinates);
    }

    if (type == "MultiPolygon") {
        return boost::json::value_to<ngen::multipolygon>(coordinates);
    }
    
    throw std::runtime_error{"Invalid GeoJSON Feature Type: `" + std::string{type.c_str()} + "`"};
}

auto tag_invoke(
    const boost::json::value_to_tag<ngen::point>& /* unused */,
    const boost::json::value& value
) -> ngen::point
{
    const auto& coordinates = value.as_array();
    return ngen::point{coordinates[0].as_double(), coordinates[1].as_double()};
}

auto tag_invoke(
    const boost::json::value_to_tag<ngen::linestring>& /* unused */,
    const boost::json::value& value
) -> ngen::linestring
{
    const auto& coordinates = value.as_array();
    ngen::linestring linestring;
    linestring.reserve(coordinates.size());
    for (const auto& position : coordinates) {
        linestring.push_back(boost::json::value_to<ngen::point>(position));
    }
    return linestring;
}

auto tag_invoke(
    const boost::json::value_to_tag<ngen::polygon>& /* unused */,
    const boost::json::value& value
) -> ngen::polygon
{
    const auto& coordinates = value.as_array();

    ngen::polygon polygon;

    const auto& outer_positions = coordinates[0].as_array();
    polygon.outer().reserve(outer_positions.size());
    for (const auto& position : outer_positions) {
        polygon.outer().push_back(boost::json::value_to<ngen::point>(position));
    }

    if (coordinates.size() > 1) {
        polygon.inners().reserve(coordinates.size() - 1);
        for (size_t i = 1; i < coordinates.size() - 1; i++) {
            const auto& inner_positions = coordinates[i].as_array();
            auto& inner_ring = polygon.inners()[i - 1];
            inner_ring.reserve(inner_positions.size());
            for (const auto& position : inner_positions) {
                inner_ring.push_back(boost::json::value_to<ngen::point>(position));
            }
        }
    }
    return polygon;
}

template<
    typename MultiType,
    std::enable_if_t<
        ngen::traits::is_same_to_any<
            MultiType,
            ngen::multipoint,
            ngen::multilinestring,
            ngen::multipolygon
        >::value,
        bool
    > = true
>
auto tag_invoke(
    const boost::json::value_to_tag<MultiType>& /* unused */,
    const boost::json::value& value
) -> MultiType
{
    MultiType multi;
    const auto& coordinates = value.as_array();
    multi.reserve(coordinates.size());
    for (const auto& position : coordinates) {
        multi.push_back(boost::json::value_to<typename MultiType::value_type>(position));
    }
    return multi;
}

struct GeoJSONDriver : public Driver<FeatureCollection> {
    FeatureCollection read(const std::string& input) override {
        const boost::json::value document = boost::json::parse(input);
        return boost::json::value_to<ngen::FeatureCollection>(document);
    }
};

} // namespace ngen
