#ifndef GEOJSON_JSONPROPERTY_H
#define GEOJSON_JSONPROPERTY_H

#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <iomanip>
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
     * @brief Struct wrapping a vector of \ref JSONProperty representing a JSON list.
     * 
     * Note: that due to the forward declaration of JSONProperty and the recursive nature of the property and the variant types
     * a pointer is required to allow this incomplete type to encapsulate the vectory of JSONProperties.
     */
    struct List{
        private:
        std::vector<JSONProperty>* values;

        public:
        /**
         * @brief Construct a new List and set its values pointer to values
         * 
         * @param values 
         */
        List (std::vector<JSONProperty>* values): values(values){}

        /**
         * @brief Get the pointer to the values vector
         * 
         * @return const std::vector<JSONProperty>* 
         */
        std::vector<JSONProperty>* get_values() const {
            return values;
        }

        /**
         * @brief Add a JSONProperty to List backing storage via the values pointer
         * 
         * @param property JSONProperty to add to the back of the List
         */
        void push_back(const JSONProperty& property){
            values->push_back(property);
        }

        /**
         * @brief A stream overload to represent this type as a LIST
         * 
         * @param os 
         * @param obj List
         * @return std::ostream& 
         */
        friend std::ostream& operator<<(std::ostream& os, const List& obj){
            os<<"LIST";
            return os;
        }

        /**
         * @brief Equality operator
         * 
         * @param other List to check equality against
         * @return true If the JSONProperty vector \ref values is equal to \ref other vector of JSONProperty.
         * @return false If the JSONProperty vectors are not equivalent.
         */
        bool inline operator==(const List& other) const{
            return *values == *(other.values);
        }
    };

    /**
     * @brief Struct wrapping a nested \ref PropertyMap representing a JSON object.
     * 
     */
    struct Object{
        private:
        PropertyMap* values;

        public:
        /**
         * @brief Construct a new Object and set its values pointer to values
         * 
         * @param values 
         */
        Object (PropertyMap* values): values(values){}

        /**
         * @brief A stream overload to represent this type as an Object
         * 
         * @param os 
         * @param obj 
         * @return std::ostream& 
         */
        friend std::ostream& operator<<(std::ostream& os, const Object& obj){
            os<<"OBJECT";
            return os;
        }

        /**
         * @brief Equality operator
         * 
         * @param other Object to check equality against
         * @return true If the backing storage pointers are the same.
         * @return false If the \ref other JSONProperty isn't pointing the same data as this JSONProperty
         */
        bool inline operator==(const Object& other) const{
            return values == other.values; //FIXME right now, Objects must be pointing to the SAME data to be considered equal...
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

                if(property_tree.empty() && property_tree.data().empty()) {
                    //Since property trees don't represent empty strings, list, or objects, they are all just `empty`
                    //so we can trap that case with `property_tree.data().empty()`, and the best we can do is make it an
                    //empty string
                    type = PropertyType::String;
                    //Also note that a boost::variant assigned to the static empty string "" like this
                    //`data = "";`
                    //will cause `data` to actually become a bool type, not the std::string. 
                    //So use a default string to intialize the empty data
                    data = std::string();
                }
                else if (property_tree.empty() && !property_tree.data().empty()) {
                    // This is a terminal node and has a raw value

                    const std::string& value = property_tree.data();

                    if (value == "true" || value == "false") {
                        type = PropertyType::Boolean;
                        data = value == "true";
                    }
                    else {
                        //Try natural number first since double cant cast to int/long
                        long casted_long_data;
                        double casted_double_data;
                        if( boost::conversion::try_lexical_convert<long>(value, casted_long_data) ){
                            type = PropertyType::Natural;
                            data = casted_long_data;
                        }
                        else if( boost::conversion::try_lexical_convert<double>(value, casted_double_data) ){
                            //Try to cast to double/real next
                            type = PropertyType::Real;
                            data = casted_double_data;
                        }
                        else{
                            //At this point, we are left with string option
                            //string = value;
                            type = PropertyType::String;
                            data = std::move(value);
                        }
                    }
                }
                else {
                    // This isn't a terminal node, therefore represents an object or array
                    //TODO unit test these construction paths...
                    for (auto &property : property_tree) {
                        if (property.first.empty()) {
                            type = PropertyType::List;
                            value_list.push_back(JSONProperty(value_key, property.second));
                            data = List(& value_list );
                        }
                        else {
                            type = PropertyType::Object;
                            values.emplace(property.first, JSONProperty(property.first, property.second));
                            data = Object( & values );
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
                data(std::string(value))
            {}

            /**
             * Create a JSONProperty that stores text
             * 
             * @param value_key: The name of the key that stores this value
             * @param value: The text that will be stored
             */
            JSONProperty(std::string value_key, std::string value):key(std::move(value_key)) {
                if (value == "true" || value == "false") {
                    type = PropertyType::Boolean;
                    //boolean = value == "true";
                    data = value == "true";
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
                        //natural_number = std::stol(value);
                        data = std::stol(value);
                    }
                    else if (is_real) {
                        type = PropertyType::Real;
                        //real_number = std::stod(value);
                        data = std::stod(value);
                    }
                    else {
                        // Otherwise we'll store everything as a raw string
                        type = PropertyType::String;
                        //string = value;
                        data = value;
                    }
                }
            }

            JSONProperty(std::string value_key, std::vector<JSONProperty> properties) : type(PropertyType::List), key(std::move(value_key)), value_list(std::move(properties))  {
                data = List( &value_list );
            }

            JSONProperty(const JSONProperty &original) {
                if( this != &original){
                    type = original.type;
                    key = original.key;
                    switch (type) {
                        case PropertyType::List:
                            for (const JSONProperty& property : original.value_list) {
                                value_list.push_back(std::move(property));
                            }
                            data = List( &value_list );
                            break;
                        case PropertyType::Object:
                            for (std::pair<std::string, const JSONProperty& > pair : original.values) {
                                values.emplace(pair.first, std::move(pair.second));
                            }
                            data = Object( &values );
                            break;
                        default:
                            data = original.data;
                    }
                }
            }

            /**
             * @brief Copy construct a JSONProperty, but use a new key value for the property
             * 
             * @param value_key 
             * @param original 
             */
            JSONProperty(const std::string& value_key, const JSONProperty&original):JSONProperty(original){
                key = value_key;
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
                data = Object( &values );
            }

            /**
             * @brief Pretty print the property to standard out stream.
             * 
             * Recurses through the property to tab/print nested objects/lists.
             * 
             * @param p Property to print
             * @param tab (optional) Additional starting tab to indent (default 0)
             * @param newline (optional) Add a new line to the end of the print (default true)
             */
            static void print_property(const geojson::JSONProperty& p, int tab=0, bool newline = true){
                char end = '\0';
                if(newline)  end = '\n';
                std::cout<<std::setw(tab);
                switch( p.get_type() ){
                    case geojson::PropertyType::String:
                        std::cout<<p.as_string()<<end;
                        break;
                    case geojson::PropertyType::Real:
                        std::cout<<p.as_real_number()<<end;
                        break;
                    case geojson::PropertyType::Natural:
                        std::cout<<p.as_natural_number()<<end;
                        break;
                    case geojson::PropertyType::Boolean:
                        if(p.as_boolean())
                            std::cout<<"true"<<end;
                        else
                            std::cout<<"false"<<end;
                        break;   
                    case geojson::PropertyType::List:
                        std::cout<<std::setw(tab)<<"[";
                        tab += 5;
                        for( const auto& lp : p.as_list() ){
                            //This is a little harder to align nicely without knowing
                            //the length of the property as a string first...so for now,
                            //just try to get a little bit in to make it easier to read
                            std::cout<<std::setw(tab);
                            print_property(lp, tab, false);
                            std::cout<<","<<end;
                        }
                        tab -= 5;
                        std::cout<<std::setw(tab)<<" ]"<<end;
                        
                        break;
                    case geojson::PropertyType::Object:
                        //tab += 5;
                        std::cout<<std::setw(tab)<<"{\n";
                        tab += 5;
                        for( auto pair : p.get_values() ){
                            std::cout<<std::setw(tab + pair.first.length())<<pair.first<<" : ";
                            print_property(pair.second, tab, false);
                            std::cout<<",\n";
                        }
                        tab -= 5;
                        std::cout<<std::setw(tab)<<"}"<<end;
                        tab -= 5;
                };
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
                AsVectorVisitor<T> visitor(vector);
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

            bool inline operator==(const JSONProperty& other) const {
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

                return this->data == other.data;
            }

            bool inline operator!=(const JSONProperty& other) {
                return not this->operator==(other);
            }
        private:
            std::string key;
            PropertyType type;
            PropertyMap values;
            std::vector<JSONProperty> value_list;
            //boost::variant to hold the parsed data
            //can be one of boost::blank, long, double, bool, string, List, Object
            //Defaults to boost::blank
            //Note that for recurssive types, the JSONProperty holds the storage for the additional JSONProperties
            //in values (for Object) and value_list (for List).  The variant types simply point to the properties values as needed.
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
            struct AsVectorVisitor : public boost::static_visitor<>
            {
                //AsVectorVisitor takes a templated vector and stores the vector
                //reference in the struct
                AsVectorVisitor(std::vector<T>& v) : vec(v) {}
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

                //Enable this conversion only if the original template param is floating point, so there is no loss
                template <typename Floating = T, std::enable_if_t<std::is_floating_point<Floating>::value, bool> = true >
                void operator () (const long& value){
                    vec.push_back(value);
                }

                /* Can overload these with custom implementation and/or errors
                void operator () (const boost::blank&){}
                void operator () (const Object&) {}
                */
                void operator () (const List& values) {
                    //Recurse through the list to filter for types T
                    for(auto property : *values.get_values()){
                        boost::apply_visitor(*this, property.data);
                    }
                    //std::for_each(values.values->begin(), values.values->end(), boost::apply_visitor(*this));
                }

                private:
                //Storage for filtered items
                std::vector<T>& vec;
            };
    };
}
#endif // GEOJSON_JSONPROPERTY_H
