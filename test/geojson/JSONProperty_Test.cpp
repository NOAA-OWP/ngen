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

TEST_F(JSONProperty_Test, property_test) {
    geojson::JSONProperty property("test", 4);
    geojson::PropertyType type = property.get_type();
    ASSERT_TRUE(type == geojson::PropertyType::Natural);
}