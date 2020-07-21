#include "JSONProperty.hpp"

using namespace geojson;

/**
 * @brief Attempt to get the natural numeric value stored within the property
 * 
 * An exception will be thrown if this property doesn't store a natural number
 * 
 * @return The natural number that is stored within the property
 */
long JSONProperty::as_natural_number() const {
    // Return a natural number if this IS a natural number
    if (type == PropertyType::Natural) {
        return natural_number;
    }

    // Throw an exception since this can't be considered a natural number
    std::string message = key + " is a " + get_propertytype_name(get_type()) + " and cannot be converted into a natural number.";
    throw std::runtime_error(message);
};

/**
 * Get the type of the property (Natural, Real, String, etc)
 * 
 * @return The type of the property
 */
PropertyType JSONProperty::get_type() const {
    return type;
};


double JSONProperty::as_real_number() const {
    if (type == PropertyType::Real) {
        return real_number;
    }
    else if (type == PropertyType::Natural) {
        return double(natural_number);
    }

    std::string message = key + " is a " + get_propertytype_name(get_type()) + " and cannot be converted into a real number.";
    throw std::runtime_error(message);
};

bool JSONProperty::as_boolean() const {
    if (type == PropertyType::Boolean) {
        return boolean;
    }

    std::string message = key + " is a " + get_propertytype_name(get_type()) + " and cannot be converted into a boolean.";
    throw std::runtime_error(message);
};

std::vector<JSONProperty> JSONProperty::as_list() const {
        std::vector<JSONProperty> copy;

    if (type == PropertyType::List) {
        for (JSONProperty value : value_list) {
            copy.push_back(JSONProperty(value));
        }
        return copy;
    }
    else if (type != PropertyType::Object) {
        copy.push_back(JSONProperty(*this));
        return copy;
    }

    std::string message = key + " is a " + get_propertytype_name(get_type()) + " and cannot be converted into a list.";
    throw std::runtime_error(message);
}

std::vector<long> JSONProperty::as_natural_vector() const {
    std::vector<long> values;

    for (JSONProperty value : this->as_list()) {
        values.push_back(value.as_natural_number());
    }

    return values;
}

std::vector<double> JSONProperty::as_real_vector() const {
    std::vector<double> values;

    for (JSONProperty value : this->as_list()) {
        values.push_back(value.as_real_number());
    }

    return values;
}

std::vector<std::string> JSONProperty::as_string_vector() const {
    std::vector<std::string> values;

    for (JSONProperty value : this->as_list()) {
        values.push_back(value.as_string());
    }

    return values;
}

std::vector<bool> JSONProperty::as_boolean_vector() const {
    std::vector<bool> values;

    for (JSONProperty value : this->as_list()) {
        values.push_back(value.as_boolean());
    }

    return values;
}

std::string JSONProperty::as_string() const {
    if (type == PropertyType::String) {
        return string;
    }
    else if (type == PropertyType::Boolean) {
        if (boolean) {
            return "true";
        }
        else {
            return "false";
        }
    }
    else if (type == PropertyType::Natural) {
        return std::to_string(natural_number);
    }
    else if(type == PropertyType::Real) {
        return std::to_string(real_number);
    }
    else if (type == PropertyType::List) {
        std::string list_description = "[";

        for (int list_index = 0; list_index < this->value_list.size(); list_index++) {
            list_description += this->value_list[list_index].as_string();

            if (list_index < this->value_list.size() - 1) {
                list_description += ",";
            }
        }

        list_description += "]";
        return list_description;
    }

    std::string message = key + " is a " + get_propertytype_name(get_type()) + " and cannot be converted into a string.";
    throw std::runtime_error(message);
};

JSONProperty JSONProperty::at(std::string key) const {
    if (type == PropertyType::Object) {
        return values.at(key);
    }

    std::string message = key + " is a " + get_propertytype_name(get_type()) + ", not an object and cannot be referenced as one.";
    throw std::runtime_error(message);
}

std::vector<std::string> JSONProperty::keys() const {
    if (type == PropertyType::Object) {
        std::vector<std::string> key_names;

        for (auto &pair : values) {
            key_names.push_back(pair.first);
        }

        return key_names;
    }

    std::string message = key + " is a " + get_propertytype_name(get_type()) + ", not an object and cannot be referenced as one.";
    throw std::runtime_error(message);
}

PropertyMap JSONProperty::get_values() const {
    if (type == PropertyType::Object) {
        return values;
    }

    std::string message = key + " is a " + get_propertytype_name(get_type()) + ", not an object and cannot be referenced as one.";
    throw std::runtime_error(message);
}

bool JSONProperty::has_key(std::string key) const {
    for (std::string property_key : this->keys()) {
        if (property_key == key) {
            return true;
        }
    }

    return false;
}

std::string JSONProperty::get_key() const {
    return key;
}
