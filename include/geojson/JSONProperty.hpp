#ifndef GEOJSON_JSONPROPERTY_H
#define GEOJSON_JSONPROPERTY_H

#include <string>
#include <map>
#include <vector>
#include <iostream>

#include <boost/property_tree/ptree.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/variant.hpp>

namespace geojson {
    class JSONProperty;
    //Forward declare variant types
    struct List;
    struct Object;
    using PropertyVariant = boost::variant<boost::blank, 
                                           long, 
                                           double, 
                                           bool, 
                                           std::string, 
                                           boost::recursive_wrapper<List>, 
                                           boost::recursive_wrapper<Object>
                                          >;
    
    /**
     * @brief Struct wrapping a vector of \ref PropertyVariant representing a JSON list.
     * 
     */
    struct List{
        std::vector<PropertyVariant> values;
        friend std::ostream& operator<<(std::ostream& os, const List& obj){
            os<<"LIST";
            return os;
        }
    };

    /**
     * @brief Struct wrapping a map of nested \ref PropertyVariant representing a JSON object.
     * 
     */
    struct Object{
        std::map<std::string, PropertyVariant> values;
        friend std::ostream& operator<<(std::ostream& os, const Object& obj){
            os<<"OBJECT";
            return os;
        }
    };
    
    /**
     * Defines the different types of properties that are stored within a JSON property 
     */
    enum class PropertyType {
        Natural,    /*!< Represents natural numbers, such as shorts, ints, and longs; no floating points */
        Real,       /*!< Represents floating point numbers */
        String,     /*!< Represents text */
        Boolean,    /*!< Represents a true or false value */
        List,       /*!< Represents a list of values */
        Object      /*!< Represents a nested map of properties */
    };

    static inline std::string get_propertytype_name(PropertyType property_type) {
        switch(property_type) {
            case PropertyType::Natural:
                return "Natural";
            case PropertyType::Real:
                return "Real";
            case PropertyType::String:
                return "String";
            case PropertyType::Boolean:
                return "Boolean";
            case PropertyType::List:
                return "List";
            default:
                return "Object";
        }
    }

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
                          //Try natural number first since double cant cast to int/long
                          try{
                            natural_number = boost::lexical_cast<long>(value);
                            type = PropertyType::Natural;
                          }
                          catch (boost::bad_lexical_cast &e)
                          {
                            try {
                              //Try to cast to double/real next
                              real_number = boost::lexical_cast<double>(value);
                              type = PropertyType::Real;
                            }
                            catch (boost::bad_lexical_cast & e)
                            {
                              //At this point, we are left with string option
                              string = value;
                              type = PropertyType::String;
                            }
                          }
                        }
                }
                else {
                    // This isn't a terminal node, therefore represents an object or array
                    for (auto &property : property_tree) {
                        if (property.first.empty()) {
                            type = PropertyType::List;
                            value_list.push_back(JSONProperty(value_key, property.second));
                        }
                        else {
                            type = PropertyType::Object;
                            values.emplace(property.first, JSONProperty(property.first, property.second));
                        }
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
                key = value_key;
                string = value;

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

            JSONProperty(std::string value_key, std::vector<JSONProperty> properties) : type(PropertyType::List), key(value_key), value_list(properties) {}

            JSONProperty(const JSONProperty &original) {
                type = original.type;
                key = original.key;

                switch (type) {
                    case PropertyType::Boolean:
                        boolean = original.boolean;
                        break;
                    case PropertyType::Natural:
                        natural_number = original.natural_number;
                        break;
                    case PropertyType::Real:
                        real_number = original.real_number;
                        break;
                    case PropertyType::String:
                        string = original.string;
                        break;
                    case PropertyType::List:
                        for (JSONProperty property : original.value_list) {
                            value_list.push_back(JSONProperty(property));
                        }
                        break;
                    default:
                        for (std::pair<std::string, JSONProperty> pair : original.values) {
                            values.emplace(pair.first, JSONProperty(pair.second));
                        }
                }
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

            std::vector<JSONProperty> as_list() const;

            std::vector<long> as_natural_vector() const;

            std::vector<double> as_real_vector() const;

            std::vector<std::string> as_string_vector() const;

            std::vector<bool> as_boolean_vector() const;

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
            std::vector<JSONProperty> value_list;
            //boost::variant to hold the parsed data
            //can be one of boost::blank, long, double, bool, string, List, Object
            //Defaults to boost::blank
            //TODO port this entire class to use this variant
            PropertyVariant data;
    };
}
#endif // GEOJSON_JSONPROPERTY_H
