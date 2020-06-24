#include "gtest/gtest.h"
#include <features/Features.hpp>
#include <JSONProperty.hpp>
#include <JSONGeometry.hpp>
#include <FeatureBuilder.hpp>
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

    // First test to see if a basic point can be created and checked
    geojson::PointFeature basic_point(geojson::coordinate_t(x, y));

    ASSERT_EQ(basic_point.get_type(), geojson::FeatureType::Point);
    ASSERT_EQ(basic_point.get_id(), "");

    ASSERT_EQ(basic_point.geometry().get<0>(), x);
    ASSERT_EQ(basic_point.geometry().get<1>(), y);

    ASSERT_EQ(basic_point.get_properties().size(), 0);
    ASSERT_EQ(basic_point.get_bounding_box().size(), 0);

    ASSERT_TRUE(basic_point.is_leaf());
    ASSERT_TRUE(basic_point.is_root());
    ASSERT_EQ(basic_point.get_origination_length(), 0);
    ASSERT_EQ(basic_point.get_destination_length(), 0);
    ASSERT_EQ(basic_point.get_contributor_count(), 0);

    ASSERT_ANY_THROW(basic_point.get_property("doesnotexist"));

    std::vector<double> bounding_box{1.0, 2.0};

    // Next, test the slightly more complicated case of the feature having bounds
    geojson::PointFeature point_and_bound(
        geojson::coordinate_t(x, y),
        "Point and Bound test",
        geojson::PropertyMap(),
        bounding_box
    );
    
    ASSERT_EQ(point_and_bound.geometry().get<0>(), x);
    ASSERT_EQ(point_and_bound.geometry().get<1>(), y);

    ASSERT_TRUE(point_and_bound.is_leaf());
    ASSERT_TRUE(point_and_bound.is_root());
    ASSERT_EQ(point_and_bound.get_origination_length(), 0);
    ASSERT_EQ(point_and_bound.get_destination_length(), 0);
    ASSERT_EQ(point_and_bound.get_contributor_count(), 0);

    ASSERT_EQ(point_and_bound.get_properties().size(), 0);
    ASSERT_EQ(point_and_bound.get_bounding_box().size(), 2);
    ASSERT_EQ(point_and_bound.get_bounding_box()[0], 1.0);
    ASSERT_EQ(point_and_bound.get_bounding_box()[1], 2.0);

    ASSERT_ANY_THROW(point_and_bound.get_property("doesnotexist"));

    geojson::PropertyMap properties{
        {"prop_0", geojson::JSONProperty("prop_0", 0)},
        {"prop_1", geojson::JSONProperty("prop_1", "1")},
        {"prop_2", geojson::JSONProperty("prop_2", false)},
        {"prop_3", geojson::JSONProperty("prop_3", 2.0)}
    };

    // Next, test the case of where the point has properties and an upstream point as well
    geojson::PointFeature point_and_properties(
      geojson::coordinate_t(x, y),
      "Point and properties test",
      properties,
      bounding_box,
      {
        &point_and_bound
      }
    );

    ASSERT_EQ(point_and_properties.geometry().get<0>(), x);
    ASSERT_EQ(point_and_properties.geometry().get<1>(), y);

    ASSERT_EQ(point_and_properties.get_type(), geojson::FeatureType::Point);

    ASSERT_EQ(point_and_properties.get_properties().size(), 4);
    ASSERT_EQ(point_and_properties.get_bounding_box().size(), 2);
    ASSERT_EQ(point_and_properties.get_bounding_box()[0], 1.0);
    ASSERT_EQ(point_and_properties.get_bounding_box()[1], 2.0);

    ASSERT_EQ(point_and_properties.get_property("prop_0").as_natural_number(), 0);
    ASSERT_EQ(point_and_properties.get_property("prop_1").as_string(), "1");
    ASSERT_FALSE(point_and_properties.get_property("prop_2").as_boolean());
    ASSERT_EQ(point_and_properties.get_property("prop_3").as_real_number(), 2.0);
    ASSERT_ANY_THROW(point_and_properties.get_property("doesnotexist"));

    ASSERT_FALSE(point_and_properties.is_root());
    ASSERT_TRUE(point_and_properties.is_leaf());
    ASSERT_EQ(point_and_properties.get_contributor_count(), 1);
    ASSERT_EQ(point_and_properties.get_origination_length(), 1);
    ASSERT_EQ(point_and_properties.get_destination_length(), 0);
    ASSERT_EQ(point_and_properties.get_number_of_origination_features(), 1);
    ASSERT_EQ(point_and_properties.get_number_of_destination_features(), 0);

    ASSERT_EQ(point_and_bound.get_destination_length(), 1);
    ASSERT_EQ(point_and_bound.get_number_of_origination_features(), 0);
    ASSERT_EQ(point_and_bound.get_number_of_destination_features(), 1);

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

    geojson::LineStringFeature linestring (
      geojson::linestring(two_dimensions),
      "linestring test",
      properties,
      bounding_box,
      {
        &basic_point,
        &point_and_bound
      },
      {
        &point_and_properties
      }
    );

    ASSERT_EQ(linestring.get_type(), geojson::FeatureType::LineString);

    ASSERT_EQ(linestring.geometry().size(), 3);

    ASSERT_EQ(linestring.get_properties().size(), 4);
    ASSERT_EQ(linestring.get_bounding_box().size(), 2);
    ASSERT_EQ(linestring.get_bounding_box()[0], 1.0);
    ASSERT_EQ(linestring.get_bounding_box()[1], 2.0);

    ASSERT_EQ(linestring.get_property("prop_0").as_natural_number(), 0);
    ASSERT_EQ(linestring.get_property("prop_1").as_string(), "1");
    ASSERT_FALSE(linestring.get_property("prop_2").as_boolean());
    ASSERT_EQ(linestring.get_property("prop_3").as_real_number(), 2.0);
    ASSERT_ANY_THROW(linestring.get_property("doesnotexist"));

    ASSERT_FALSE(linestring.is_leaf());
    ASSERT_FALSE(linestring.is_root());
    ASSERT_EQ(linestring.get_destination_length(), 1);
    ASSERT_EQ(linestring.get_origination_length(), 1);
    ASSERT_EQ(linestring.get_contributor_count(), 2);
    ASSERT_EQ(linestring.get_number_of_destination_features(), 1);
    ASSERT_EQ(linestring.get_number_of_origination_features(), 2);
    
    ASSERT_EQ(basic_point.get_number_of_destination_features(), 1);
    ASSERT_EQ(basic_point.get_number_of_origination_features(), 0);
    ASSERT_EQ(basic_point.get_destination_length(), 2);

    ASSERT_EQ(point_and_bound.get_number_of_destination_features(), 2);
    ASSERT_EQ(point_and_bound.get_number_of_origination_features(), 0);
    ASSERT_EQ(point_and_bound.get_destination_length(), 2);

    ASSERT_EQ(point_and_properties.get_number_of_destination_features(), 0);
    ASSERT_EQ(point_and_properties.get_number_of_origination_features(), 2);
    ASSERT_EQ(point_and_properties.get_origination_length(), 2);

    geojson::PolygonFeature polygon(
      geojson::polygon(three_dimensions),
      "Polygon test",
      properties,
      bounding_box,
      {
        &linestring
      }
    );

    ASSERT_EQ(polygon.get_type(), geojson::FeatureType::Polygon);

    ASSERT_EQ(polygon.geometry().outer().size(), 3);
    ASSERT_EQ(polygon.geometry().inners().size(), 1);
    ASSERT_EQ(polygon.geometry().inners()[0].size(), 3);

    ASSERT_EQ(polygon.get_properties().size(), 4);
    ASSERT_EQ(polygon.get_bounding_box().size(), 2);
    ASSERT_EQ(polygon.get_bounding_box()[0], 1.0);
    ASSERT_EQ(polygon.get_bounding_box()[1], 2.0);

    ASSERT_EQ(polygon.get_property("prop_0").as_natural_number(), 0);
    ASSERT_EQ(polygon.get_property("prop_1").as_string(), "1");
    ASSERT_FALSE(polygon.get_property("prop_2").as_boolean());
    ASSERT_EQ(polygon.get_property("prop_3").as_real_number(), 2.0);
    ASSERT_ANY_THROW(polygon.get_property("doesnotexist"));

    ASSERT_FALSE(polygon.is_root());
    ASSERT_TRUE(polygon.is_leaf());
    ASSERT_EQ(polygon.get_origination_length(), 2);
    ASSERT_EQ(polygon.get_destination_length(), 0);
    ASSERT_EQ(polygon.get_contributor_count(), 3);
    ASSERT_EQ(polygon.get_number_of_destination_features(), 0);
    ASSERT_EQ(polygon.get_number_of_origination_features(), 1);
    
    ASSERT_EQ(basic_point.get_number_of_destination_features(), 1);
    ASSERT_EQ(basic_point.get_number_of_origination_features(), 0);
    ASSERT_EQ(basic_point.get_destination_length(), 2);

    ASSERT_EQ(point_and_bound.get_number_of_destination_features(), 2);
    ASSERT_EQ(point_and_bound.get_number_of_origination_features(), 0);
    ASSERT_EQ(point_and_bound.get_destination_length(), 2);

    ASSERT_EQ(point_and_properties.get_number_of_destination_features(), 0);
    ASSERT_EQ(point_and_properties.get_number_of_origination_features(), 2);
    ASSERT_EQ(point_and_properties.get_origination_length(), 2);

    ASSERT_EQ(linestring.get_number_of_destination_features(), 2);
    ASSERT_EQ(linestring.get_number_of_origination_features(), 2);
    ASSERT_EQ(linestring.get_origination_length(), 1);
    ASSERT_EQ(linestring.get_destination_length(), 1);

    geojson::MultiPointFeature multipoint(
      geojson::multipoint(two_dimensions),
      "multipoint test",
      properties,
      bounding_box,
      {
        &polygon
      },
      {
        &point_and_properties
      }
    );

    ASSERT_EQ(multipoint.get_type(), geojson::FeatureType::MultiPoint);
    ASSERT_EQ(multipoint.geometry().size(), 3);

    ASSERT_EQ(multipoint.get_properties().size(), 4);
    ASSERT_EQ(multipoint.get_bounding_box().size(), 2);
    ASSERT_EQ(multipoint.get_bounding_box()[0], 1.0);
    ASSERT_EQ(multipoint.get_bounding_box()[1], 2.0);

    ASSERT_EQ(multipoint.get_property("prop_0").as_natural_number(), 0);
    ASSERT_EQ(multipoint.get_property("prop_1").as_string(), "1");
    ASSERT_FALSE(multipoint.get_property("prop_2").as_boolean());
    ASSERT_EQ(multipoint.get_property("prop_3").as_real_number(), 2.0);
    ASSERT_ANY_THROW(multipoint.get_property("doesnotexist"));
    
    ASSERT_FALSE(multipoint.is_root());
    ASSERT_FALSE(multipoint.is_leaf());
    ASSERT_EQ(multipoint.get_origination_length(), 3);
    ASSERT_EQ(multipoint.get_destination_length(), 1);
    ASSERT_EQ(multipoint.get_contributor_count(), 4);
    ASSERT_EQ(multipoint.get_number_of_origination_features(), 1);
    ASSERT_EQ(multipoint.get_number_of_destination_features(), 1);
    
    ASSERT_EQ(basic_point.get_number_of_destination_features(), 1);
    ASSERT_EQ(basic_point.get_number_of_origination_features(), 0);
    ASSERT_EQ(basic_point.get_destination_length(), 4);
    ASSERT_EQ(basic_point.get_origination_length(), 0);
    
    ASSERT_EQ(point_and_bound.get_number_of_destination_features(), 2);
    ASSERT_EQ(point_and_bound.get_number_of_origination_features(), 0);
    ASSERT_EQ(point_and_bound.get_destination_length(), 4);
    ASSERT_EQ(point_and_bound.get_origination_length(), 0);

    ASSERT_EQ(point_and_properties.get_number_of_destination_features(), 0);
    ASSERT_EQ(point_and_properties.get_number_of_origination_features(), 3);
    ASSERT_EQ(point_and_properties.get_origination_length(), 4);
    ASSERT_EQ(point_and_properties.get_destination_length(), 0);

    ASSERT_EQ(linestring.get_number_of_destination_features(), 2);
    ASSERT_EQ(linestring.get_number_of_origination_features(), 2);
    ASSERT_EQ(linestring.get_origination_length(), 1);
    ASSERT_EQ(linestring.get_destination_length(), 3);

    ASSERT_EQ(polygon.get_number_of_destination_features(), 1);
    ASSERT_EQ(polygon.get_number_of_origination_features(), 1);
    ASSERT_EQ(polygon.get_origination_length(), 2);
    ASSERT_EQ(polygon.get_destination_length(), 2);

    geojson::MultiLineStringFeature multilinestring(
      geojson::multilinestring(three_dimensions),
      "MultiLineString test",
      properties,
      bounding_box,
      {
        &linestring,
        &polygon
      },
      {
        &multipoint
      }
    );

    ASSERT_EQ(multilinestring.get_type(), geojson::FeatureType::MultiLineString);

    ASSERT_EQ(multilinestring.geometry().size(), 2);
    ASSERT_EQ(multilinestring.geometry()[0].size(), 3);
    ASSERT_EQ(multilinestring.geometry()[1].size(), 3);

    ASSERT_EQ(multilinestring.get_properties().size(), 4);
    ASSERT_EQ(multilinestring.get_bounding_box().size(), 2);
    ASSERT_EQ(multilinestring.get_bounding_box()[0], 1.0);
    ASSERT_EQ(multilinestring.get_bounding_box()[1], 2.0);

    ASSERT_EQ(multilinestring.get_property("prop_0").as_natural_number(), 0);
    ASSERT_EQ(multilinestring.get_property("prop_1").as_string(), "1");
    ASSERT_FALSE(multilinestring.get_property("prop_2").as_boolean());
    ASSERT_EQ(multilinestring.get_property("prop_3").as_real_number(), 2.0);
    ASSERT_ANY_THROW(multilinestring.get_property("doesnotexist"));

    ASSERT_FALSE(multilinestring.is_root());
    ASSERT_FALSE(multilinestring.is_leaf());
    ASSERT_EQ(multilinestring.get_origination_length(), 3);
    ASSERT_EQ(multilinestring.get_destination_length(), 2);
    ASSERT_EQ(multilinestring.get_contributor_count(), 4);
    ASSERT_EQ(multilinestring.get_number_of_origination_features(), 2);
    ASSERT_EQ(multilinestring.get_number_of_destination_features(), 1);
    
    ASSERT_EQ(basic_point.get_number_of_destination_features(), 1);
    ASSERT_EQ(basic_point.get_number_of_origination_features(), 0);
    ASSERT_EQ(basic_point.get_destination_length(), 5);
    ASSERT_EQ(basic_point.get_origination_length(), 0);

    ASSERT_EQ(point_and_bound.get_number_of_destination_features(), 2);
    ASSERT_EQ(point_and_bound.get_number_of_origination_features(), 0);
    ASSERT_EQ(point_and_bound.get_destination_length(), 5);
    ASSERT_EQ(point_and_bound.get_origination_length(), 0);

    ASSERT_EQ(point_and_properties.get_number_of_destination_features(), 0);
    ASSERT_EQ(point_and_properties.get_number_of_origination_features(), 3);
    ASSERT_EQ(point_and_properties.get_origination_length(), 5);
    ASSERT_EQ(point_and_properties.get_destination_length(), 0);
    ASSERT_EQ(point_and_properties.get_contributor_count(), 6);

    ASSERT_EQ(linestring.get_number_of_destination_features(), 3);
    ASSERT_EQ(linestring.get_number_of_origination_features(), 2);
    ASSERT_EQ(linestring.get_origination_length(), 1);
    ASSERT_EQ(linestring.get_destination_length(), 4);
    ASSERT_EQ(linestring.get_contributor_count(), 2);

    ASSERT_EQ(polygon.get_number_of_destination_features(), 2);
    ASSERT_EQ(polygon.get_number_of_origination_features(), 1);
    ASSERT_EQ(polygon.get_origination_length(), 2);
    ASSERT_EQ(polygon.get_destination_length(), 3);
    ASSERT_EQ(polygon.get_contributor_count(), 3);

    ASSERT_EQ(multipoint.get_number_of_destination_features(), 1);
    ASSERT_EQ(multipoint.get_number_of_origination_features(), 2);
    ASSERT_EQ(multipoint.get_origination_length(), 4);
    ASSERT_EQ(multipoint.get_destination_length(), 1);
    ASSERT_EQ(multipoint.get_contributor_count(), 5);

    geojson::MultiPolygonFeature multipolygon(
      geojson::multipolygon(four_dimensions),
      "MultiPolygon test",
      properties,
      bounding_box,
      {
        &multilinestring,
        &multipoint
      }
    );

    ASSERT_EQ(multipolygon.get_type(), geojson::FeatureType::MultiPolygon);

    ASSERT_EQ(multipolygon.geometry().size(), 2);
    ASSERT_EQ(multipolygon.geometry()[0].outer().size(), 3);
    ASSERT_EQ(multipolygon.geometry()[0].inners().size(), 0);
    ASSERT_EQ(multipolygon.geometry()[1].outer().size(), 3);
    ASSERT_EQ(multipolygon.geometry()[1].inners().size(), 0);

    ASSERT_EQ(multipolygon.get_properties().size(), 4);
    ASSERT_EQ(multipolygon.get_bounding_box().size(), 2);
    ASSERT_EQ(multipolygon.get_bounding_box()[0], 1.0);
    ASSERT_EQ(multipolygon.get_bounding_box()[1], 2.0);

    ASSERT_EQ(multipolygon.get_property("prop_0").as_natural_number(), 0);
    ASSERT_EQ(multipolygon.get_property("prop_1").as_string(), "1");
    ASSERT_FALSE(multipolygon.get_property("prop_2").as_boolean());
    ASSERT_EQ(multipolygon.get_property("prop_3").as_real_number(), 2.0);
    ASSERT_ANY_THROW(multipolygon.get_property("doesnotexist"));

    ASSERT_FALSE(multipolygon.is_root());
    ASSERT_TRUE(multipolygon.is_leaf());
    ASSERT_EQ(multipolygon.get_origination_length(), 5);
    ASSERT_EQ(multipolygon.get_destination_length(), 0);
    ASSERT_EQ(multipolygon.get_contributor_count(), 6);
    ASSERT_EQ(multipolygon.get_number_of_origination_features(), 2);
    ASSERT_EQ(multipolygon.get_number_of_destination_features(), 0);
    
    ASSERT_EQ(basic_point.get_number_of_destination_features(), 1);
    ASSERT_EQ(basic_point.get_number_of_origination_features(), 0);
    ASSERT_EQ(basic_point.get_destination_length(), 5);
    ASSERT_EQ(basic_point.get_origination_length(), 0);

    ASSERT_EQ(point_and_bound.get_number_of_destination_features(), 2);
    ASSERT_EQ(point_and_bound.get_number_of_origination_features(), 0);
    ASSERT_EQ(point_and_bound.get_destination_length(), 5);
    ASSERT_EQ(point_and_bound.get_origination_length(), 0);

    ASSERT_EQ(point_and_properties.get_number_of_destination_features(), 0);
    ASSERT_EQ(point_and_properties.get_number_of_origination_features(), 3);
    ASSERT_EQ(point_and_properties.get_origination_length(), 5);
    ASSERT_EQ(point_and_properties.get_destination_length(), 0);
    ASSERT_EQ(point_and_properties.get_contributor_count(), 6);

    ASSERT_EQ(linestring.get_number_of_destination_features(), 3);
    ASSERT_EQ(linestring.get_number_of_origination_features(), 2);
    ASSERT_EQ(linestring.get_origination_length(), 1);
    ASSERT_EQ(linestring.get_destination_length(), 4);
    ASSERT_EQ(linestring.get_contributor_count(), 2);

    ASSERT_EQ(polygon.get_number_of_destination_features(), 2);
    ASSERT_EQ(polygon.get_number_of_origination_features(), 1);
    ASSERT_EQ(polygon.get_origination_length(), 2);
    ASSERT_EQ(polygon.get_destination_length(), 3);
    ASSERT_EQ(polygon.get_contributor_count(), 3);

    ASSERT_EQ(multipoint.get_number_of_destination_features(), 2);
    ASSERT_EQ(multipoint.get_number_of_origination_features(), 2);
    ASSERT_EQ(multipoint.get_origination_length(), 4);
    ASSERT_EQ(multipoint.get_destination_length(), 1);
    ASSERT_EQ(multipoint.get_contributor_count(), 5);

    ASSERT_EQ(multilinestring.get_number_of_destination_features(), 2);
    ASSERT_EQ(multilinestring.get_number_of_origination_features(), 2);
    ASSERT_EQ(multilinestring.get_origination_length(), 3);
    ASSERT_EQ(multilinestring.get_destination_length(), 2);
    ASSERT_EQ(multilinestring.get_contributor_count(), 4);

    geojson::MultiPolygonFeature copy = geojson::MultiPolygonFeature(multipolygon);

    ASSERT_EQ(copy.get_type(), geojson::FeatureType::MultiPolygon);

    ASSERT_EQ(copy.geometry().size(), 2);
    ASSERT_EQ(copy.geometry()[0].outer().size(), 3);
    ASSERT_EQ(copy.geometry()[0].inners().size(), 0);
    ASSERT_EQ(copy.geometry()[1].outer().size(), 3);
    ASSERT_EQ(copy.geometry()[1].inners().size(), 0);

    ASSERT_EQ(copy.get_properties().size(), 4);
    ASSERT_EQ(copy.get_bounding_box().size(), 2);
    ASSERT_EQ(copy.get_bounding_box()[0], 1.0);
    ASSERT_EQ(copy.get_bounding_box()[1], 2.0);

    ASSERT_EQ(copy.get_property("prop_0").as_natural_number(), 0);
    ASSERT_EQ(copy.get_property("prop_1").as_string(), "1");
    ASSERT_FALSE(copy.get_property("prop_2").as_boolean());
    ASSERT_EQ(copy.get_property("prop_3").as_real_number(), 2.0);
    ASSERT_ANY_THROW(copy.get_property("doesnotexist"));

    ASSERT_FALSE(copy.is_root());
    ASSERT_TRUE(copy.is_leaf());
    ASSERT_EQ(copy.get_origination_length(), 5);
    ASSERT_EQ(copy.get_destination_length(), 0);
    ASSERT_EQ(copy.get_contributor_count(), 6);
    ASSERT_EQ(copy.get_number_of_origination_features(), 2);
    ASSERT_EQ(copy.get_number_of_destination_features(), 0);

    ASSERT_FALSE(multipolygon.is_root());
    ASSERT_TRUE(multipolygon.is_leaf());
    ASSERT_EQ(multipolygon.get_origination_length(), 5);
    ASSERT_EQ(multipolygon.get_destination_length(), 0);
    ASSERT_EQ(multipolygon.get_contributor_count(), 6);
    ASSERT_EQ(multipolygon.get_number_of_origination_features(), 2);
    ASSERT_EQ(multipolygon.get_number_of_destination_features(), 0);
    
    ASSERT_EQ(basic_point.get_number_of_destination_features(), 1);
    ASSERT_EQ(basic_point.get_number_of_origination_features(), 0);
    ASSERT_EQ(basic_point.get_destination_length(), 5);
    ASSERT_EQ(basic_point.get_origination_length(), 0);

    ASSERT_EQ(point_and_bound.get_number_of_destination_features(), 2);
    ASSERT_EQ(point_and_bound.get_number_of_origination_features(), 0);
    ASSERT_EQ(point_and_bound.get_destination_length(), 5);
    ASSERT_EQ(point_and_bound.get_origination_length(), 0);

    ASSERT_EQ(point_and_properties.get_number_of_destination_features(), 0);
    ASSERT_EQ(point_and_properties.get_number_of_origination_features(), 3);
    ASSERT_EQ(point_and_properties.get_origination_length(), 5);
    ASSERT_EQ(point_and_properties.get_destination_length(), 0);
    ASSERT_EQ(point_and_properties.get_contributor_count(), 6);

    ASSERT_EQ(linestring.get_number_of_destination_features(), 3);
    ASSERT_EQ(linestring.get_number_of_origination_features(), 2);
    ASSERT_EQ(linestring.get_origination_length(), 1);
    ASSERT_EQ(linestring.get_destination_length(), 4);
    ASSERT_EQ(linestring.get_contributor_count(), 2);

    ASSERT_EQ(polygon.get_number_of_destination_features(), 2);
    ASSERT_EQ(polygon.get_number_of_origination_features(), 1);
    ASSERT_EQ(polygon.get_origination_length(), 2);
    ASSERT_EQ(polygon.get_destination_length(), 3);
    ASSERT_EQ(polygon.get_contributor_count(), 3);

    // This number is larger than before due to the copy of the multipolygon feature
    ASSERT_EQ(multipoint.get_number_of_destination_features(), 2);
    ASSERT_EQ(multipoint.get_number_of_origination_features(), 2);
    ASSERT_EQ(multipoint.get_origination_length(), 4);
    ASSERT_EQ(multipoint.get_destination_length(), 1);
    ASSERT_EQ(multipoint.get_contributor_count(), 5);

    ASSERT_EQ(multilinestring.get_number_of_destination_features(), 2);
    ASSERT_EQ(multilinestring.get_number_of_origination_features(), 2);
    ASSERT_EQ(multilinestring.get_origination_length(), 3);
    ASSERT_EQ(multilinestring.get_destination_length(), 2);
    ASSERT_EQ(multilinestring.get_contributor_count(), 4);
}

TEST_F(Feature_Test, geometry_collection_test) {
    std::string data = "{ "
      "\"type\": \"Feature\", "
      "\"bbox\": [102.0, 80, 14, 35], "
      "\"geometries\": [ "
        "{ "
            "\"type\": \"Point\", "
            "\"coordinates\": [102.0, 0.5] "
        "}, "
        "{ "
          "\"type\": \"LineString\", "
          "\"coordinates\": [ "
            "[102.0, 0.0], [103.0, 1.0], [104.0, 0.0], [105.0, 1.0] "
          "] "
        "}, "
        "{ "
            "\"type\": \"Polygon\", "
            "\"coordinates\": [ "
                "[[35, 10], [45, 45], [15, 40], [10, 20], [35, 10]], "
                "[[20, 30], [35, 35], [30, 20], [20, 30]] "
            "] "
        "}, "
        "{ "
            "\"type\": \"MultiPoint\", "
            "\"coordinates\": [ "
                "[10, 40], [40, 30], [20, 20], [30, 10] "
            "] "
        "}, "
        "{ "
            "\"type\": \"MultiLineString\", "
            "\"coordinates\": [ "
            "    [ "
            "        [170.0, 45.0], [180.0, 45.0] "
            "    ], [ "
            "        [-180.0, 45.0], [-170.0, 45.0] "
            "    ] "
            "] "
        "}, "
        "{ "
          "   \"type\": \"MultiPolygon\", "
          "   \"coordinates\": [ "
          "       [ "
          "           [ "
          "               [180.0, 40.0], [180.0, 50.0], [170.0, 50.0], "
          "               [170.0, 40.0], [180.0, 40.0] "
          "           ] "
          "       ], "
          "       [ "
          "           [ "
          "               [-170.0, 40.0], [-170.0, 50.0], [-180.0, 50.0], "
          "               [-180.0, 40.0], [-170.0, 40.0] "
          "           ] "
          "       ] "
          "   ] "
          "}"
      "], "
      "\"properties\": { "
        "\"prop0\": \"value0\", "
        "\"prop1\": 0.0, "
        "\"prop2\": \"false\""
      "}, "
      "\"foreign_1\": \"member\", "
      "\"foreign_2\": 2, "
      "\"foreign_3\": true"
    "}";

    std::stringstream stream;
    stream << data;
    boost::property_tree::ptree tree;
    boost::property_tree::json_parser::read_json(stream, tree);

    auto feature_ptr = geojson::build_feature(tree);

    auto feature = (geojson::CollectionFeature*)feature_ptr.get();
    ASSERT_EQ(feature->get_type(), geojson::FeatureType::GeometryCollection);
    ASSERT_EQ(feature->get_geometry_collection().size(), 6);
    
    ASSERT_EQ(feature->point(0).get<0>(), 102.0);
    ASSERT_EQ(feature->point(0).get<1>(), 0.5);

    ASSERT_EQ(feature->linestring(1).size(), 4);
    ASSERT_EQ(feature->linestring(1)[0].get<0>(), 102.0);
    ASSERT_EQ(feature->linestring(1)[0].get<1>(), 0.0);

    ASSERT_EQ(feature->linestring(1)[1].get<0>(), 103.0);
    ASSERT_EQ(feature->linestring(1)[1].get<1>(), 1.0);

    ASSERT_EQ(feature->linestring(1)[2].get<0>(), 104.0);
    ASSERT_EQ(feature->linestring(1)[2].get<1>(), 0.0);

    ASSERT_EQ(feature->linestring(1)[3].get<0>(), 105.0);
    ASSERT_EQ(feature->linestring(1)[3].get<1>(), 1.0);

    ASSERT_EQ(feature->polygon(2).outer().size(), 5);
    ASSERT_EQ(feature->polygon(2).inners().size(), 1);
    ASSERT_EQ(feature->polygon(2).inners()[0].size(), 4);

    auto multipoint = feature->geometry<geojson::multipoint_t>(3);

    ASSERT_EQ(multipoint.size(), 4);

    ASSERT_EQ(multipoint[0].get<0>(), 10);
    ASSERT_EQ(multipoint[0].get<1>(), 40);
    
    ASSERT_EQ(multipoint[1].get<0>(), 40);
    ASSERT_EQ(multipoint[1].get<1>(), 30);
    
    ASSERT_EQ(multipoint[2].get<0>(), 20);
    ASSERT_EQ(multipoint[2].get<1>(), 20);
    
    ASSERT_EQ(multipoint[3].get<0>(), 30);
    ASSERT_EQ(multipoint[3].get<1>(), 10);
    
    auto multilinestring = feature->multilinestring(4);

    ASSERT_EQ(multilinestring.size(), 2);

    ASSERT_EQ(multilinestring[0].size(), 2);

    ASSERT_EQ(multilinestring[0][0].get<0>(), 170.0);
    ASSERT_EQ(multilinestring[0][0].get<1>(), 45.0);
    
    ASSERT_EQ(multilinestring[0][1].get<0>(), 180.0);
    ASSERT_EQ(multilinestring[0][1].get<1>(), 45.0);
    
    ASSERT_EQ(multilinestring[1].size(), 2);

    ASSERT_EQ(multilinestring[1][0].get<0>(), -180.0);
    ASSERT_EQ(multilinestring[1][0].get<1>(), 45.0);
    
    ASSERT_EQ(multilinestring[1][1].get<0>(), -170.0);
    ASSERT_EQ(multilinestring[1][1].get<1>(), 45.0);

    auto multipolygon = *feature->multipolygons()[0];

    ASSERT_EQ(multipolygon.size(), 2);

    ASSERT_EQ(multipolygon[0].outer().size(), 5);
    ASSERT_EQ(multipolygon[0].inners().size(), 0);

    ASSERT_EQ(multipolygon[1].outer().size(), 5);
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
      "}, "
      "\"foreign_1\": \"member\", "
      "\"foreign_2\": 2, "
      "\"foreign_3\": true"
    "}";

    std::stringstream stream;
    stream << data;
    boost::property_tree::ptree tree;
    boost::property_tree::json_parser::read_json(stream, tree);

    auto feature = geojson::build_feature(tree);

    ASSERT_EQ(feature->get_bounding_box().size(), 4);
    ASSERT_EQ(feature->get_bounding_box()[0], 102.0);
    ASSERT_EQ(feature->get_bounding_box()[1], 80);
    ASSERT_EQ(feature->get_bounding_box()[2], 14);
    ASSERT_EQ(feature->get_bounding_box()[3], 35);

    ASSERT_EQ(feature->get_properties().size(), 3);
    ASSERT_EQ(feature->get_property("prop0").get_type(), geojson::PropertyType::String);
    ASSERT_EQ(feature->get_property("prop0").as_string(), "value0");
    ASSERT_EQ(feature->get_property("prop1").get_type(), geojson::PropertyType::Real);
    ASSERT_EQ(feature->get_property("prop1").as_real_number(), 0.0);
    ASSERT_EQ(feature->get_property("prop2").get_type(), geojson::PropertyType::Boolean);
    ASSERT_FALSE(feature->get_property("prop2").as_boolean());
    
    ASSERT_EQ(feature->get_type(), geojson::FeatureType::LineString);
    ASSERT_EQ(feature->geometry<geojson::linestring_t>().size(), 4);

    ASSERT_EQ(feature->get("foreign_1").as_string(), "member");
    ASSERT_EQ(feature->get("foreign_2").as_natural_number(), 2);
    ASSERT_EQ(feature->get("foreign_3").as_boolean(), true);

    ASSERT_EQ(feature->get_id(), "");

    data = "{ "
      "\"type\": \"Feature\", "
      "\"bbox\": [102.0, 80, 14, 35], "
      "\"id\": \"test_feature\", "
      "\"geometry\": { "
        "\"type\": \"Point\", "
        "\"coordinates\": [102.0, 0.0] "
      "}, "
      "\"properties\": { "
        "\"prop0\": \"value0\", "
        "\"prop1\": 0.0, "
        "\"prop2\": \"false\""
      "}, "
      "\"foreign_1\": \"member\", "
      "\"foreign_2\": 2, "
      "\"foreign_3\": true"
    "}";

    stream.str("");
    stream << data;
    tree = boost::property_tree::ptree();
    boost::property_tree::json_parser::read_json(stream, tree);

    geojson::Feature point_feature = geojson::build_feature(tree);

    ASSERT_EQ(point_feature->get_bounding_box().size(), 4);
    ASSERT_EQ(point_feature->get_bounding_box()[0], 102.0);
    ASSERT_EQ(point_feature->get_bounding_box()[1], 80);
    ASSERT_EQ(point_feature->get_bounding_box()[2], 14);
    ASSERT_EQ(point_feature->get_bounding_box()[3], 35);

    ASSERT_EQ(point_feature->get_properties().size(), 3);
    ASSERT_EQ(point_feature->get_property("prop0").get_type(), geojson::PropertyType::String);
    ASSERT_EQ(point_feature->get_property("prop0").as_string(), "value0");
    ASSERT_EQ(point_feature->get_property("prop1").get_type(), geojson::PropertyType::Real);
    ASSERT_EQ(point_feature->get_property("prop1").as_real_number(), 0.0);
    ASSERT_EQ(point_feature->get_property("prop2").get_type(), geojson::PropertyType::Boolean);
    ASSERT_FALSE(point_feature->get_property("prop2").as_boolean());
    
    ASSERT_EQ(point_feature->get_type(), geojson::FeatureType::Point);
    ASSERT_EQ(point_feature->geometry<geojson::coordinate_t>().get<0>(), 102);
    ASSERT_EQ(point_feature->geometry<geojson::coordinate_t>().get<1>(), 0);

    ASSERT_EQ(point_feature->get("foreign_1").as_string(), "member");
    ASSERT_EQ(point_feature->get("foreign_2").as_natural_number(), 2);
    ASSERT_EQ(point_feature->get("foreign_3").as_boolean(), true);

    ASSERT_EQ(point_feature->get_id(), "test_feature");

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
      "}, "
      "\"foreign_1\": \"member\", "
      "\"foreign_2\": 2, "
      "\"foreign_3\": true"
    "}";

    stream.str("");
    stream << data;
    tree = boost::property_tree::ptree();
    boost::property_tree::json_parser::read_json(stream, tree);

    feature = geojson::build_feature(tree);

    ASSERT_EQ(feature->get_bounding_box().size(), 4);
    ASSERT_EQ(feature->get_bounding_box()[0], 102.0);
    ASSERT_EQ(feature->get_bounding_box()[1], 80);
    ASSERT_EQ(feature->get_bounding_box()[2], 14);
    ASSERT_EQ(feature->get_bounding_box()[3], 35);

    ASSERT_EQ(feature->get_properties().size(), 3);
    ASSERT_EQ(feature->get_property("prop0").get_type(), geojson::PropertyType::String);
    ASSERT_EQ(feature->get_property("prop0").as_string(), "value0");
    ASSERT_EQ(feature->get_property("prop1").get_type(), geojson::PropertyType::Real);
    ASSERT_EQ(feature->get_property("prop1").as_real_number(), 0.0);
    ASSERT_EQ(feature->get_property("prop2").get_type(), geojson::PropertyType::Boolean);
    ASSERT_FALSE(feature->get_property("prop2").as_boolean());
    
    ASSERT_EQ(feature->get_type(), geojson::FeatureType::Polygon);
    ASSERT_EQ(feature->geometry<geojson::polygon_t>().outer().size(), 4);

    ASSERT_EQ(feature->get("foreign_1").as_string(), "member");
    ASSERT_EQ(feature->get("foreign_2").as_natural_number(), 2);
    ASSERT_EQ(feature->get("foreign_3").as_boolean(), true);

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
      "}, "
      "\"foreign_1\": \"member\", "
      "\"foreign_2\": 2, "
      "\"foreign_3\": true"
    "}";

    stream.str("");
    stream << data;
    tree = boost::property_tree::ptree();
    boost::property_tree::json_parser::read_json(stream, tree);

    feature = geojson::build_feature(tree);

    ASSERT_EQ(feature->get_bounding_box().size(), 4);
    ASSERT_EQ(feature->get_bounding_box()[0], 102.0);
    ASSERT_EQ(feature->get_bounding_box()[1], 80);
    ASSERT_EQ(feature->get_bounding_box()[2], 14);
    ASSERT_EQ(feature->get_bounding_box()[3], 35);

    ASSERT_EQ(feature->get_properties().size(), 3);
    ASSERT_EQ(feature->get_property("prop0").get_type(), geojson::PropertyType::String);
    ASSERT_EQ(feature->get_property("prop0").as_string(), "value0");
    ASSERT_EQ(feature->get_property("prop1").get_type(), geojson::PropertyType::Real);
    ASSERT_EQ(feature->get_property("prop1").as_real_number(), 0.0);
    ASSERT_EQ(feature->get_property("prop2").get_type(), geojson::PropertyType::Boolean);
    ASSERT_FALSE(feature->get_property("prop2").as_boolean());
    
    ASSERT_EQ(feature->get_type(), geojson::FeatureType::MultiPoint);
    ASSERT_EQ(feature->geometry<geojson::multipoint_t>().size(), 4);

    ASSERT_EQ(feature->get("foreign_1").as_string(), "member");
    ASSERT_EQ(feature->get("foreign_2").as_natural_number(), 2);
    ASSERT_EQ(feature->get("foreign_3").as_boolean(), true);

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
      "}, "
      "\"foreign_1\": \"member\", "
      "\"foreign_2\": 2, "
      "\"foreign_3\": true"
    "}";

    stream.str("");
    stream << data;
    tree = boost::property_tree::ptree();
    boost::property_tree::json_parser::read_json(stream, tree);

    feature = geojson::build_feature(tree);

    ASSERT_EQ(feature->get_bounding_box().size(), 4);
    ASSERT_EQ(feature->get_bounding_box()[0], 102.0);
    ASSERT_EQ(feature->get_bounding_box()[1], 80);
    ASSERT_EQ(feature->get_bounding_box()[2], 14);
    ASSERT_EQ(feature->get_bounding_box()[3], 35);

    ASSERT_EQ(feature->get_properties().size(), 3);
    ASSERT_EQ(feature->get_property("prop0").get_type(), geojson::PropertyType::String);
    ASSERT_EQ(feature->get_property("prop0").as_string(), "value0");
    ASSERT_EQ(feature->get_property("prop1").get_type(), geojson::PropertyType::Real);
    ASSERT_EQ(feature->get_property("prop1").as_real_number(), 0.0);
    ASSERT_EQ(feature->get_property("prop2").get_type(), geojson::PropertyType::Boolean);
    ASSERT_FALSE(feature->get_property("prop2").as_boolean());
    
    ASSERT_EQ(feature->get_type(), geojson::FeatureType::MultiLineString);
    ASSERT_EQ(feature->geometry<geojson::multilinestring_t>().size(), 2);
    ASSERT_EQ(feature->geometry<geojson::multilinestring_t>()[0].size(), 4);
    ASSERT_EQ(feature->geometry<geojson::multilinestring_t>()[1].size(), 4);

    ASSERT_EQ(feature->get("foreign_1").as_string(), "member");
    ASSERT_EQ(feature->get("foreign_2").as_natural_number(), 2);
    ASSERT_EQ(feature->get("foreign_3").as_boolean(), true);

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
      "}, "
      "\"foreign_1\": \"member\", "
      "\"foreign_2\": 2, "
      "\"foreign_3\": true"
    "}";

    stream.str("");
    stream << data;
    tree = boost::property_tree::ptree();
    boost::property_tree::json_parser::read_json(stream, tree);

    feature = geojson::build_feature(tree);

    ASSERT_EQ(feature->get_bounding_box().size(), 4);
    ASSERT_EQ(feature->get_bounding_box()[0], 102.0);
    ASSERT_EQ(feature->get_bounding_box()[1], 80);
    ASSERT_EQ(feature->get_bounding_box()[2], 14);
    ASSERT_EQ(feature->get_bounding_box()[3], 35);

    ASSERT_EQ(feature->get_properties().size(), 3);
    ASSERT_EQ(feature->get_property("prop0").get_type(), geojson::PropertyType::String);
    ASSERT_EQ(feature->get_property("prop0").as_string(), "value0");
    ASSERT_EQ(feature->get_property("prop1").get_type(), geojson::PropertyType::Real);
    ASSERT_EQ(feature->get_property("prop1").as_real_number(), 0.0);
    ASSERT_EQ(feature->get_property("prop2").get_type(), geojson::PropertyType::Boolean);
    ASSERT_FALSE(feature->get_property("prop2").as_boolean());
    
    ASSERT_EQ(feature->get_type(), geojson::FeatureType::MultiPolygon);
    ASSERT_EQ(feature->geometry<geojson::multipolygon_t>().size(), 2);
    ASSERT_EQ(feature->geometry<geojson::multipolygon_t>()[0].outer().size(), 5);
    ASSERT_EQ(feature->geometry<geojson::multipolygon_t>()[0].inners().size(), 0);
    ASSERT_EQ(feature->geometry<geojson::multipolygon_t>()[1].outer().size(), 5);
    ASSERT_EQ(feature->geometry<geojson::multipolygon_t>()[1].inners().size(), 0);

    ASSERT_EQ(feature->get("foreign_1").as_string(), "member");
    ASSERT_EQ(feature->get("foreign_2").as_natural_number(), 2);
    ASSERT_EQ(feature->get("foreign_3").as_boolean(), true);
}