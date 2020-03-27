#include "JSONProperty.hpp"

geojson::PropertyType geojson::JSONProperty::get_type() const {
    return type;
}

long geojson::JSONProperty::as_natural_number() const {
    // Return a natural number if this IS a natural number
    if (type == PropertyType::Natural) {
        return natural_number;
    }

    // Throw an exception since this can't be considered a natural number
    std::string message = key + " cannot be converted into a natural number.";
    throw std::runtime_error(message);
}

double geojson::JSONProperty::as_real_number() const {
    if (type == PropertyType::Real) {
        return real_number;
    }
    else if (type == PropertyType::Natural) {
        return double(natural_number);
    }

    std::string message = key + " cannot be converted into a real number.";
    throw std::runtime_error(message);
}

bool geojson::JSONProperty::as_boolean() const {
    if (type == PropertyType::Boolean) {
        return boolean;
    }

    std::string message = key + " cannot be converted into a boolean.";
    throw std::runtime_error(message);
}

std::string geojson::JSONProperty::as_string() const {
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
}

geojson::JSONProperty geojson::JSONProperty::at(std::string key) const {
    if (type == PropertyType::Object) {
        return values.at(key);
    }

    std::string message = key + " is not an object and cannot be referenced as one.";
    throw std::runtime_error(message);
}

std::map<std::string, geojson::JSONProperty> geojson::JSONProperty::get_values() const {
    if (type == PropertyType::Object) {
        return values;
    }

    std::string message = key + " is not an object.";
    throw std::runtime_error(message);
}

std::string geojson::JSONProperty::get_key() const {
    return key;
}
