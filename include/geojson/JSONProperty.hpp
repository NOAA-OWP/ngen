#ifndef GEOJSON_JSONPROPERTY_H
#define GEOJSON_JSONPROPERTY_H

#include <string>
#include <map>
#include <vector>

#include <boost/property_tree/ptree.hpp>

namespace geojson {
    class JSONProperty;
    
    /**
     * Defines the different types of properties that are stored within a JSON property 
     */
    enum class PropertyType {
        Natural,    /*!< Represents natural numbers, such as shorts, ints, and longs; no floating points */
        Real,       /*!< Represents floating point numbers */
        String,     /*!< Represents text */
        Boolean,    /*!< Represents a true or false value */
        Object      /*!< Represents a nested map of properties */
    };

    /**
     * Shorthand for a mapping between strings and properties
     */
    typedef std::map<std::string, JSONProperty> PropertyMap;

    /** @TODO: Convert JSONProperty into a variant of the supported types  */
    /**
     * Object used to store basic geojson property data
     */
    class JSONProperty {
        public:
            
            JSONProperty(std::string value_key, const boost::property_tree::ptree& property_tree) {
                key = value_key;

                if (property_tree.empty() && !property_tree.data().empty()) {
                    // This is a terminal node and has a raw value
                        // TODO: Add handling for nested objects by determining if property.second is another ptree
                        std::string value = property_tree.data();

                        if (value == "true" || value == "false") {
                            type = PropertyType::Boolean;
                            boolean = value == "true";
                        }
                        else {
                            bool is_numeric = true;
                            bool is_real = true;
                            bool decimal_already_hit = false;

                            for(int character_index = 0; character_index < value.length(); character_index++) {
                                char character = value[character_index];

                                // If the first character is a '0' or isn't a digit, the whole value cannot be a number
                                if (character_index == 0 && character == '0') {
                                    is_numeric = false;
                                }
                                else if (character != '.' && !std::isdigit(character)) {
                                    // If this character isn't a decimal point and isn't a digit, the whole value cannot be a number
                                    is_numeric = false;
                                    is_real = false;
                                    break;
                                }
                                else if (character == '.' && decimal_already_hit) {
                                    // If this character is a decimal point, but we've already seen one, the whole value cannot be a number
                                    is_real = false;
                                    break;
                                }
                                else if (character == '.') {
                                    // If a decimal point is seen, the whole value cannot be an integer
                                    is_numeric = false;
                                    decimal_already_hit = true;
                                }
                            }

                            // If the value can be represented as a whole number, we want to go with that
                            if (is_numeric) {
                                type = PropertyType::Natural;
                                natural_number = std::stol(value);
                            }
                            else if (is_real) {
                                type = PropertyType::Real;
                                real_number = std::stod(value);
                            }
                            else {
                                // Otherwise we'll store everything as a raw string
                                type = PropertyType::String;
                                string = value;
                            }
                        }
                }
                else {
                    // This isn't a terminal node, therefore represents an object
                    type = PropertyType::Object;
                    for (auto &property : property_tree) {
                        values.emplace(property.first, JSONProperty(property.first, property.second));
                    }
                }
            }

            /**
             * Create a JSONProperty that stores a natural number
             * 
             * @param value_key: The name of the key that stores this value
             * @param value: The natural number that will be stored
             */
            JSONProperty(std::string value_key, short value)
                : type(PropertyType::Natural),
                    key(value_key),
                    natural_number(long(value))
            {}

            /**
             * Create a JSONProperty that stores a natural number
             * 
             * @param value_key: The name of the key that stores this value
             * @param value: The natural number that will be stored
             */
            JSONProperty(std::string value_key, int value)
                : type(PropertyType::Natural),
                    key(value_key),
                    natural_number(long(value))
            {}

            /**
             * Create a JSONProperty that stores a natural number
             * 
             * @param value_key: The name of the key that stores this value
             * @param value: The natural number that will be stored
             */
            JSONProperty(std::string value_key, long value)
                : type(PropertyType::Natural),
                    key(value_key),
                    natural_number(value)
            {}

            /**
             * Create a JSONProperty that stores a floating point number
             * 
             * @param value_key: The name of the key that stores this value
             * @param value: The floating point number that will be stored
             */
            JSONProperty(std::string value_key, float value)
                : type(PropertyType::Real),
                    key(value_key),
                    real_number(double(value))
            {}

            /**
             * Create a JSONProperty that stores a floating point number
             * 
             * @param value_key: The name of the key that stores this value
             * @param value: The floating point number that will be stored
             */
            JSONProperty(std::string value_key, double value)
                : type(PropertyType::Real),
                    key(value_key),
                    real_number(value)
            {}

            /**
             * Create a JSONProperty that stores text
             * 
             * @param value_key: The name of the key that stores this value
             * @param value: The text that will be stored
             */
            JSONProperty(std::string value_key, const char *value) {
                type = PropertyType::String;
                key = value_key;
                string = value;
            }

            /**
             * Create a JSONProperty that stores text
             * 
             * @param value_key: The name of the key that stores this value
             * @param value: The text that will be stored
             */
            JSONProperty(std::string value_key, std::string value) {
                type = PropertyType::String;
                key = value_key;
                string = value;
            }

            /**
             * A basic destructor
             */
            virtual ~JSONProperty(){};

            /**
             * Create a JSONProperty that stores a true or false value
             * 
             * @param value_key: The name of the key that stores this value
             * @param value: The true or false value that will be stored
             */
            JSONProperty(std::string value_key, bool value) {
                type = PropertyType::Boolean;
                key = value_key;
                boolean = value;
            }

            /**
             * Create a JSONProperty that stores a nested map of properties
             * 
             * @param value_key: The name of the key that stores this value
             * @param value: A map of nested properties that will be stored
             */
            JSONProperty(std::string value_key, PropertyMap &value)
                : type(PropertyType::Object),
                    key(value_key),
                    values(value)
            {}

            /**
             * Get the type of the property (Natural, Real, String, etc)
             * 
             * @return The type of the property
             */
            PropertyType get_type() const;

            /**
             * @brief Attempt to get the natural numeric value stored within the property
             * 
             * An exception will be thrown if this property doesn't store a natural number
             * 
             * @return The natural number that is stored within the property
             */
            long as_natural_number() const;

            double as_real_number() const;

            bool as_boolean() const;

            std::string as_string() const;

            JSONProperty at(std::string key) const;

            std::vector<std::string> keys() const;

            std::map<std::string, JSONProperty> get_values() const;

            std::string get_key() const;

            bool has_key(std::string key) const;

            bool inline operator==(const JSONProperty& other) {
                if (not (this->type == other.type)) {
                    return false;
                }

                if (this->type == PropertyType::Object) {
                    if (this->values.size() != other.values.size()) {
                        return false;
                    }
                    
                    for (std::string their_key : other.keys()) {
                        if (not this->has_key(their_key) or this->at(their_key) != other.at(their_key)) {
                            return false;
                        }
                    }

                }

                return this->natural_number == other.natural_number 
                    and this->real_number == other.real_number 
                    and this->string == other.string 
                    and this->boolean == other.boolean;
            }

            bool inline operator!=(const JSONProperty& other) {
                return not this->operator==(other);
            }
        private:
            PropertyType type;
            std::string key;
            long natural_number;
            double real_number;
            std::string string;
            bool boolean;
            PropertyMap values;
    };
}
#endif // GEOJSON_JSONPROPERTY_H