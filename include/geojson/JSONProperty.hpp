#ifndef GEOJSON_JSONPROPERTY_H
#define GEOJSON_JSONPROPERTY_H

#include <string>
#include <map>

namespace geojson {
    /**
     * Defines the different types of properties that are stored within a JSON property 
     */
    enum PropertyType {
        Natural,    /** Represents natural numbers, such as shorts, ints, and longs; no floating points */
        Real,       /** Represents floating point numbers */
        String,     /** Represents text */
        Boolean,    /** Represents a true or false value */
        Object      /** Represents a nested map of properties */
    };

    /**
     * Object used to store basic geojson property data
     */
    class JSONProperty {
        public:
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
            JSONProperty(std::string value_key, std::map<std::string, JSONProperty> &value)
                : type(PropertyType::Object),
                    key(value_key),
                    values(value)
            {}

            /**
             * Copy constructor for a property
             * 
             * @param json_property: The property to copy
             */
            JSONProperty(const JSONProperty &json_property) {

                // Store the value that matches the proper type
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

            /**
             * A basic destructor
             */
            virtual ~JSONProperty(){};

            /**
             * Get the type of the property (Natural, Real, String, etc)
             * 
             * @return The type of the property
             */
            PropertyType get_type() const;;

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

            std::map<std::string, JSONProperty> get_values() const;

            std::string get_key() const;

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