#ifndef GEOJSON_JSONPROPERTY_H
#define GEOJSON_JSONPROPERTY_H

#include <string>
#include <map>

namespace geojson {
    enum PropertyType {
        Natural,
        Real,
        String,
        Boolean,
        Object
    };

    class JSONProperty {
        public:
        JSONProperty(std::string value_key, short value)
            : type(PropertyType::Natural),
                key(value_key),
                natural_number(long(value))
        {}
        JSONProperty(std::string value_key, int value)
            : type(PropertyType::Natural),
                key(value_key),
                natural_number(long(value))
        {}
        JSONProperty(std::string value_key, long value)
            : type(PropertyType::Natural),
                key(value_key),
                natural_number(value)
        {}
        JSONProperty(std::string value_key, float value)
            : type(PropertyType::Real),
                key(value_key),
                real_number(double(value))
        {}
        JSONProperty(std::string value_key, double value)
            : type(PropertyType::Real),
                key(value_key),
                real_number(value)
        {}
        JSONProperty(std::string value_key, std::string value)
            : type(PropertyType::String),
                key(value_key),
                string(value)
        {}
        JSONProperty(std::string value_key, bool value)
            : type(PropertyType::Boolean),
                key(value_key),
                boolean(value)
        {}
        JSONProperty(std::string value_key, std::map<std::string, JSONProperty> &value)
            : type(PropertyType::Object),
                key(value_key),
                values(value)
        {}
        JSONProperty(const JSONProperty &json_property) {
            switch(json_property.get_type()) {
                case PropertyType::Natural:
                    natural_number = json_property.as_natural_number();
                    break;
                case PropertyType::Real:
                    real_number = json_property.as_real_number();
                    break;
                case PropertyType::String:
                    string = json_property.as_string();
                    break;
                case PropertyType::Boolean:
                    boolean = json_property.as_boolean();
                    break;
                case PropertyType::Object:
                    values = json_property.get_values();
                    break;
            }

            type = json_property.type;
            key = json_property.key;            
        }
        virtual ~JSONProperty(){};

        PropertyType get_type() const {
            return type;
        };

    long as_natural_number() const {
        if (type == PropertyType::Natural) {
            return natural_number;
        }
        std::string message = key + " cannot be converted into a natural number.";
        throw std::runtime_error(message);
    }

    double as_real_number() const {
        if (type == PropertyType::Real) {
            return real_number;
        }
        else if (type == PropertyType::Natural) {
            return double(natural_number);
        }

        std::string message = key + " cannot be converted into a real number.";
        throw std::runtime_error(message);
    }

    bool as_boolean() const {
        if (type == PropertyType::Boolean) {
            return boolean;
        }

        std::string message = key + " cannot be converted into a boolean.";
        throw std::runtime_error(message);
    }

    std::string as_string() const {
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

    JSONProperty from_values(std::string key) const {
        if (type == PropertyType::Object) {
            return values.at(key);
        }

        std::string message = key + " is not an object and cannot be referenced as one.";
        throw std::runtime_error(message);
    }

    std::map<std::string, JSONProperty> get_values() const {
        if (type == PropertyType::Object) {
            return values;
        }

        std::string message = key + " is not an object.";
        throw std::runtime_error(message);
    }

    std::string get_key() const {
        return key;
    }
        private:
            PropertyType type;
            std::string key;
            long natural_number;
            double real_number;
            std::string string;
            bool boolean;
            std::map<std::string, JSONProperty> values;
    };
}
#endif // GEOJSON_JSONPROPERTY_H