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
    std::string message = key + " cannot be converted into a natural number.";
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

    std::string message = key + " cannot be converted into a real number.";
    throw std::runtime_error(message);
};

bool JSONProperty::as_boolean() const {
    if (type == PropertyType::Boolean) {
        return boolean;
    }

    std::string message = key + " cannot be converted into a boolean.";
    throw std::runtime_error(message);
};

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

    std::string message = key + " cannot be converted into a string.";
    throw std::runtime_error(message);
};

JSONProperty JSONProperty::at(std::string key) const {
    if (type == PropertyType::Object) {
        return values.at(key);
    }

    std::string message = key + " is not an object and cannot be referenced as one.";
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

    std::string message = key + " is not an object and cannot be referenced as one.";
    throw std::runtime_error(message);
}

PropertyMap JSONProperty::get_values() const {
    if (type == PropertyType::Object) {
        return values;
    }

    std::string message = key + " is not an object.";
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
