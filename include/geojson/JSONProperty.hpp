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
    /**
     * Shorthand for a mapping between strings and properties
     */
    typedef std::map<std::string, JSONProperty> PropertyMap;

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

    static inline std::string get_propertytype_name(PropertyType&& property_type) {
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

    /** @TODO: Convert JSONProperty into a variant of the supported types  */
    /**
     * Object used to store basic geojson property data
     */
    class JSONProperty {
        public:
            
            JSONProperty(std::string value_key, const boost::property_tree::ptree& property_tree):
            key(std::move(value_key)) {

                if (property_tree.empty() && !property_tree.data().empty()) {
                    // This is a terminal node and has a raw value

                        std::string value = property_tree.data();

                        if (value == "true" || value == "false") {
                            type = PropertyType::Boolean;
                            boolean = value == "true";
                            data = boolean;
                        }
                        else {
                          //Try natural number first since double cant cast to int/long
                          try{
                            natural_number = boost::lexical_cast<long>(value);
                            type = PropertyType::Natural;
                            data = natural_number;
                          }
                          catch (boost::bad_lexical_cast &e)
                          {
                            try {
                              //Try to cast to double/real next
                              real_number = boost::lexical_cast<double>(value);
                              type = PropertyType::Real;
                              data = real_number;
                            }
                            catch (boost::bad_lexical_cast & e)
                            {
                              //At this point, we are left with string option
                              string = value;
                              type = PropertyType::String;
                              data = string;
                            }
                          }
                        }
                }
                else {
                    // This isn't a terminal node, therefore represents an object or array
                    List tmp_list;
                    Object tmp_obj;
                    //TODO unit test these construction paths...
                    for (auto &property : property_tree) {
                        if (property.first.empty()) {
                            type = PropertyType::List;
                            JSONProperty tmp = JSONProperty(value_key, property.second);
                            value_list.push_back(tmp);
                            tmp_list.values.push_back(tmp.data);
                            data = tmp_list;
                        }
                        else {
                            type = PropertyType::Object;
                            JSONProperty tmp = JSONProperty(property.first, property.second);
                            values.emplace(property.first, tmp);
                            tmp_obj.values.emplace(property.first, tmp.data);
                            data = tmp_obj;
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
                    key(std::move(value_key)),
                    natural_number(long(value)),
                    data(long(value))
            {}

            /**
             * Create a JSONProperty that stores a natural number
             * 
             * @param value_key: The name of the key that stores this value
             * @param value: The natural number that will be stored
             */
            JSONProperty(std::string value_key, int value)
                : type(PropertyType::Natural),
                    key(std::move(value_key)),
                    natural_number(long(value)),
                    data(long(value))
            {}

            /**
             * Create a JSONProperty that stores a natural number
             * 
             * @param value_key: The name of the key that stores this value
             * @param value: The natural number that will be stored
             */
            JSONProperty(std::string value_key, long value)
                : type(PropertyType::Natural),
                    key(std::move(value_key)),
                    natural_number(value),
                    data(value)
            {}

            /**
             * Create a JSONProperty that stores a floating point number
             * 
             * @param value_key: The name of the key that stores this value
             * @param value: The floating point number that will be stored
             */
            JSONProperty(std::string value_key, float value)
                : type(PropertyType::Real),
                    key(std::move(value_key)),
                    real_number(double(value)),
                    data(double(value))
            {}

            /**
             * Create a JSONProperty that stores a floating point number
             * 
             * @param value_key: The name of the key that stores this value
             * @param value: The floating point number that will be stored
             */
            JSONProperty(std::string value_key, double value)
                : type(PropertyType::Real),
                    key(std::move(value_key)),
                    real_number(value),
                    data(value)
            {}

            /**
             * Create a JSONProperty that stores text
             * 
             * @param value_key: The name of the key that stores this value
             * @param value: The text that will be stored
             */
            JSONProperty(std::string value_key, const char *value):
                type(PropertyType::String),
                key(std::move(value_key)),
                string(value),
                data(std::string(value))
            {}

            /**
             * Create a JSONProperty that stores text
             * 
             * @param value_key: The name of the key that stores this value
             * @param value: The text that will be stored
             */
            JSONProperty(std::string value_key, std::string value):key(std::move(value_key)) {
                string = value;

                if (value == "true" || value == "false") {
                    type = PropertyType::Boolean;
                    boolean = value == "true";
                    data = boolean;
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
                        data = natural_number;
                    }
                    else if (is_real) {
                        type = PropertyType::Real;
                        real_number = std::stod(value);
                        data = real_number;
                    }
                    else {
                        // Otherwise we'll store everything as a raw string
                        type = PropertyType::String;
                        string = value;
                        data = string;
                    }
                }
            }

            JSONProperty(std::string value_key, std::vector<JSONProperty> properties) : type(PropertyType::List), key(std::move(value_key)), value_list(std::move(properties))  {
                List tmp;
                for(const auto& p : properties){
                    tmp.values.push_back(p.data);
                }
                data = tmp;
            }

            JSONProperty(const JSONProperty &original) {
                type = original.type;
                key = original.key;
                data = original.data;
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
            JSONProperty(std::string value_key, bool value):
                type(PropertyType::Boolean),
                key(std::move(value_key)),
                boolean(value),
                data(value)
            {}

            /**
             * Create a JSONProperty that stores a nested map of properties
             * 
             * @param value_key: The name of the key that stores this value
             * @param value: A map of nested properties that will be stored
             */
            JSONProperty(std::string value_key, PropertyMap &value)
                : type(PropertyType::Object),
                    key(std::move(value_key)),
                    values(value)
            {   
                Object tmp;
                for(auto const& property : value){
                    tmp.values.emplace(property.first, property.second.data);
                }
                data = tmp;
            }

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

            /**
             * @brief Populates a std::vector<T> with PropertyVariant values.
             * 
             * Scalar properties will yeild a vector of size 1.
             * 
             * List properties will yield a vector of compatible types.
             * E.g. a List with [1, 2.1, 3] can upcast the Natural number 1 and 3 iff 
             * a container is provided with sufficient datatype (double)
             * 
             * std::vector<double> double_vec;
             * as_vector(double_vec);
             * 
             * Will give a vector of doubles = {1.0, 2.1, 3.0}.
             * 
             * However, if a vector of long is used, only the Natural numbers will be extracted
             * 
             * std::vector<long> long_vec;
             * as_vector(long_vec); 
             * 
             * Will give a vector of longs = {1, 3}.
             * 
             * Other than this caveat, as_vector effetively filters the property list for types
             * representable by T.
             * 
             * @tparam T 
             * @param vector 
             */
            template <typename T>
            void as_vector(std::vector<T>& vector) const{
                PropertyVisitor<T> visitor(vector);
                boost::apply_visitor(visitor, data);
            }

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
            //TODO make sure all construction paths for `data` are unit tested
            PropertyVariant data;
        
            /**
             * @brief Visitor to filter boost variants by type T into a vector of type T
             * 
             * A specialization is defined in JSONProperty.cpp which allows
             * a vistitor to coerce long type variants into double types.
             * See \ref as_vector for some explanation.
             * 
             * TODO rename this visitor to be a litte more clear on its semantics.
             * @tparam T 
             */
            template<typename T>
            struct PropertyVisitor : public boost::static_visitor<>
            {
                //PropertyVisitor takes a templated vector and stores the vector
                //reference in the struct
                PropertyVisitor(std::vector<T>& v) : vec(v) {}
                //Vistor operators, first is generic template operator
                template<typename Variant>
                void operator () (const Variant& value) {
                    //This is a no-op for all types that are not T
                }
     
                //Visitor operator for type T, adds T types to vector
                void operator () (const T& value)
                {
                    vec.push_back(value);
                }

                /* Can overload these with custom implementation and/or errors
                void operator () (const boost::blank&){}
                void operator () (const Object&) {}
                */
                void operator () (const List& values) {
                    //Recurse through the list to filter for types T
                    std::for_each(values.values.begin(), values.values.end(), boost::apply_visitor(*this));
                }

                private:
                //Storage for filtered items
                std::vector<T>& vec;
            };
    };
}
#endif // GEOJSON_JSONPROPERTY_H
