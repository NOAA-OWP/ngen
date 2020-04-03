#ifndef GEOJSON_FEATURE_COLLECTION_H
#define GEOJSON_FEATURE_COLLECTION_H

#include "Feature.hpp"
#include <memory>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <typeinfo>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace geojson {
    class FeatureCollection {
        public:
            typedef std::vector<Feature> FeatureList;

            FeatureCollection(FeatureList &features, std::vector<double> bounding_box)
            : features(features),
                bounding_box(bounding_box)
            {}

            FeatureCollection(const FeatureCollection &feature_collection);

            virtual ~FeatureCollection(){};

            typedef FeatureList::iterator iterator;

            typedef FeatureList::const_iterator const_iterator;

            int get_size();;

            std::vector<double> get_bounding_box() const;

            Feature get_feature(int index) const;;

            const_iterator begin() const;

            const_iterator end() const;

            static FeatureCollection read(const std::string &file_path);

            static FeatureCollection read(std::stringstream &data);

            static FeatureCollection parse(const boost::property_tree::ptree json);

        private:
            FeatureList features;
            std::vector<double> bounding_box;
    };
}

#endif // GEOJSON_FEATURE_COLLECTION_H