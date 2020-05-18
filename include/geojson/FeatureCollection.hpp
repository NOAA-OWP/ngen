#ifndef GEOJSON_FEATURE_COLLECTION_H
#define GEOJSON_FEATURE_COLLECTION_H

#include <features/Features.hpp>

#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <exception>
#include <memory>
#include <algorithm>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace geojson {
    class FeatureCollection {
        public:
            /**
             * Constructor
             * 
             * @param features An overall list of features
             * @param bounding_box A set of bounds for all features
             */
            FeatureCollection(FeatureList &new_features, std::vector<double> bounding_box) : bounding_box(bounding_box)
            {
                for (Feature feature : new_features) {
                    features.push_back(feature);
                }
            }

            /**
             * Copy constructor
             * 
             * @param feature_collection The FeatureCollection to copy
             */
            FeatureCollection(const FeatureCollection &feature_collection) {
                bounding_box = feature_collection.get_bounding_box();
                for (Feature feature : feature_collection) {
                    features.push_back(feature);
                }
            }

            /**
             * Destructor
             */
            virtual ~FeatureCollection(){};

            /**
             * @return The number of elements within the collection
             */
            int get_size();

            /**
             * @return Whether or not the collection is empty
             */
            bool is_empty();

            std::vector<double> get_bounding_box() const;

            /**
             * Retrieve Feature by index in collection
             * 
             * @param index The index of the feature
             * @return The Feature at the given index
             */
            Feature get_feature(int index) const;

            /**
             * Finds the index of given Feature in the collection
             * 
             * @param feature The feature to look for
             * @return -1 if the feature isn't in the collection, the numerical index otherwise
             */
            int find(Feature feature);

            /**
             * Finds the index of a Feature with the given ID
             * 
             * @param ID The ID of the Feature to look for
             * @return -1 if the feature isn't in the collection, the numerical index otherwise
             */
            int find(std::string ID);

            /**
             * Removes a feature from the collection based on index
             * 
             * @param index The index of the Feature to remove
             * @return The Feature that was removed
             */
            Feature remove_feature(int index);

            /**
             * Removes a Feature based on its ID
             * 
             * @param ID The ID of the Feature to remove
             * @return The removed Feature; null if a Feature wasn't found 
             */
            Feature remove_feature_by_id(std::string ID);

            /**
             * Retrieves a Feature based on its ID
             * 
             * @param ID The ID of the Feature to retrieve
             * @return The feature with the given ID; null if a Feature with that ID isn't present
             */
            Feature get_feature(std::string id) const;

            FeatureList::const_iterator begin() const;

            FeatureList::const_iterator end() const;

            JSONProperty get(std::string key) const;

            void visit_features(FeatureVisitor& visitor);

            void set(std::string key, short value);

            void set(std::string key, int value);

            void set(std::string key, long value);

            void set(std::string key, float value);

            void set(std::string key, double value);

            void set(std::string key, std::string value);

            void set(std::string key, JSONProperty& property);

            void add_feature(Feature feature, std::string *id = nullptr);

            /**
             * Add a reference to a feature by id
             * 
             * @param id The id used to reference a feature
             * @param feature A feature that may be referred to by id
             */
            void add_feature_id(std::string id, Feature feature);

            void set_ids_from_member(std::string member_name = "id");

            void set_ids_from_property(std::string property_name = "id");

            void set_ids(std::string id_field_name = "id");

            void update_ids();

            int link_features_from_property(std::string* from_property = nullptr, std::string* to_property = nullptr);

            int link_features_from_attribute(std::string* from_attribute = nullptr, std::string* to_attribute = nullptr);
        private:
            FeatureList features;
            std::vector<double> bounding_box;
            std::map<std::string, Feature> feature_by_id;
            std::map<std::string, JSONProperty> foreign_members;
    };
}

#endif // GEOJSON_FEATURE_COLLECTION_H