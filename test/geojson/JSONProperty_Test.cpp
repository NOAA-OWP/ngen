#include "gtest/gtest.h"
#include <JSONProperty.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

class JSONProperty_Test : public ::testing::Test {

    protected:


    JSONProperty_Test() {

    }

    ~JSONProperty_Test() override {

    }

    void SetUp() override {

    }

    void TearDown() override {

    }

};

TEST_F(JSONProperty_Test, natural_property_test) {
    geojson::JSONProperty natural_property("natural", 4);
    ASSERT_EQ(natural_property.get_type(), geojson::PropertyType::Natural);
    ASSERT_EQ(natural_property.as_natural_number(), 4);
}

TEST_F(JSONProperty_Test, real_property_test) {
    geojson::JSONProperty real_property("real", 3.0);
    ASSERT_EQ(real_property.get_type(), geojson::PropertyType::Real);
    ASSERT_EQ(real_property.as_real_number(), 3.0);
}

TEST_F(JSONProperty_Test, string_property_test) {
    geojson::JSONProperty string_property("string", "test_string");
    ASSERT_EQ(string_property.get_type(), geojson::PropertyType::String);
    ASSERT_EQ(string_property.as_string(), "test_string");
}

TEST_F(JSONProperty_Test, boolean_property_test) {
    geojson::JSONProperty boolean_property("boolean", true);
    ASSERT_EQ(boolean_property.get_type(), geojson::PropertyType::Boolean);
    ASSERT_EQ(boolean_property.as_boolean(), true);
}

TEST_F(JSONProperty_Test, object_property_test) {
    geojson::PropertyMap object;
    
    object.emplace("natural", geojson::JSONProperty("natural", 4));
    object.emplace("real", geojson::JSONProperty("real", 3.0));
    object.emplace("string", geojson::JSONProperty("string", "test_string"));
    object.emplace("boolean", geojson::JSONProperty("boolean", true));

    geojson::JSONProperty object_property("object", object);
    ASSERT_EQ(object_property.get_values().size(), 4);
    ASSERT_EQ(object_property.at("natural").get_type(), geojson::PropertyType::Natural);
    ASSERT_EQ(object_property.at("natural").as_natural_number(), 4);
}

TEST_F(JSONProperty_Test, list_property_test) {
    std::vector<geojson::JSONProperty> properties;

    properties.push_back(geojson::JSONProperty("natural", 4));
    properties.push_back(geojson::JSONProperty("real", 3.0));
    properties.push_back(geojson::JSONProperty("string", "test_string"));
    properties.push_back(geojson::JSONProperty("boolean", true));

    geojson::JSONProperty list_property("list_property", properties);

    ASSERT_EQ(list_property.as_list()[0].as_natural_number(), properties[0].as_natural_number());
    ASSERT_EQ(list_property.as_list()[1].as_real_number(), properties[1].as_real_number());
    ASSERT_EQ(list_property.as_list()[2].as_string(), properties[2].as_string());
    ASSERT_EQ(list_property.as_list()[3].as_boolean(), properties[3].as_boolean());
}

TEST_F(JSONProperty_Test, natural_vector_test) {
    std::vector<geojson::JSONProperty> properties;

    properties.push_back(geojson::JSONProperty("one", 1));
    properties.push_back(geojson::JSONProperty("two", 2));
    properties.push_back(geojson::JSONProperty("three", 3));

    geojson::JSONProperty list_property("list_property", properties);

    std::vector<long> values = list_property.as_natural_vector();

    ASSERT_EQ(values.size(), 3);
    ASSERT_EQ(values.size(), list_property.as_list().size());
    ASSERT_EQ(values[0], 1);
    ASSERT_EQ(values[1], 2);
    ASSERT_EQ(values[2], 3);
}

TEST_F(JSONProperty_Test, real_vector_test) {
    std::vector<geojson::JSONProperty> properties;

    properties.push_back(geojson::JSONProperty("one", 1.0));
    properties.push_back(geojson::JSONProperty("two", 2.0));
    properties.push_back(geojson::JSONProperty("three", 3.0));

    geojson::JSONProperty list_property("list_property", properties);

    std::vector<double> values = list_property.as_real_vector();

    ASSERT_EQ(values.size(), 3);
    ASSERT_EQ(values.size(), list_property.as_list().size());
    ASSERT_EQ(values[0], 1.0);
    ASSERT_EQ(values[1], 2.0);
    ASSERT_EQ(values[2], 3.0);
}

TEST_F(JSONProperty_Test, boolean_vector_test) {
    std::vector<geojson::JSONProperty> properties;

    properties.push_back(geojson::JSONProperty("one", true));
    properties.push_back(geojson::JSONProperty("two", false));
    properties.push_back(geojson::JSONProperty("three", true));

    geojson::JSONProperty list_property("list_property", properties);

    std::vector<bool> values = list_property.as_boolean_vector();

    ASSERT_EQ(values.size(), 3);
    ASSERT_EQ(values.size(), list_property.as_list().size());
    ASSERT_EQ(values[0], true);
    ASSERT_EQ(values[1], false);
    ASSERT_EQ(values[2], true);
}

TEST_F(JSONProperty_Test, string_vector_test) {
    std::vector<geojson::JSONProperty> properties;

    properties.push_back(geojson::JSONProperty("one", "one"));
    properties.push_back(geojson::JSONProperty("two", "two"));
    properties.push_back(geojson::JSONProperty("three", "three"));

    geojson::JSONProperty list_property("list_property", properties);

    std::vector<std::string> values = list_property.as_string_vector();

    ASSERT_EQ(values.size(), 3);
    ASSERT_EQ(values.size(), list_property.as_list().size());
    ASSERT_EQ(values[0], "one");
    ASSERT_EQ(values[1], "two");
    ASSERT_EQ(values[2], "three");
}

TEST_F(JSONProperty_Test, ptree_test) {
    std::string data = "{ "
               "\"type\": \"LineString\", "
               "\"coordinates\": [0.0,1.0,3.0], "
               "\"boolean_value\": true, " 
               "\"natural_value\": 4, "
               "\"real_value\": 4.3123 "
           "}";

    std::stringstream stream;
    stream << data;
    boost::property_tree::ptree tree;
    boost::property_tree::json_parser::read_json(stream, tree);

    std::vector<geojson::JSONProperty> properties;
    for(auto &pair : tree) {
        properties.push_back(geojson::JSONProperty(pair.first, pair.second));
    }

    ASSERT_EQ(properties.size(), 5);
    ASSERT_EQ(properties[0].get_type(), geojson::PropertyType::String);
    ASSERT_EQ(properties[0].as_string(), "LineString");

    ASSERT_EQ(properties[1].get_type(), geojson::PropertyType::List);
    ASSERT_EQ(properties[1].as_list().size(), 3);
    ASSERT_EQ(properties[1].as_list()[0].as_real_number(), 0.0);
    ASSERT_EQ(properties[1].as_list()[1].as_real_number(), 1.0);
    ASSERT_EQ(properties[1].as_list()[2].as_real_number(), 3.0);

    ASSERT_EQ(properties[2].get_type(), geojson::PropertyType::Boolean);
    ASSERT_EQ(properties[3].get_type(), geojson::PropertyType::Natural);
    ASSERT_EQ(properties[4].get_type(), geojson::PropertyType::Real);
}