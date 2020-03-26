#include "gtest/gtest.h"
#include <JSONProperty.hpp>

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
    std::map<std::string, geojson::JSONProperty> object;
    
    object.emplace("natural", geojson::JSONProperty("natural", 4));
    object.emplace("real", geojson::JSONProperty("real", 3.0));
    object.emplace("string", geojson::JSONProperty("string", "test_string"));
    object.emplace("boolean", geojson::JSONProperty("boolean", true));

    geojson::JSONProperty object_property("object", object);
    ASSERT_EQ(object_property.get_values().size(), 4);
    ASSERT_EQ(object_property.at("natural").get_type(), geojson::PropertyType::Natural);
    ASSERT_EQ(object_property.at("natural").as_natural_number(), 4);
}