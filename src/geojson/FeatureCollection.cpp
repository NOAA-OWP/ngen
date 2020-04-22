#include "FeatureCollection.hpp"
#include "features/Features.hpp"

using namespace geojson;

Feature FeatureCollection::get_feature(int index) const {
    return features[index];
}

int FeatureCollection::get_size() {
    return features.size();
};

std::vector<double> FeatureCollection::get_bounding_box() const {
    return bounding_box;
}

Feature FeatureCollection::get_feature(std::string id) const {
    return feature_by_id.at(id);
}

FeatureList::const_iterator FeatureCollection::begin() const {
    return features.cbegin();
}

FeatureList::const_iterator FeatureCollection::end() const {
    return features.cend();
}

JSONProperty FeatureCollection::get(std::string key) const {
    return foreign_members.at(key);
}

void FeatureCollection::visit_features(FeatureVisitor& visitor) {
    for (Feature feature : this->features) {
        feature->visit(visitor);
    }
}

void FeatureCollection::set(std::string key, short value) {
    foreign_members.emplace(key, JSONProperty(key, value));
}

void FeatureCollection::set(std::string key, int value) {
    foreign_members.emplace(key, JSONProperty(key, value));
}

void FeatureCollection::set(std::string key, long value) {
    foreign_members.emplace(key, JSONProperty(key, value));
}

void FeatureCollection::set(std::string key, float value) {
    foreign_members.emplace(key, JSONProperty(key, value));
}

void FeatureCollection::set(std::string key, double value) {
    foreign_members.emplace(key, JSONProperty(key, value));
}

void FeatureCollection::set(std::string key, std::string value) {
    foreign_members.emplace(key, JSONProperty(key, value));
}

void FeatureCollection::set(std::string key, JSONProperty& property) {
    foreign_members.emplace(key, property);
}

int FeatureCollection::link_features_from_property(std::string* from_property, std::string* to_property) {
    int links_found = 0;

    for (Feature feature : features) {
        if (from_property != nullptr) {
            std::string from_id = feature->get_property(*from_property).as_string();
            
            if (feature_by_id.find(from_id) != feature_by_id.end()) {
                feature->add_upstream_feature(feature_by_id[from_id].get());
                links_found++;
            }
        }

        if (to_property != nullptr) {
            std::string to_id = feature->get_property(*to_property).as_string();

            if (feature_by_id.find(to_id) != feature_by_id.end()) {
                feature->add_downstream_feature(feature_by_id[to_id].get());
                links_found++;
            }
        }
    }

    return links_found;
}

int FeatureCollection::link_features_from_attribute(std::string* from_attribute, std::string* to_attribute) {
    int links_found = 0;

    for (Feature feature : features) {
        if (from_attribute != nullptr) {
            std::string from_id = feature->get(*from_attribute).as_string();
            
            if (from_id != "null" && feature_by_id.find(from_id) != feature_by_id.end()) {
                feature->add_upstream_feature(feature_by_id[from_id].get());
                links_found++;
            }
        }

        if (to_attribute != nullptr) {
            std::string to_id = feature->get(*to_attribute).as_string();

            if (to_id != "null" && feature_by_id.find(to_id) != feature_by_id.end()) {
                feature->add_downstream_feature(feature_by_id[to_id].get());
                links_found++;
            }
        }
    }
    return links_found;
}

void FeatureCollection::add_feature(Feature feature, std::string *id) {
    features.push_back(feature);

    if (id != nullptr) {
        feature_by_id.emplace(*id, feature);
    }
    else if (feature->get_id() != "") {
        feature_by_id.emplace(feature->get_id(), feature);
    }
}

void FeatureCollection::update_ids() {
    feature_by_id.clear();
    for (auto feature : features) {
        if (feature->get_id() != "") {
            feature_by_id.emplace(feature->get_id(), feature);
        }
    }
}
