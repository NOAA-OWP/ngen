#include "FeatureCollection.hpp"

geojson::FeatureCollection::FeatureCollection(const geojson::FeatureCollection &feature_collection) {
    bounding_box = feature_collection.get_bounding_box();
    for (Feature feature : feature_collection) {
        features.push_back(feature);
    }
}

int geojson::FeatureCollection::get_size() {
    return features.size();
}

std::vector<double> geojson::FeatureCollection::get_bounding_box() const {
    return bounding_box;
}

geojson::Feature geojson::FeatureCollection::get_feature(int index) const {
    return features[index];
}

geojson::FeatureCollection::const_iterator geojson::FeatureCollection::begin() const {
    return features.cbegin();
}

geojson::FeatureCollection::const_iterator geojson::FeatureCollection::end() const {
    return features.cend();
}

geojson::FeatureCollection geojson::FeatureCollection::read(const std::string &file_path) {
    boost::property_tree::ptree tree;
    boost::property_tree::json_parser::read_json(file_path, tree);
    return FeatureCollection::parse(tree);
}

geojson::FeatureCollection geojson::FeatureCollection::read(std::stringstream &data) {
    boost::property_tree::ptree tree;
    boost::property_tree::json_parser::read_json(data, tree);
    return FeatureCollection::parse(tree);
}

geojson::FeatureCollection geojson::FeatureCollection::parse(const boost::property_tree::ptree json) {
    std::vector<std::string> bounding_box;
    for (auto &feature : json.get_child("bbox")) {
        bounding_box.push_back(feature.second.data());
    }
    std::vector<double> bbox_values;

    for (int point_index = 0; point_index < bounding_box.size(); point_index++) {
        bbox_values.push_back(std::stod(bounding_box[point_index]));
    }

    FeatureList features;
    boost::optional<const boost::property_tree::ptree&> e = json.get_child_optional("features");

    if (e) {
        for(auto feature_tree : *e) {
            features.push_back(Feature(feature_tree.second));
        }
    }
    else {
        std::cout << "No features were found" << std::endl;
    }

    return FeatureCollection(features, bbox_values);
}
