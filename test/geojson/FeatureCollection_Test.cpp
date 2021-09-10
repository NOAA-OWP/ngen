#include "gtest/gtest.h"
#include <FeatureCollection.hpp>
#include <features/Features.hpp>
#include <FeatureBuilder.hpp>
#include <FeatureVisitor.hpp>
#include <vector>
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

class Visitor : public geojson::FeatureVisitor {
    public:
        void visit(geojson::PointFeature *feature) {
            this->types.push_back("PointFeature");
        }

        void visit(geojson::LineStringFeature *feature) {
            this->types.push_back("LineStringFeature");
        }

        void visit(geojson::PolygonFeature *feature) {
            this->types.push_back("PolygonFeature");
        }

        void visit(geojson::MultiPointFeature *feature) {
            this->types.push_back("MultiPointFeature");
        }

        void visit(geojson::MultiLineStringFeature *feature) {
            this->types.push_back("MultiLineStringFeature");
        }

        void visit(geojson::MultiPolygonFeature *feature) {
            this->types.push_back("MultiPolygonFeature");
        }

        void visit(geojson::CollectionFeature *feature) {
            this->types.push_back("CollectionFeature");
        }

        std::string get(int index) {
            if( index >= 0 && index < types.size() )
              return this->types[index];
            else
              return "NULL";
        }

    private:
        std::vector<std::string> types;
};

TEST_F(FeatureCollection_Test, ptree_test) {
    std::string data = "{ "
        "\"type\": \"FeatureCollection\", "
        "\"bbox\": [1, 2, 3, 4, 5 ], "
        "\"features\": [ "
            "{ "
                "\"type\": \"Feature\", "
                "\"id\": \"First\", "
                "\"geometry\": { "
                "    \"type\": \"Point\", "
                "    \"coordinates\": [102.0, 0.5] "
                "} "
            "}, "
            "{ "
                "\"type\": \"Feature\", "
                "\"id\": \"Second\", "
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

    geojson::GeoJSON collection = geojson::read(stream);
    std::vector<double> bbox = collection->get_bounding_box();
    ASSERT_EQ(bbox.size(), 5);
    ASSERT_EQ(bbox[0], 1.0);
    ASSERT_EQ(bbox[1], 2.0);
    ASSERT_EQ(bbox[2], 3.0);
    ASSERT_EQ(bbox[3], 4.0);
    ASSERT_EQ(bbox[4], 5.0);
    ASSERT_EQ(2, collection->get_size());

    geojson::Feature first = collection->get_feature(0);
    geojson::Feature second = collection->get_feature(1);
    
    ASSERT_EQ(first->get_id(), "First");
    
    ASSERT_EQ(first->get_type(), geojson::FeatureType::Point);
    ASSERT_EQ(second->get_type(), geojson::FeatureType::LineString);

    ASSERT_TRUE(first->is_leaf());
    ASSERT_TRUE(first->is_root());

    ASSERT_TRUE(second->is_leaf());
    ASSERT_TRUE(second->is_root());

    ASSERT_EQ(collection->get_feature("First"), first);
    ASSERT_EQ(collection->get_feature("Second"), second);

    Visitor visitor;

    collection->visit_features(visitor);

    ASSERT_EQ(visitor.get(0), "PointFeature");
    ASSERT_EQ(visitor.get(1), "LineStringFeature");
}

TEST_F(FeatureCollection_Test, subset_test) {
    std::string data = "{ "
        "\"type\": \"FeatureCollection\", "
        "\"bbox\": [1, 2, 3, 4, 5 ], "
        "\"features\": [ "
            "{ "
                "\"type\": \"Feature\", "
                "\"id\": \"First\", "
                "\"geometry\": { "
                "    \"type\": \"Point\", "
                "    \"coordinates\": [102.0, 0.5] "
                "} "
            "}, "
            "{ "
                "\"type\": \"Feature\", "
                "\"id\": \"Second\", "
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
    std::vector<std::string> subset = {"First"};

    geojson::GeoJSON collection = geojson::read(stream, subset);

    std::vector<double> bbox = collection->get_bounding_box();
    ASSERT_EQ(bbox.size(), 5);
    ASSERT_EQ(bbox[0], 1.0);
    ASSERT_EQ(bbox[1], 2.0);
    ASSERT_EQ(bbox[2], 3.0);
    ASSERT_EQ(bbox[3], 4.0);
    ASSERT_EQ(bbox[4], 5.0);
    //Should only have the first feature in the collection
    ASSERT_EQ(1, collection->get_size());

    geojson::Feature first = collection->get_feature(0);
    geojson::Feature second = collection->get_feature(1);

    ASSERT_EQ(first->get_id(), "First");

    ASSERT_TRUE(second == nullptr);
    ASSERT_EQ(first->get_type(), geojson::FeatureType::Point);

    ASSERT_TRUE(first->is_leaf());
    ASSERT_TRUE(first->is_root());

    ASSERT_EQ(collection->get_feature("First"), first);
    ASSERT_EQ(collection->get_feature("Second"), nullptr);

    Visitor visitor;

    collection->visit_features(visitor);

    ASSERT_EQ(visitor.get(0), "PointFeature");
    ASSERT_EQ(visitor.get(1), "NULL");
}

TEST_F(FeatureCollection_Test, copy_test) {
    std::string data = "{ "
        "\"type\": \"FeatureCollection\", "
        "\"bbox\": [1, 2, 3, 4, 5 ], "
        "\"features\": [ "
            "{ "
                "\"type\": \"Feature\", "
                "\"id\": \"First\", "
                "\"geometry\": { "
                "    \"type\": \"Point\", "
                "    \"coordinates\": [102.0, 0.5] "
                "} "
            "}, "
            "{ "
                "\"type\": \"Feature\", "
                "\"id\": \"Second\", "
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

    geojson::GeoJSON orig_collection = geojson::read(stream);
    geojson::GeoJSON collection = std::make_shared<geojson::FeatureCollection>(*orig_collection);

    std::vector<double> bbox = collection->get_bounding_box();
    ASSERT_EQ(bbox.size(), 5);
    ASSERT_EQ(bbox[0], 1.0);
    ASSERT_EQ(bbox[1], 2.0);
    ASSERT_EQ(bbox[2], 3.0);
    ASSERT_EQ(bbox[3], 4.0);
    ASSERT_EQ(bbox[4], 5.0);
    ASSERT_EQ(2, collection->get_size());

    geojson::Feature first = collection->get_feature(0);
    geojson::Feature second = collection->get_feature(1);
    
    ASSERT_EQ(first->get_id(), "First");
    
    ASSERT_EQ(first->get_type(), geojson::FeatureType::Point);
    ASSERT_EQ(second->get_type(), geojson::FeatureType::LineString);

    ASSERT_TRUE(first->is_leaf());
    ASSERT_TRUE(first->is_root());

    ASSERT_TRUE(second->is_leaf());
    ASSERT_TRUE(second->is_root());

    ASSERT_EQ(collection->get_feature("First"), first);
    ASSERT_EQ(collection->get_feature("Second"), second);

    Visitor visitor;

    collection->visit_features(visitor);

    ASSERT_EQ(visitor.get(0), "PointFeature");
    ASSERT_EQ(visitor.get(1), "LineStringFeature");
}

TEST_F(FeatureCollection_Test, copy_filter_test) {
    std::string data = "{ "
        "\"type\": \"FeatureCollection\", "
        "\"bbox\": [1, 2, 3, 4, 5 ], "
        "\"features\": [ "
            "{ "
                "\"type\": \"Feature\", "
                "\"id\": \"First\", "
                "\"geometry\": { "
                "    \"type\": \"Point\", "
                "    \"coordinates\": [102.0, 0.5] "
                "} "
            "}, "
            "{ "
                "\"type\": \"Feature\", "
                "\"id\": \"Second\", "
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
        
    std::vector<std::string> filter = { "Second" };

    std::stringstream stream;
    stream << data;

    geojson::GeoJSON orig_collection = geojson::read(stream);
    geojson::GeoJSON collection = std::make_shared<geojson::FeatureCollection>(*orig_collection, filter);
    
    std::vector<double> bbox = collection->get_bounding_box();
    ASSERT_EQ(bbox.size(), 5);
    ASSERT_EQ(bbox[0], 1.0);
    ASSERT_EQ(bbox[1], 2.0);
    ASSERT_EQ(bbox[2], 3.0);
    ASSERT_EQ(bbox[3], 4.0);
    ASSERT_EQ(bbox[4], 5.0);
    ASSERT_EQ(1, collection->get_size());

    geojson::Feature second = collection->get_feature(0);
    
    ASSERT_EQ(second->get_id(), "Second");
    
    ASSERT_EQ(second->get_type(), geojson::FeatureType::LineString);

    ASSERT_TRUE(second->is_leaf());
    ASSERT_TRUE(second->is_root());

    ASSERT_EQ(collection->get_feature("Second"), second);

    Visitor visitor;

    collection->visit_features(visitor);

    ASSERT_EQ(visitor.get(0), "LineStringFeature");
}
