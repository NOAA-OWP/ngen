#include "FeatureCollection.hpp"
#include "features/Features.hpp"

using namespace geojson;

Feature FeatureCollection::get_feature(int index) const {
    return features[index];
}

int FeatureCollection::find(Feature feature) {
    for(int feature_index = 0; feature_index < this->get_size(); feature_index++) {
        if (*this->features[feature_index] == *feature) {
            return feature_index;
        }
    }

    return -1;
}

int FeatureCollection::find(std::string ID) {
    for (int feature_index = 0; feature_index < this->get_size(); feature_index++) {
        if (this->features[feature_index]->get_id() == ID) {
            return feature_index;
        }
    }

    return -1;
}

Feature FeatureCollection::remove_feature(int feature_index) {
    Feature popped_feature = this->get_feature(feature_index);

    if (this->find(popped_feature->get_id()) >= 0) {
        this->feature_by_id.erase(popped_feature->get_id());
    }

    this->features.erase(this->features.begin() + feature_index);

    return popped_feature;
}

Feature FeatureCollection::remove_feature_by_id(std::string ID) {
    Feature popped_feature = this->get_feature(ID);

    if (popped_feature) {
        this->feature_by_id.erase(ID);

        int feature_index = 0;

        for (; feature_index < this->get_size(); feature_index++) {
            if (this->features[feature_index]->get_id() == popped_feature->get_id()) {
                break;
            }
        }

        this->features.erase(this->features.begin() + feature_index);
    }
    
    return popped_feature;
}

int FeatureCollection::get_size() {
    return features.size();
};

bool FeatureCollection::is_empty() {
    return features.size() == 0;
}

std::vector<double> FeatureCollection::get_bounding_box() const {
    return bounding_box;
}

Feature FeatureCollection::get_feature(std::string id) const {
    if (feature_by_id.find(id) == feature_by_id.end()) {
        return Feature();
    }

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
        if (from_property != nullptr and feature->has_property(*from_property)) {
            std::string from_id = feature->get_property(*from_property).as_string();
            
            if (feature_by_id.find(from_id) != feature_by_id.end()) {
                feature->add_origination_feature(feature_by_id[from_id].get());
                links_found++;
            }
        }

        if (to_property != nullptr and feature->has_property(*to_property)) {
            std::string to_id = feature->get_property(*to_property).as_string();

            if (feature_by_id.find(to_id) != feature_by_id.end()) {
                feature->add_destination_feature(feature_by_id[to_id].get());
                links_found++;
            }
        }
    }

    return links_found;
}

int FeatureCollection::link_features_from_attribute(std::string* from_attribute, std::string* to_attribute) {
    int links_found = 0;

    for (Feature feature : features) {
        if (from_attribute != nullptr and feature->has_key(*from_attribute)) {
            std::string from_id = feature->get(*from_attribute).as_string();

            bool has_value = not (
                from_id == ""
                    or from_id == "null"
                    or from_id == "Null"
                    or from_id == "NULL"
                    or from_id == "None"
                    or from_id == "NONE"
            );
            
            if (has_value && feature_by_id.find(from_id) != feature_by_id.end()) {
                feature->add_origination_feature(feature_by_id[from_id].get());
                links_found++;
            }
        }

        if (to_attribute != nullptr and feature->has_key(*to_attribute)) {
            std::string to_id = feature->get(*to_attribute).as_string();

            bool has_value = not (
                to_id == ""
                    or to_id == "null"
                    or to_id == "Null"
                    or to_id == "NULL"
                    or to_id == "None"
                    or to_id == "NONE"
            );

            if (has_value && feature_by_id.find(to_id) != feature_by_id.end()) {
                feature->add_destination_feature(feature_by_id[to_id].get());
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
    else if (not feature->get_id().empty()) {
        feature_by_id.emplace(feature->get_id(), feature);
    }
}

void FeatureCollection::set_ids_from_member(std::string member_name) {
    for (auto feature : this->features) {
        if(contains(feature->keys(), member_name)) {
            feature->set_id(feature->get(member_name).as_string());
        }
    }

    this->update_ids();
}

void FeatureCollection::set_ids_from_property(std::string property_name) {
    for (auto feature : this->features) {
        if (contains(feature->property_keys(), property_name)) {
            feature->set_id(feature->get_property(property_name).as_string());
        }
    }

    this->update_ids();
}

void FeatureCollection::set_ids(std::string id_field_name) {
    for (auto feature : this->features) {
        if(contains(feature->property_keys(), id_field_name)) {
            feature->set_id(feature->get_property(id_field_name).as_string());
        }
        else if (contains(feature->keys(), id_field_name)) {
            feature->set_id(feature->get(id_field_name).as_string());
        }
    }

    this->update_ids();
}

void FeatureCollection::update_ids() {
    feature_by_id.clear();
    for (auto feature : features) {
        if (feature->get_id() != "") {
            feature_by_id.emplace(feature->get_id(), feature);
        }
    }
}

void FeatureCollection::add_feature_id(std::string id, Feature feature) {
    if (id != "") {
        feature_by_id.emplace(id, feature);
    }
    else {
        throw std::invalid_argument("id");
    }
}