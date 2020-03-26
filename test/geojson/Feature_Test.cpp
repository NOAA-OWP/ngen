#include "gtest/gtest.h"
#include <Feature.hpp>
#include <JSONProperty.hpp>
#include <JSONGeometry.hpp>
#include <iostream>
#include <sstream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

class Feature_Test : public ::testing::Test {

    protected:


    Feature_Test() {

    }

    ~Feature_Test() override {

    }

    void SetUp() override {

    }

    void TearDown() override {

    }

};

TEST_F(Feature_Test, geometry_test) {
    double x = 102.0;
    double y = 0.5;
    geojson::JSONGeometry geometry = geojson::JSONGeometry::of_point(x, y);
    geojson::Feature feature(geometry);

    ASSERT_EQ(feature.get_type(), geojson::FeatureType::Point);

    ASSERT_EQ(feature.get_geometry().as_point().get<0>(), x);
    ASSERT_EQ(feature.get_geometry().as_point().get<1>(), y);
    ASSERT_EQ(feature.get_geometry().get_type(), geojson::JSONGeometryType::Point);

    ASSERT_EQ(feature.get_properties().size(), 0);
    ASSERT_EQ(feature.get_bounding_box().size(), 0);

    ASSERT_ANY_THROW(feature.get_property("doesnotexist"));

    std::vector<double> bounding_box{1.0, 2.0};

    feature = geojson::Feature(geometry, bounding_box);

    ASSERT_EQ(feature.get_geometry().as_point().get<0>(), x);
    ASSERT_EQ(feature.get_geometry().as_point().get<1>(), y);
    ASSERT_EQ(feature.get_geometry().get_type(), geojson::JSONGeometryType::Point);

    ASSERT_EQ(feature.get_properties().size(), 0);
    ASSERT_EQ(feature.get_bounding_box().size(), 2);
    ASSERT_EQ(feature.get_bounding_box()[0], 1.0);
    ASSERT_EQ(feature.get_bounding_box()[1], 2.0);

    ASSERT_ANY_THROW(feature.get_property("doesnotexist"));

    geojson::property_map properties{
        {"prop_0", geojson::JSONProperty("prop_0", 0)},
        {"prop_1", geojson::JSONProperty("prop_1", "1")},
        {"prop_2", geojson::JSONProperty("prop_2", false)},
        {"prop_3", geojson::JSONProperty("prop_3", 2.0)}
    };

    feature = geojson::Feature(geometry, bounding_box, properties);

    ASSERT_EQ(feature.get_geometry().as_point().get<0>(), x);
    ASSERT_EQ(feature.get_geometry().as_point().get<1>(), y);
    ASSERT_EQ(feature.get_geometry().get_type(), geojson::JSONGeometryType::Point);

    ASSERT_EQ(feature.get_properties().size(), 4);
    ASSERT_EQ(feature.get_bounding_box().size(), 2);
    ASSERT_EQ(feature.get_bounding_box()[0], 1.0);
    ASSERT_EQ(feature.get_bounding_box()[1], 2.0);

    ASSERT_EQ(feature.get_property("prop_0").as_natural_number(), 0);
    ASSERT_EQ(feature.get_property("prop_1").as_string(), "1");
    ASSERT_FALSE(feature.get_property("prop_2").as_boolean());
    ASSERT_EQ(feature.get_property("prop_3").as_real_number(), 2.0);
    ASSERT_ANY_THROW(feature.get_property("doesnotexist"));

    geojson::two_dimensional_coordinates two_dimensions {
        {1.0, 2.0},
        {3.0, 4.0},
        {5.0, 6.0}
    };

    geojson::three_dimensional_coordinates three_dimensions {
        {
            {1.0, 2.0},
            {3.0, 4.0},
            {5.0, 6.0}
        },
        {
            {7.0, 8.0},
            {9.0, 10.0},
            {11.0, 12.0}
        }
    };

    geojson::four_dimensional_coordinates four_dimensions {
        {
            {
                {1.0, 2.0},
                {3.0, 4.0},
                {5.0, 6.0}
            }
        },
        {
            {
                {7.0, 8.0},
                {9.0, 10.0},
                {11.0, 12.0}
            }
        }
    };

    geometry = geojson::JSONGeometry::of_linestring(two_dimensions);

    feature = geojson::Feature(geometry, bounding_box, properties);

    ASSERT_EQ(feature.get_geometry().get_type(), geojson::JSONGeometryType::LineString);
    ASSERT_EQ(feature.get_geometry().as_linestring().size(), 3);

    ASSERT_EQ(feature.get_properties().size(), 4);
    ASSERT_EQ(feature.get_bounding_box().size(), 2);
    ASSERT_EQ(feature.get_bounding_box()[0], 1.0);
    ASSERT_EQ(feature.get_bounding_box()[1], 2.0);

    ASSERT_EQ(feature.get_property("prop_0").as_natural_number(), 0);
    ASSERT_EQ(feature.get_property("prop_1").as_string(), "1");
    ASSERT_FALSE(feature.get_property("prop_2").as_boolean());
    ASSERT_EQ(feature.get_property("prop_3").as_real_number(), 2.0);
    ASSERT_ANY_THROW(feature.get_property("doesnotexist"));

    geometry = geojson::JSONGeometry::of_polygon(three_dimensions);

    feature = geojson::Feature(geometry, bounding_box, properties);

    ASSERT_EQ(feature.get_geometry().get_type(), geojson::JSONGeometryType::Polygon);
    ASSERT_EQ(feature.get_geometry().as_polygon().outer().size(), 3);
    ASSERT_EQ(feature.get_geometry().as_polygon().inners().size(), 1);
    ASSERT_EQ(feature.get_geometry().as_polygon().inners()[0].size(), 3);

    ASSERT_EQ(feature.get_properties().size(), 4);
    ASSERT_EQ(feature.get_bounding_box().size(), 2);
    ASSERT_EQ(feature.get_bounding_box()[0], 1.0);
    ASSERT_EQ(feature.get_bounding_box()[1], 2.0);

    ASSERT_EQ(feature.get_property("prop_0").as_natural_number(), 0);
    ASSERT_EQ(feature.get_property("prop_1").as_string(), "1");
    ASSERT_FALSE(feature.get_property("prop_2").as_boolean());
    ASSERT_EQ(feature.get_property("prop_3").as_real_number(), 2.0);
    ASSERT_ANY_THROW(feature.get_property("doesnotexist"));

    geometry = geojson::JSONGeometry::of_multipoint(two_dimensions);

    feature = geojson::Feature(geometry, bounding_box, properties);

    ASSERT_EQ(feature.get_geometry().get_type(), geojson::JSONGeometryType::MultiPoint);
    ASSERT_EQ(feature.get_geometry().as_multipoint().size(), 3);

    ASSERT_EQ(feature.get_properties().size(), 4);
    ASSERT_EQ(feature.get_bounding_box().size(), 2);
    ASSERT_EQ(feature.get_bounding_box()[0], 1.0);
    ASSERT_EQ(feature.get_bounding_box()[1], 2.0);

    ASSERT_EQ(feature.get_property("prop_0").as_natural_number(), 0);
    ASSERT_EQ(feature.get_property("prop_1").as_string(), "1");
    ASSERT_FALSE(feature.get_property("prop_2").as_boolean());
    ASSERT_EQ(feature.get_property("prop_3").as_real_number(), 2.0);
    ASSERT_ANY_THROW(feature.get_property("doesnotexist"));

    geometry = geojson::JSONGeometry::of_multilinestring(three_dimensions);

    feature = geojson::Feature(geometry, bounding_box, properties);

    ASSERT_EQ(feature.get_geometry().get_type(), geojson::JSONGeometryType::MultiLineString);
    ASSERT_EQ(feature.get_geometry().as_multilinestring().size(), 2);
    ASSERT_EQ(feature.get_geometry().as_multilinestring()[0].size(), 3);
    ASSERT_EQ(feature.get_geometry().as_multilinestring()[1].size(), 3);

    ASSERT_EQ(feature.get_properties().size(), 4);
    ASSERT_EQ(feature.get_bounding_box().size(), 2);
    ASSERT_EQ(feature.get_bounding_box()[0], 1.0);
    ASSERT_EQ(feature.get_bounding_box()[1], 2.0);

    ASSERT_EQ(feature.get_property("prop_0").as_natural_number(), 0);
    ASSERT_EQ(feature.get_property("prop_1").as_string(), "1");
    ASSERT_FALSE(feature.get_property("prop_2").as_boolean());
    ASSERT_EQ(feature.get_property("prop_3").as_real_number(), 2.0);
    ASSERT_ANY_THROW(feature.get_property("doesnotexist"));

    geometry = geojson::JSONGeometry::of_multipolygon(four_dimensions);

    feature = geojson::Feature(geometry, bounding_box, properties);

    ASSERT_EQ(feature.get_geometry().get_type(), geojson::JSONGeometryType::MultiPolygon);
    ASSERT_EQ(feature.get_geometry().as_multipolygon().size(), 2);
    ASSERT_EQ(feature.get_geometry().as_multipolygon()[0].outer().size(), 3);
    ASSERT_EQ(feature.get_geometry().as_multipolygon()[0].inners().size(), 0);
    ASSERT_EQ(feature.get_geometry().as_multipolygon()[1].outer().size(), 3);
    ASSERT_EQ(feature.get_geometry().as_multipolygon()[1].inners().size(), 0);

    ASSERT_EQ(feature.get_properties().size(), 4);
    ASSERT_EQ(feature.get_bounding_box().size(), 2);
    ASSERT_EQ(feature.get_bounding_box()[0], 1.0);
    ASSERT_EQ(feature.get_bounding_box()[1], 2.0);

    ASSERT_EQ(feature.get_property("prop_0").as_natural_number(), 0);
    ASSERT_EQ(feature.get_property("prop_1").as_string(), "1");
    ASSERT_FALSE(feature.get_property("prop_2").as_boolean());
    ASSERT_EQ(feature.get_property("prop_3").as_real_number(), 2.0);
    ASSERT_ANY_THROW(feature.get_property("doesnotexist"));

    geojson::Feature copy = geojson::Feature(feature);

    ASSERT_EQ(feature.get_geometry().get_type(), geojson::JSONGeometryType::MultiPolygon);
    ASSERT_EQ(feature.get_geometry().as_multipolygon().size(), 2);
    ASSERT_EQ(feature.get_geometry().as_multipolygon()[0].outer().size(), 3);
    ASSERT_EQ(feature.get_geometry().as_multipolygon()[0].inners().size(), 0);
    ASSERT_EQ(feature.get_geometry().as_multipolygon()[1].outer().size(), 3);
    ASSERT_EQ(feature.get_geometry().as_multipolygon()[1].inners().size(), 0);

    ASSERT_EQ(feature.get_properties().size(), 4);
    ASSERT_EQ(feature.get_bounding_box().size(), 2);
    ASSERT_EQ(feature.get_bounding_box()[0], 1.0);
    ASSERT_EQ(feature.get_bounding_box()[1], 2.0);

    ASSERT_EQ(feature.get_property("prop_0").as_natural_number(), 0);
    ASSERT_EQ(feature.get_property("prop_1").as_string(), "1");
    ASSERT_FALSE(feature.get_property("prop_2").as_boolean());
    ASSERT_EQ(feature.get_property("prop_3").as_real_number(), 2.0);
    ASSERT_ANY_THROW(feature.get_property("doesnotexist"));
}

TEST_F(Feature_Test, geometry_collection_test) {

}

TEST_F(Feature_Test, ptree_test) {
    std::string data = "{ "
      "\"type\": \"Feature\", "
      "\"bbox\": [102.0, 80, 14, 35], "
      "\"geometry\": { "
        "\"type\": \"LineString\", "
        "\"coordinates\": [ "
          "[102.0, 0.0], [103.0, 1.0], [104.0, 0.0], [105.0, 1.0] "
        "] "
      "}, "
      "\"properties\": { "
        "\"prop0\": \"value0\", "
        "\"prop1\": 0.0, "
        "\"prop2\": \"false\""
      "} "
    "}";

    std::stringstream stream;
    stream << data;
    boost::property_tree::ptree tree;
    boost::property_tree::json_parser::read_json(stream, tree);

    geojson::Feature feature(tree);

    ASSERT_EQ(feature.get_bounding_box().size(), 4);
    ASSERT_EQ(feature.get_bounding_box()[0], 102.0);
    ASSERT_EQ(feature.get_bounding_box()[1], 80);
    ASSERT_EQ(feature.get_bounding_box()[2], 14);
    ASSERT_EQ(feature.get_bounding_box()[3], 35);

    ASSERT_EQ(feature.get_properties().size(), 3);
    ASSERT_EQ(feature.get_property("prop0").get_type(), geojson::PropertyType::String);
    ASSERT_EQ(feature.get_property("prop0").as_string(), "value0");
    ASSERT_EQ(feature.get_property("prop1").get_type(), geojson::PropertyType::Real);
    ASSERT_EQ(feature.get_property("prop1").as_real_number(), 0.0);
    ASSERT_EQ(feature.get_property("prop2").get_type(), geojson::PropertyType::Boolean);
    ASSERT_FALSE(feature.get_property("prop2").as_boolean());
    
    ASSERT_EQ(feature.get_type(), geojson::FeatureType::LineString);
    ASSERT_EQ(feature.get_geometry().get_type(), geojson::JSONGeometryType::LineString);
    ASSERT_EQ(feature.get_geometry().as_linestring().size(), 4);

    data = "{ "
      "\"type\": \"Feature\", "
      "\"bbox\": [102.0, 80, 14, 35], "
      "\"geometry\": { "
        "\"type\": \"Point\", "
        "\"coordinates\": [102.0, 0.0] "
      "}, "
      "\"properties\": { "
        "\"prop0\": \"value0\", "
        "\"prop1\": 0.0, "
        "\"prop2\": \"false\""
      "} "
    "}";

    stream = std::stringstream();
    stream << data;
    tree = boost::property_tree::ptree();
    boost::property_tree::json_parser::read_json(stream, tree);

    feature = geojson::Feature(tree);

    ASSERT_EQ(feature.get_bounding_box().size(), 4);
    ASSERT_EQ(feature.get_bounding_box()[0], 102.0);
    ASSERT_EQ(feature.get_bounding_box()[1], 80);
    ASSERT_EQ(feature.get_bounding_box()[2], 14);
    ASSERT_EQ(feature.get_bounding_box()[3], 35);

    ASSERT_EQ(feature.get_properties().size(), 3);
    ASSERT_EQ(feature.get_property("prop0").get_type(), geojson::PropertyType::String);
    ASSERT_EQ(feature.get_property("prop0").as_string(), "value0");
    ASSERT_EQ(feature.get_property("prop1").get_type(), geojson::PropertyType::Real);
    ASSERT_EQ(feature.get_property("prop1").as_real_number(), 0.0);
    ASSERT_EQ(feature.get_property("prop2").get_type(), geojson::PropertyType::Boolean);
    ASSERT_FALSE(feature.get_property("prop2").as_boolean());
    
    ASSERT_EQ(feature.get_type(), geojson::FeatureType::Point);
    ASSERT_EQ(feature.get_geometry().get_type(), geojson::JSONGeometryType::Point);
    ASSERT_EQ(feature.get_geometry().as_point().get<0>(), 102);
    ASSERT_EQ(feature.get_geometry().as_point().get<1>(), 0);

    data = "{ "
      "\"type\": \"Feature\", "
      "\"bbox\": [102.0, 80, 14, 35], "
      "\"geometry\": { "
        "\"type\": \"Polygon\", "
        "\"coordinates\": [ "
               "[ "
                   "[-10.0, -10.0], "
                   "[10.0, -10.0], "
                   "[10.0, 10.0], "
                   "[-10.0, -10.0] "
               "] "
           "] "
      "}, "
      "\"properties\": { "
        "\"prop0\": \"value0\", "
        "\"prop1\": 0.0, "
        "\"prop2\": \"false\""
      "} "
    "}";

    stream = std::stringstream();
    stream << data;
    tree = boost::property_tree::ptree();
    boost::property_tree::json_parser::read_json(stream, tree);

    feature = geojson::Feature(tree);

    ASSERT_EQ(feature.get_bounding_box().size(), 4);
    ASSERT_EQ(feature.get_bounding_box()[0], 102.0);
    ASSERT_EQ(feature.get_bounding_box()[1], 80);
    ASSERT_EQ(feature.get_bounding_box()[2], 14);
    ASSERT_EQ(feature.get_bounding_box()[3], 35);

    ASSERT_EQ(feature.get_properties().size(), 3);
    ASSERT_EQ(feature.get_property("prop0").get_type(), geojson::PropertyType::String);
    ASSERT_EQ(feature.get_property("prop0").as_string(), "value0");
    ASSERT_EQ(feature.get_property("prop1").get_type(), geojson::PropertyType::Real);
    ASSERT_EQ(feature.get_property("prop1").as_real_number(), 0.0);
    ASSERT_EQ(feature.get_property("prop2").get_type(), geojson::PropertyType::Boolean);
    ASSERT_FALSE(feature.get_property("prop2").as_boolean());
    
    ASSERT_EQ(feature.get_type(), geojson::FeatureType::Polygon);
    ASSERT_EQ(feature.get_geometry().get_type(), geojson::JSONGeometryType::Polygon);
    ASSERT_EQ(feature.get_geometry().as_polygon().outer().size(), 4);

    data = "{ "
      "\"type\": \"Feature\", "
      "\"bbox\": [102.0, 80, 14, 35], "
      "\"geometry\": { "
        "\"type\": \"MultiPoint\", "
        "\"coordinates\": [ "
          "[102.0, 0.0], [103.0, 1.0], [104.0, 0.0], [105.0, 1.0] "
        "] "
      "}, "
      "\"properties\": { "
        "\"prop0\": \"value0\", "
        "\"prop1\": 0.0, "
        "\"prop2\": \"false\""
      "} "
    "}";

    stream = std::stringstream();
    stream << data;
    tree = boost::property_tree::ptree();
    boost::property_tree::json_parser::read_json(stream, tree);

    feature = geojson::Feature(tree);

    ASSERT_EQ(feature.get_bounding_box().size(), 4);
    ASSERT_EQ(feature.get_bounding_box()[0], 102.0);
    ASSERT_EQ(feature.get_bounding_box()[1], 80);
    ASSERT_EQ(feature.get_bounding_box()[2], 14);
    ASSERT_EQ(feature.get_bounding_box()[3], 35);

    ASSERT_EQ(feature.get_properties().size(), 3);
    ASSERT_EQ(feature.get_property("prop0").get_type(), geojson::PropertyType::String);
    ASSERT_EQ(feature.get_property("prop0").as_string(), "value0");
    ASSERT_EQ(feature.get_property("prop1").get_type(), geojson::PropertyType::Real);
    ASSERT_EQ(feature.get_property("prop1").as_real_number(), 0.0);
    ASSERT_EQ(feature.get_property("prop2").get_type(), geojson::PropertyType::Boolean);
    ASSERT_FALSE(feature.get_property("prop2").as_boolean());
    
    ASSERT_EQ(feature.get_type(), geojson::FeatureType::MultiPoint);
    ASSERT_EQ(feature.get_geometry().get_type(), geojson::JSONGeometryType::MultiPoint);
    ASSERT_EQ(feature.get_geometry().as_multipoint().size(), 4);

    data = "{ "
      "\"type\": \"Feature\", "
      "\"bbox\": [102.0, 80, 14, 35], "
      "\"geometry\": { "
        "\"type\": \"MultiLineString\", "
        "\"coordinates\": [ "
            "[ "
                "[102.0, 0.0], [103.0, 1.0], [104.0, 0.0], [105.0, 1.0] "
            "], "
            "[ "
                "[152.0, 5.0], [153.0, 6.0], [154.0, 5.0], [155.0, 6.0] "
            "] "
        "] "
      "}, "
      "\"properties\": { "
        "\"prop0\": \"value0\", "
        "\"prop1\": 0.0, "
        "\"prop2\": \"false\""
      "} "
    "}";

    stream = std::stringstream();
    stream << data;
    tree = boost::property_tree::ptree();
    boost::property_tree::json_parser::read_json(stream, tree);

    feature = geojson::Feature(tree);

    ASSERT_EQ(feature.get_bounding_box().size(), 4);
    ASSERT_EQ(feature.get_bounding_box()[0], 102.0);
    ASSERT_EQ(feature.get_bounding_box()[1], 80);
    ASSERT_EQ(feature.get_bounding_box()[2], 14);
    ASSERT_EQ(feature.get_bounding_box()[3], 35);

    ASSERT_EQ(feature.get_properties().size(), 3);
    ASSERT_EQ(feature.get_property("prop0").get_type(), geojson::PropertyType::String);
    ASSERT_EQ(feature.get_property("prop0").as_string(), "value0");
    ASSERT_EQ(feature.get_property("prop1").get_type(), geojson::PropertyType::Real);
    ASSERT_EQ(feature.get_property("prop1").as_real_number(), 0.0);
    ASSERT_EQ(feature.get_property("prop2").get_type(), geojson::PropertyType::Boolean);
    ASSERT_FALSE(feature.get_property("prop2").as_boolean());
    
    ASSERT_EQ(feature.get_type(), geojson::FeatureType::MultiLineString);
    ASSERT_EQ(feature.get_geometry().get_type(), geojson::JSONGeometryType::MultiLineString);
    ASSERT_EQ(feature.get_geometry().as_multilinestring().size(), 2);
    ASSERT_EQ(feature.get_geometry().as_multilinestring()[0].size(), 4);
    ASSERT_EQ(feature.get_geometry().as_multilinestring()[1].size(), 4);

    data = "{ "
      "\"type\": \"Feature\", "
      "\"bbox\": [102.0, 80, 14, 35], "
      "\"geometry\": { "
        "\"type\": \"MultiPolygon\", "
        "\"coordinates\": [ "
           "[ "
               "[ "
                   "[180.0, 40.0], [180.0, 50.0], [170.0, 50.0], "
                   "[170.0, 40.0], [180.0, 40.0] "
               "] "
           "], "
           "[ "
               "[ "
                   "[-170.0, 40.0], [-170.0, 50.0], [-180.0, 50.0], "
                   "[-180.0, 40.0], [-170.0, 40.0] "
               "] "
           "] "
       "] "
      "}, "
      "\"properties\": { "
        "\"prop0\": \"value0\", "
        "\"prop1\": 0.0, "
        "\"prop2\": \"false\""
      "} "
    "}";

    stream = std::stringstream();
    stream << data;
    tree = boost::property_tree::ptree();
    boost::property_tree::json_parser::read_json(stream, tree);

    feature = geojson::Feature(tree);

    ASSERT_EQ(feature.get_bounding_box().size(), 4);
    ASSERT_EQ(feature.get_bounding_box()[0], 102.0);
    ASSERT_EQ(feature.get_bounding_box()[1], 80);
    ASSERT_EQ(feature.get_bounding_box()[2], 14);
    ASSERT_EQ(feature.get_bounding_box()[3], 35);

    ASSERT_EQ(feature.get_properties().size(), 3);
    ASSERT_EQ(feature.get_property("prop0").get_type(), geojson::PropertyType::String);
    ASSERT_EQ(feature.get_property("prop0").as_string(), "value0");
    ASSERT_EQ(feature.get_property("prop1").get_type(), geojson::PropertyType::Real);
    ASSERT_EQ(feature.get_property("prop1").as_real_number(), 0.0);
    ASSERT_EQ(feature.get_property("prop2").get_type(), geojson::PropertyType::Boolean);
    ASSERT_FALSE(feature.get_property("prop2").as_boolean());
    
    ASSERT_EQ(feature.get_type(), geojson::FeatureType::MultiPolygon);
    ASSERT_EQ(feature.get_geometry().get_type(), geojson::JSONGeometryType::MultiPolygon);
    ASSERT_EQ(feature.get_geometry().as_multipolygon().size(), 2);
    ASSERT_EQ(feature.get_geometry().as_multipolygon()[0].outer().size(), 5);
    ASSERT_EQ(feature.get_geometry().as_multipolygon()[0].inners().size(), 0);
    ASSERT_EQ(feature.get_geometry().as_multipolygon()[1].outer().size(), 5);
    ASSERT_EQ(feature.get_geometry().as_multipolygon()[1].inners().size(), 0);
}