#ifndef GEOJSON_FEATURE_COLLECTION_H
#define GEOJSON_FEATURE_COLLECTION_H

#include <features/Features.hpp>

#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <exception>
#include <memory>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace geojson {
    class FeatureCollection {
        public:
            /**
             * Add a reference to a feature by id
             * 
             * @param id The id used to reference a feature
             * @param feature A feature that may be referred to by id
             */
            void add_feature_id(std::string id, Feature feature) {
                if (id != "") {
                    feature_by_id.emplace(id, feature);
                }
                else {
                    throw std::invalid_argument("id");
                }
            }

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

            int get_size();

            std::vector<double> get_bounding_box() const;

            Feature get_feature(int index) const;

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