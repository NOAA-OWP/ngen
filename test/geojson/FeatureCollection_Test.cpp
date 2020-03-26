#include "gtest/gtest.h"
#include <FeatureCollection.hpp>
#include <iostream>

class FeatureCollection_Test : public ::testing::Test {

    protected:


    FeatureCollection_Test() {

    }

    ~FeatureCollection_Test() override {

    }

    void SetUp() override {};

    void TearDown() override {};

};

TEST_F(FeatureCollection_Test, ptree_test) {
    std::string data = "{ "
        "\"type\": \"FeatureCollection\", "
        "\"bbox\": [1, 2, 3, 4, 5 ], "
        "\"features\": [ "
            "{ "
                "\"type\": \"Feature\", "
                "\"geometry\": { "
                "    \"type\": \"Point\", "
                "    \"coordinates\": [102.0, 0.5] "
                "} "
            "}, "
            "{ "
                "\"type\": \"Feature\", "
                "\"geometry\": { "
                    "\"type\": \"LineString\", "
                    "\"coordinates\": [ "
                        "[102.0, 0.0], "
                        "[103.0, 1.0], "
                        "[104.0, 0.0], "
                        "[105.0, 1.0] "
                    "] "
                "} "
            "} "
        "] "
        "}";
        
    std::stringstream stream;
    stream << data;

    geojson::FeatureCollection collection = geojson::FeatureCollection::read(stream);
    std::vector<double> bbox = collection.get_bounding_box();
    ASSERT_EQ(bbox.size(), 5);
    ASSERT_EQ(bbox[0], 1.0);
    ASSERT_EQ(bbox[1], 2.0);
    ASSERT_EQ(bbox[2], 3.0);
    ASSERT_EQ(bbox[3], 4.0);
    ASSERT_EQ(bbox[4], 5.0);
    ASSERT_EQ(2, collection.get_size());

    ASSERT_EQ(collection.get_feature(0).get_type(), geojson::FeatureType::Point);
    ASSERT_EQ(collection.get_feature(1).get_type(), geojson::FeatureType::LineString);
}