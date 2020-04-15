#include "gtest/gtest.h"
#include <JSONGeometry.hpp>
#include <iostream>
#include <sstream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

class JSONGeometry_Test : public ::testing::Test {

    protected:


    JSONGeometry_Test() {

    }

    ~JSONGeometry_Test() override {

    }

    void SetUp() override {};

    void TearDown() override {};

};

TEST_F(JSONGeometry_Test, point_test) {
    double x = 102.0;
    double y = 0.5;
    geojson::JSONGeometry point = geojson::JSONGeometry::of_point(x, y);

    ASSERT_EQ(point.as_point().get<0>(), x);
    ASSERT_EQ(point.as_point().get<1>(), y);

    ASSERT_EQ(point.get_type(), geojson::JSONGeometryType::Point);

    geojson::JSONGeometry point_from_coordinate(point.as_point());

    ASSERT_EQ(point_from_coordinate.as_point().get<0>(), x);
    ASSERT_EQ(point_from_coordinate.as_point().get<1>(), y);

    ASSERT_EQ(point_from_coordinate.get_type(), geojson::JSONGeometryType::Point);

    std::string data = "{ "
               "\"type\": \"Point\", "
               "\"coordinates\": [102.0, 0.5] "
           "}";

    std::stringstream stream;
    stream << data;
    boost::property_tree::ptree tree;
    boost::property_tree::json_parser::read_json(stream, tree);
    geojson::JSONGeometry geometry = geojson::JSONGeometry::of_point(tree);

    ASSERT_EQ(geometry.get_type(), geojson::JSONGeometryType::Point);

    ASSERT_EQ(geometry.as_point().get<0>(), x);
    ASSERT_EQ(geometry.as_point().get<1>(), y);

    stream.str("");
    stream << data;
    boost::property_tree::ptree indirect_tree;
    boost::property_tree::json_parser::read_json(stream, indirect_tree);
    geojson::JSONGeometry indirect_geometry = geojson::JSONGeometry::from_ptree(indirect_tree);

    ASSERT_EQ(geometry.as_point().get<0>(), x);
    ASSERT_EQ(geometry.as_point().get<1>(), y);

    ASSERT_EQ(geometry.get_type(), geojson::JSONGeometryType::Point);

    geojson::JSONGeometry copy = geojson::JSONGeometry(geometry);

    ASSERT_EQ(copy.as_point().get<0>(), x);
    ASSERT_EQ(copy.as_point().get<1>(), y);

    ASSERT_EQ(copy.get_type(), geojson::JSONGeometryType::Point);
}

TEST_F(JSONGeometry_Test, linestring_test) {
    std::string data = "{ "
               "\"type\": \"LineString\", "
               "\"coordinates\": [ "
               "   [102.0, 0.0], "
               "   [103.0, 1.0], "
               "   [104.0, 0.0], "
               "   [105.0, 1.0] "
               "] "
           "}";

    std::stringstream stream;
    stream << data;
    boost::property_tree::ptree tree;
    boost::property_tree::json_parser::read_json(stream, tree);
    geojson::JSONGeometry geometry = geojson::JSONGeometry::of_linestring(tree);

    ASSERT_EQ(geometry.get_type(), geojson::JSONGeometryType::LineString);
    ASSERT_EQ(geometry.as_linestring().size(), 4);
    ASSERT_EQ(geometry.as_linestring()[0].get<0>(), 102.0);
    ASSERT_EQ(geometry.as_linestring()[0].get<1>(), 0.0);

    ASSERT_EQ(geometry.as_linestring()[1].get<0>(), 103.0);
    ASSERT_EQ(geometry.as_linestring()[1].get<1>(), 1.0);

    ASSERT_EQ(geometry.as_linestring()[2].get<0>(), 104.0);
    ASSERT_EQ(geometry.as_linestring()[2].get<1>(), 0.0);

    ASSERT_EQ(geometry.as_linestring()[3].get<0>(), 105.0);
    ASSERT_EQ(geometry.as_linestring()[3].get<1>(), 1.0);

    stream.str("");
    stream << data;
    boost::property_tree::ptree indirect_tree;
    boost::property_tree::json_parser::read_json(stream, indirect_tree);
    geojson::JSONGeometry indirect_geometry = geojson::JSONGeometry::from_ptree(indirect_tree);

    ASSERT_EQ(indirect_geometry.get_type(), geojson::JSONGeometryType::LineString);
    ASSERT_EQ(indirect_geometry.as_linestring().size(), 4);
    ASSERT_EQ(indirect_geometry.as_linestring()[0].get<0>(), 102.0);
    ASSERT_EQ(indirect_geometry.as_linestring()[0].get<1>(), 0.0);

    ASSERT_EQ(indirect_geometry.as_linestring()[1].get<0>(), 103.0);
    ASSERT_EQ(indirect_geometry.as_linestring()[1].get<1>(), 1.0);

    ASSERT_EQ(indirect_geometry.as_linestring()[2].get<0>(), 104.0);
    ASSERT_EQ(indirect_geometry.as_linestring()[2].get<1>(), 0.0);

    ASSERT_EQ(indirect_geometry.as_linestring()[3].get<0>(), 105.0);
    ASSERT_EQ(indirect_geometry.as_linestring()[3].get<1>(), 1.0);

    geojson::JSONGeometry copied_geometry = geojson::JSONGeometry(geometry.as_linestring());

    ASSERT_EQ(copied_geometry.get_type(), geojson::JSONGeometryType::LineString);
    ASSERT_EQ(copied_geometry.as_linestring().size(), 4);
    ASSERT_EQ(copied_geometry.as_linestring()[0].get<0>(), 102.0);
    ASSERT_EQ(copied_geometry.as_linestring()[0].get<1>(), 0.0);

    ASSERT_EQ(copied_geometry.as_linestring()[1].get<0>(), 103.0);
    ASSERT_EQ(copied_geometry.as_linestring()[1].get<1>(), 1.0);

    ASSERT_EQ(copied_geometry.as_linestring()[2].get<0>(), 104.0);
    ASSERT_EQ(copied_geometry.as_linestring()[2].get<1>(), 0.0);

    ASSERT_EQ(copied_geometry.as_linestring()[3].get<0>(), 105.0);
    ASSERT_EQ(copied_geometry.as_linestring()[3].get<1>(), 1.0);

    geojson::JSONGeometry copy = geojson::JSONGeometry(geometry);

    ASSERT_EQ(copy.get_type(), geojson::JSONGeometryType::LineString);
    ASSERT_EQ(copy.as_linestring().size(), 4);
    ASSERT_EQ(copy.as_linestring()[0].get<0>(), 102.0);
    ASSERT_EQ(copy.as_linestring()[0].get<1>(), 0.0);

    ASSERT_EQ(copy.as_linestring()[1].get<0>(), 103.0);
    ASSERT_EQ(copy.as_linestring()[1].get<1>(), 1.0);

    ASSERT_EQ(copy.as_linestring()[2].get<0>(), 104.0);
    ASSERT_EQ(copy.as_linestring()[2].get<1>(), 0.0);

    ASSERT_EQ(copy.as_linestring()[3].get<0>(), 105.0);
    ASSERT_EQ(copy.as_linestring()[3].get<1>(), 1.0);
}

TEST_F(JSONGeometry_Test, polygon_test) {
    std::string data = "{ "
        "\"type\": \"Polygon\", "
        "\"coordinates\": [ "
            "[[35, 10], [45, 45], [15, 40], [10, 20], [35, 10]], "
            "[[20, 30], [35, 35], [30, 20], [20, 30]] "
        "] "
    "} ";

    std::stringstream stream;
    stream << data;
    boost::property_tree::ptree tree;
    boost::property_tree::json_parser::read_json(stream, tree);
    geojson::JSONGeometry geometry = geojson::JSONGeometry::of_polygon(tree);

    ASSERT_EQ(geometry.get_type(), geojson::JSONGeometryType::Polygon);
    ASSERT_EQ(geometry.as_polygon().outer().size(), 5);
    ASSERT_EQ(geometry.as_polygon().inners().size(), 1);
    ASSERT_EQ(geometry.as_polygon().inners()[0].size(), 4);

    stream.str("");
    stream << data;
    boost::property_tree::ptree indirect_tree;
    boost::property_tree::json_parser::read_json(stream, indirect_tree);
    geojson::JSONGeometry indirect_geometry = geojson::JSONGeometry::from_ptree(indirect_tree);

    ASSERT_EQ(indirect_geometry.get_type(), geojson::JSONGeometryType::Polygon);
    ASSERT_EQ(indirect_geometry.as_polygon().outer().size(), 5);
    ASSERT_EQ(indirect_geometry.as_polygon().inners().size(), 1);
    ASSERT_EQ(indirect_geometry.as_polygon().inners()[0].size(), 4);

    geojson::JSONGeometry copied_geometry = geojson::JSONGeometry(geometry.as_polygon());

    ASSERT_EQ(copied_geometry.get_type(), geojson::JSONGeometryType::Polygon);
    ASSERT_EQ(copied_geometry.as_polygon().outer().size(), 5);
    ASSERT_EQ(copied_geometry.as_polygon().inners().size(), 1);
    ASSERT_EQ(copied_geometry.as_polygon().inners()[0].size(), 4);
}

TEST_F(JSONGeometry_Test, multipoint_test) {
    std::string data = "{ "
    "\"type\": \"MultiPoint\", "
    "\"coordinates\": [ "
        "[10, 40], [40, 30], [20, 20], [30, 10] "
    "] "
"}";

    std::stringstream stream;
    stream << data;
    boost::property_tree::ptree tree;
    boost::property_tree::json_parser::read_json(stream, tree);
    geojson::JSONGeometry geometry = geojson::JSONGeometry::of_multipoint(tree);

    ASSERT_EQ(geometry.get_type(), geojson::JSONGeometryType::MultiPoint);

    ASSERT_EQ(geometry.as_multipoint().size(), 4);

    ASSERT_EQ(geometry.as_multipoint()[0].get<0>(), 10);
    ASSERT_EQ(geometry.as_multipoint()[0].get<1>(), 40);
    
    ASSERT_EQ(geometry.as_multipoint()[1].get<0>(), 40);
    ASSERT_EQ(geometry.as_multipoint()[1].get<1>(), 30);
    
    ASSERT_EQ(geometry.as_multipoint()[2].get<0>(), 20);
    ASSERT_EQ(geometry.as_multipoint()[2].get<1>(), 20);
    
    ASSERT_EQ(geometry.as_multipoint()[3].get<0>(), 30);
    ASSERT_EQ(geometry.as_multipoint()[3].get<1>(), 10);

    stream.str("");
    stream << data;
    boost::property_tree::ptree indirect_tree;
    boost::property_tree::json_parser::read_json(stream, indirect_tree);
    geojson::JSONGeometry indirect_geometry = geojson::JSONGeometry::from_ptree(indirect_tree);

    ASSERT_EQ(indirect_geometry.get_type(), geojson::JSONGeometryType::MultiPoint);

    ASSERT_EQ(indirect_geometry.as_multipoint().size(), 4);

    ASSERT_EQ(indirect_geometry.as_multipoint()[0].get<0>(), 10);
    ASSERT_EQ(indirect_geometry.as_multipoint()[0].get<1>(), 40);
    
    ASSERT_EQ(indirect_geometry.as_multipoint()[1].get<0>(), 40);
    ASSERT_EQ(indirect_geometry.as_multipoint()[1].get<1>(), 30);
    
    ASSERT_EQ(indirect_geometry.as_multipoint()[2].get<0>(), 20);
    ASSERT_EQ(indirect_geometry.as_multipoint()[2].get<1>(), 20);
    
    ASSERT_EQ(indirect_geometry.as_multipoint()[3].get<0>(), 30);
    ASSERT_EQ(indirect_geometry.as_multipoint()[3].get<1>(), 10);

    geojson::JSONGeometry copied_geometry = geojson::JSONGeometry(geometry.as_multipoint());

    ASSERT_EQ(copied_geometry.get_type(), geojson::JSONGeometryType::MultiPoint);

    ASSERT_EQ(copied_geometry.as_multipoint().size(), 4);

    ASSERT_EQ(copied_geometry.as_multipoint()[0].get<0>(), 10);
    ASSERT_EQ(copied_geometry.as_multipoint()[0].get<1>(), 40);
    
    ASSERT_EQ(copied_geometry.as_multipoint()[1].get<0>(), 40);
    ASSERT_EQ(copied_geometry.as_multipoint()[1].get<1>(), 30);
    
    ASSERT_EQ(copied_geometry.as_multipoint()[2].get<0>(), 20);
    ASSERT_EQ(copied_geometry.as_multipoint()[2].get<1>(), 20);
    
    ASSERT_EQ(copied_geometry.as_multipoint()[3].get<0>(), 30);
    ASSERT_EQ(copied_geometry.as_multipoint()[3].get<1>(), 10);

    geojson::JSONGeometry copy = geojson::JSONGeometry(geometry);

    ASSERT_EQ(copy.get_type(), geojson::JSONGeometryType::MultiPoint);

    ASSERT_EQ(copy.as_multipoint().size(), 4);

    ASSERT_EQ(copy.as_multipoint()[0].get<0>(), 10);
    ASSERT_EQ(copy.as_multipoint()[0].get<1>(), 40);
    
    ASSERT_EQ(copy.as_multipoint()[1].get<0>(), 40);
    ASSERT_EQ(copy.as_multipoint()[1].get<1>(), 30);
    
    ASSERT_EQ(copy.as_multipoint()[2].get<0>(), 20);
    ASSERT_EQ(copy.as_multipoint()[2].get<1>(), 20);
    
    ASSERT_EQ(copy.as_multipoint()[3].get<0>(), 30);
    ASSERT_EQ(copy.as_multipoint()[3].get<1>(), 10);

}

TEST_F(JSONGeometry_Test, multilinestring_test) {
    std::string data = "{ "
       "\"type\": \"MultiLineString\", "
       "\"coordinates\": [ "
       "    [ "
       "        [170.0, 45.0], [180.0, 45.0] "
       "    ], [ "
       "        [-180.0, 45.0], [-170.0, 45.0] "
       "    ] "
       "] "
   "}";

    std::stringstream stream;
    stream << data;
    boost::property_tree::ptree tree;
    boost::property_tree::json_parser::read_json(stream, tree);
    geojson::JSONGeometry geometry = geojson::JSONGeometry::of_multilinestring(tree);

    ASSERT_EQ(geometry.get_type(), geojson::JSONGeometryType::MultiLineString);
    ASSERT_EQ(geometry.as_multilinestring().size(), 2);

    ASSERT_EQ(geometry.as_multilinestring()[0].size(), 2);

    ASSERT_EQ(geometry.as_multilinestring()[0][0].get<0>(), 170.0);
    ASSERT_EQ(geometry.as_multilinestring()[0][0].get<1>(), 45.0);
    
    ASSERT_EQ(geometry.as_multilinestring()[0][1].get<0>(), 180.0);
    ASSERT_EQ(geometry.as_multilinestring()[0][1].get<1>(), 45.0);
    
    ASSERT_EQ(geometry.as_multilinestring()[1].size(), 2);

    ASSERT_EQ(geometry.as_multilinestring()[1][0].get<0>(), -180.0);
    ASSERT_EQ(geometry.as_multilinestring()[1][0].get<1>(), 45.0);
    
    ASSERT_EQ(geometry.as_multilinestring()[1][1].get<0>(), -170.0);
    ASSERT_EQ(geometry.as_multilinestring()[1][1].get<1>(), 45.0);

    stream.str("");
    stream << data;
    boost::property_tree::ptree indirect_tree;
    boost::property_tree::json_parser::read_json(stream, indirect_tree);
    geojson::JSONGeometry indirect_geometry = geojson::JSONGeometry::from_ptree(indirect_tree);

    ASSERT_EQ(indirect_geometry.get_type(), geojson::JSONGeometryType::MultiLineString);
    ASSERT_EQ(indirect_geometry.as_multilinestring().size(), 2);

    ASSERT_EQ(indirect_geometry.as_multilinestring()[0].size(), 2);

    ASSERT_EQ(indirect_geometry.as_multilinestring()[0][0].get<0>(), 170.0);
    ASSERT_EQ(indirect_geometry.as_multilinestring()[0][0].get<1>(), 45.0);
    
    ASSERT_EQ(indirect_geometry.as_multilinestring()[0][1].get<0>(), 180.0);
    ASSERT_EQ(indirect_geometry.as_multilinestring()[0][1].get<1>(), 45.0);
    
    ASSERT_EQ(indirect_geometry.as_multilinestring()[1].size(), 2);

    ASSERT_EQ(indirect_geometry.as_multilinestring()[1][0].get<0>(), -180.0);
    ASSERT_EQ(indirect_geometry.as_multilinestring()[1][0].get<1>(), 45.0);
    
    ASSERT_EQ(indirect_geometry.as_multilinestring()[1][1].get<0>(), -170.0);
    ASSERT_EQ(indirect_geometry.as_multilinestring()[1][1].get<1>(), 45.0);

    geojson::JSONGeometry copied_geometry = geojson::JSONGeometry(geometry.as_multilinestring());

    ASSERT_EQ(copied_geometry.get_type(), geojson::JSONGeometryType::MultiLineString);
    ASSERT_EQ(copied_geometry.as_multilinestring().size(), 2);

    ASSERT_EQ(copied_geometry.as_multilinestring()[0].size(), 2);

    ASSERT_EQ(copied_geometry.as_multilinestring()[0][0].get<0>(), 170.0);
    ASSERT_EQ(copied_geometry.as_multilinestring()[0][0].get<1>(), 45.0);
    
    ASSERT_EQ(copied_geometry.as_multilinestring()[0][1].get<0>(), 180.0);
    ASSERT_EQ(copied_geometry.as_multilinestring()[0][1].get<1>(), 45.0);
    
    ASSERT_EQ(copied_geometry.as_multilinestring()[1].size(), 2);

    ASSERT_EQ(copied_geometry.as_multilinestring()[1][0].get<0>(), -180.0);
    ASSERT_EQ(copied_geometry.as_multilinestring()[1][0].get<1>(), 45.0);
    
    ASSERT_EQ(copied_geometry.as_multilinestring()[1][1].get<0>(), -170.0);
    ASSERT_EQ(copied_geometry.as_multilinestring()[1][1].get<1>(), 45.0);

    geojson::JSONGeometry copy = geojson::JSONGeometry(geometry);

    ASSERT_EQ(copy.get_type(), geojson::JSONGeometryType::MultiLineString);
    ASSERT_EQ(copy.as_multilinestring().size(), 2);

    ASSERT_EQ(copy.as_multilinestring()[0].size(), 2);

    ASSERT_EQ(copy.as_multilinestring()[0][0].get<0>(), 170.0);
    ASSERT_EQ(copy.as_multilinestring()[0][0].get<1>(), 45.0);
    
    ASSERT_EQ(copy.as_multilinestring()[0][1].get<0>(), 180.0);
    ASSERT_EQ(copy.as_multilinestring()[0][1].get<1>(), 45.0);
    
    ASSERT_EQ(copy.as_multilinestring()[1].size(), 2);

    ASSERT_EQ(copy.as_multilinestring()[1][0].get<0>(), -180.0);
    ASSERT_EQ(copy.as_multilinestring()[1][0].get<1>(), 45.0);
    
    ASSERT_EQ(copy.as_multilinestring()[1][1].get<0>(), -170.0);
    ASSERT_EQ(copy.as_multilinestring()[1][1].get<1>(), 45.0);
}

TEST_F(JSONGeometry_Test, multipolygon_test) {
    std::string data = "{ "
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
   "}";

    std::stringstream stream;
    stream << data;
    boost::property_tree::ptree tree;
    boost::property_tree::json_parser::read_json(stream, tree);
    geojson::JSONGeometry geometry = geojson::JSONGeometry::of_multipolygon(tree);

    ASSERT_EQ(geometry.get_type(), geojson::JSONGeometryType::MultiPolygon);
    ASSERT_EQ(geometry.as_multipolygon().size(), 2);

    ASSERT_EQ(geometry.as_multipolygon()[0].outer().size(), 5);
    ASSERT_EQ(geometry.as_multipolygon()[0].inners().size(), 0);

    ASSERT_EQ(geometry.as_multipolygon()[1].outer().size(), 5);
    ASSERT_EQ(geometry.as_multipolygon()[1].inners().size(), 0);

    stream.str("");
    stream << data;
    boost::property_tree::ptree indirect_tree;
    boost::property_tree::json_parser::read_json(stream, indirect_tree);
    geojson::JSONGeometry indirect_geometry = geojson::JSONGeometry::from_ptree(indirect_tree);

    ASSERT_EQ(indirect_geometry.get_type(), geojson::JSONGeometryType::MultiPolygon);
    ASSERT_EQ(indirect_geometry.as_multipolygon().size(), 2);

    ASSERT_EQ(indirect_geometry.as_multipolygon()[0].outer().size(), 5);
    ASSERT_EQ(indirect_geometry.as_multipolygon()[0].inners().size(), 0);

    ASSERT_EQ(indirect_geometry.as_multipolygon()[1].outer().size(), 5);
    ASSERT_EQ(indirect_geometry.as_multipolygon()[1].inners().size(), 0);

    geojson::JSONGeometry copied_geometry = geojson::JSONGeometry(geometry.as_multipolygon());

    ASSERT_EQ(copied_geometry.get_type(), geojson::JSONGeometryType::MultiPolygon);
    ASSERT_EQ(copied_geometry.as_multipolygon().size(), 2);

    ASSERT_EQ(copied_geometry.as_multipolygon()[0].outer().size(), 5);
    ASSERT_EQ(copied_geometry.as_multipolygon()[0].inners().size(), 0);

    ASSERT_EQ(copied_geometry.as_multipolygon()[1].outer().size(), 5);
    ASSERT_EQ(copied_geometry.as_multipolygon()[1].inners().size(), 0);

    geojson::JSONGeometry copy = geojson::JSONGeometry(geometry);

    ASSERT_EQ(copy.get_type(), geojson::JSONGeometryType::MultiPolygon);
    ASSERT_EQ(copy.as_multipolygon().size(), 2);

    ASSERT_EQ(copy.as_multipolygon()[0].outer().size(), 5);
    ASSERT_EQ(copy.as_multipolygon()[0].inners().size(), 0);

    ASSERT_EQ(copy.as_multipolygon()[1].outer().size(), 5);
    ASSERT_EQ(copy.as_multipolygon()[1].inners().size(), 0);
}
