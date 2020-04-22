#include "gtest/gtest.h"
#include <JSONGeometry.hpp>
#include <FeatureBuilder.hpp>
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
    auto point = geojson::point(x, y);

    ASSERT_EQ(point.get<0>(), x);
    ASSERT_EQ(point.get<1>(), y);

    std::string data = "{ "
               "\"type\": \"Point\", "
               "\"coordinates\": [102.0, 0.5] "
           "}";

    std::stringstream stream;
    stream << data;
    boost::property_tree::ptree tree;
    boost::property_tree::json_parser::read_json(stream, tree);
    auto geometry = geojson::get_shape<geojson::coordinate_t>(geojson::build_geometry(tree));
    ASSERT_EQ(geometry.get<0>(), x);
    ASSERT_EQ(geometry.get<1>(), y);

    stream.str("");
    stream << data;
    boost::property_tree::ptree indirect_tree;
    boost::property_tree::json_parser::read_json(stream, indirect_tree);
    auto indirect_geometry = geojson::build_geometry(indirect_tree);
    geometry = geojson::get_shape<geojson::coordinate_t>(indirect_geometry);
    ASSERT_EQ(geometry.get<0>(), x);
    ASSERT_EQ(geometry.get<1>(), y);
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
    auto read_geometry = geojson::build_geometry(tree);
    auto geometry = geojson::get_shape<geojson::linestring_t>(read_geometry);

    ASSERT_EQ(geometry.size(), 4);
    ASSERT_EQ(geometry[0].get<0>(), 102.0);
    ASSERT_EQ(geometry[0].get<1>(), 0.0);

    ASSERT_EQ(geometry[1].get<0>(), 103.0);
    ASSERT_EQ(geometry[1].get<1>(), 1.0);

    ASSERT_EQ(geometry[2].get<0>(), 104.0);
    ASSERT_EQ(geometry[2].get<1>(), 0.0);

    ASSERT_EQ(geometry[3].get<0>(), 105.0);
    ASSERT_EQ(geometry[3].get<1>(), 1.0);

    stream.str("");
    stream << data;
    boost::property_tree::ptree indirect_tree;
    boost::property_tree::json_parser::read_json(stream, indirect_tree);
    read_geometry = geojson::build_geometry(indirect_tree);
    geometry = geojson::get_shape<geojson::linestring_t>(read_geometry);

    ASSERT_EQ(geometry.size(), 4);
    ASSERT_EQ(geometry[0].get<0>(), 102.0);
    ASSERT_EQ(geometry[0].get<1>(), 0.0);

    ASSERT_EQ(geometry[1].get<0>(), 103.0);
    ASSERT_EQ(geometry[1].get<1>(), 1.0);

    ASSERT_EQ(geometry[2].get<0>(), 104.0);
    ASSERT_EQ(geometry[2].get<1>(), 0.0);

    ASSERT_EQ(geometry[3].get<0>(), 105.0);
    ASSERT_EQ(geometry[3].get<1>(), 1.0);
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
    auto read_geometry = geojson::build_geometry(tree);
    auto geometry = geojson::get_shape<geojson::polygon_t>(read_geometry);

    ASSERT_EQ(geometry.outer().size(), 5);
    ASSERT_EQ(geometry.inners().size(), 1);
    ASSERT_EQ(geometry.inners()[0].size(), 4);

    stream.str("");
    stream << data;
    boost::property_tree::ptree indirect_tree;
    boost::property_tree::json_parser::read_json(stream, indirect_tree);
    read_geometry = geojson::build_geometry(indirect_tree);
    geometry = geojson::get_shape<geojson::polygon_t>(read_geometry);

    ASSERT_EQ(geometry.outer().size(), 5);
    ASSERT_EQ(geometry.inners().size(), 1);
    ASSERT_EQ(geometry.inners()[0].size(), 4);
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
    auto read_geometry = geojson::build_geometry(tree);
    auto geometry = geojson::get_shape<geojson::multipoint_t>(read_geometry);

    ASSERT_EQ(geometry.size(), 4);

    ASSERT_EQ(geometry[0].get<0>(), 10);
    ASSERT_EQ(geometry[0].get<1>(), 40);
    
    ASSERT_EQ(geometry[1].get<0>(), 40);
    ASSERT_EQ(geometry[1].get<1>(), 30);
    
    ASSERT_EQ(geometry[2].get<0>(), 20);
    ASSERT_EQ(geometry[2].get<1>(), 20);
    
    ASSERT_EQ(geometry[3].get<0>(), 30);
    ASSERT_EQ(geometry[3].get<1>(), 10);

    stream.str("");
    stream << data;
    boost::property_tree::ptree indirect_tree;
    boost::property_tree::json_parser::read_json(stream, indirect_tree);
    read_geometry = geojson::build_geometry(tree);
    geometry = geojson::get_shape<geojson::multipoint_t>(read_geometry);

    ASSERT_EQ(geometry.size(), 4);

    ASSERT_EQ(geometry[0].get<0>(), 10);
    ASSERT_EQ(geometry[0].get<1>(), 40);
    
    ASSERT_EQ(geometry[1].get<0>(), 40);
    ASSERT_EQ(geometry[1].get<1>(), 30);
    
    ASSERT_EQ(geometry[2].get<0>(), 20);
    ASSERT_EQ(geometry[2].get<1>(), 20);
    
    ASSERT_EQ(geometry[3].get<0>(), 30);
    ASSERT_EQ(geometry[3].get<1>(), 10);

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
    auto read_geometry = geojson::build_geometry(tree);
    auto geometry = geojson::get_shape<geojson::multilinestring_t>(read_geometry);

    ASSERT_EQ(geometry.size(), 2);

    ASSERT_EQ(geometry[0].size(), 2);

    ASSERT_EQ(geometry[0][0].get<0>(), 170.0);
    ASSERT_EQ(geometry[0][0].get<1>(), 45.0);
    
    ASSERT_EQ(geometry[0][1].get<0>(), 180.0);
    ASSERT_EQ(geometry[0][1].get<1>(), 45.0);
    
    ASSERT_EQ(geometry[1].size(), 2);

    ASSERT_EQ(geometry[1][0].get<0>(), -180.0);
    ASSERT_EQ(geometry[1][0].get<1>(), 45.0);
    
    ASSERT_EQ(geometry[1][1].get<0>(), -170.0);
    ASSERT_EQ(geometry[1][1].get<1>(), 45.0);

    stream.str("");
    stream << data;
    boost::property_tree::ptree indirect_tree;
    boost::property_tree::json_parser::read_json(stream, indirect_tree);
    read_geometry = geojson::build_geometry(tree);
    geometry = geojson::get_shape<geojson::multilinestring_t>(read_geometry);
    
    ASSERT_EQ(geometry.size(), 2);

    ASSERT_EQ(geometry[0].size(), 2);

    ASSERT_EQ(geometry[0][0].get<0>(), 170.0);
    ASSERT_EQ(geometry[0][0].get<1>(), 45.0);
    
    ASSERT_EQ(geometry[0][1].get<0>(), 180.0);
    ASSERT_EQ(geometry[0][1].get<1>(), 45.0);
    
    ASSERT_EQ(geometry[1].size(), 2);

    ASSERT_EQ(geometry[1][0].get<0>(), -180.0);
    ASSERT_EQ(geometry[1][0].get<1>(), 45.0);
    
    ASSERT_EQ(geometry[1][1].get<0>(), -170.0);
    ASSERT_EQ(geometry[1][1].get<1>(), 45.0);
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
    auto read_geometry = geojson::build_geometry(tree);
    auto geometry = geojson::get_shape<geojson::multipolygon_t>(read_geometry);
    
    ASSERT_EQ(geometry.size(), 2);

    ASSERT_EQ(geometry[0].outer().size(), 5);
    ASSERT_EQ(geometry[0].inners().size(), 0);

    ASSERT_EQ(geometry[1].outer().size(), 5);
    ASSERT_EQ(geometry[1].inners().size(), 0);

    stream.str("");
    stream << data;
    boost::property_tree::ptree indirect_tree;
    boost::property_tree::json_parser::read_json(stream, indirect_tree);
    read_geometry = geojson::build_geometry(indirect_tree);
    geometry = geojson::get_shape<geojson::multipolygon_t>(read_geometry);
    
    ASSERT_EQ(geometry.size(), 2);

    ASSERT_EQ(geometry[0].outer().size(), 5);
    ASSERT_EQ(geometry[0].inners().size(), 0);

    ASSERT_EQ(geometry[1].outer().size(), 5);
    ASSERT_EQ(geometry[1].inners().size(), 0);
}
