#ifndef GEOJSON_JSONPROPERTY_H
#define GEOJSON_JSONPROPERTY_H

#include <string>
#include <map>

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
             * A basic destructor
             */
            virtual ~JSONProperty(){};

            /**
             * Get the type of the property (Natural, Real, String, etc)
             * 
             * @return The type of the property
             */
            PropertyType get_type() const {
                return type;
            };

            /**
             * @brief Attempt to get the natural numeric value stored within the property
             * 
             * An exception will be thrown if this property doesn't store a natural number
             * 
             * @return The natural number that is stored within the property
             */
            long as_natural_number() const {
                // Return a natural number if this IS a natural number
                if (type == PropertyType::Natural) {
                    return natural_number;
                }

                // Throw an exception since this can't be considered a natural number
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
             * A basic destructor
             */
            virtual ~JSONProperty(){};

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