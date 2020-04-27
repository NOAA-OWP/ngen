#ifndef GEOJSON_FEATURE_H
#define GEOJSON_FEATURE_H

#include "JSONGeometry.hpp"
#include "JSONProperty.hpp"
#include "FeatureVisitor.hpp"

#include <memory>
#include <ostream>
#include <exception>
#include <string>

#include <boost/type_index.hpp>
#include <boost/property_tree/ptree.hpp>

namespace geojson {
    template<typename T>
    static bool contains(const std::vector<T*> &container, const T *value) {
        return not (std::find(container.begin(), container.end(), value) == container.end());
    }

    class FeatureBase;

    /**
     * An easy name for a smart pointer for FeatureBase and its children
     */
    typedef std::shared_ptr<FeatureBase> Feature;
            
    /**
     * Shorthand for a collection of pointers to Features
     */
    typedef std::vector<Feature> FeatureList;

    /**
     *  Describes a type of features
     */
    enum class FeatureType {
        None,                   /*!< Represents an empty feature with no sort of geometry */
        Point,                  /*!< Represents a feature that contains a single Point geometry */
        LineString,             /*!< Represents a feature that is represented by a series of interconnected points */
        Polygon,                /*!< Represents a feature that is represented by a defined area */
        MultiPoint,             /*!< Represents a feature that is represented by many points */
        MultiLineString,        /*!< Represents a feature that is represented by multiple series of interconnected points */
        MultiPolygon,           /*!< Represents a feature that is represented by multiple areas */
        GeometryCollection      /*!< Represents a feature that contains a collection of different types of geometry */
    };

    typedef std::map<std::string, JSONProperty> property_map;

    /**
     * Represents an individual feature within a Geojson definition
     */
    class FeatureBase {
        // Only provide the following constructors for children
        protected:
            /**
             * Verbose constructor for the basic attributes of a feature
             * 
             * @param new_id An id for the feature so that it may be referenced by name
             * @param new_properties A mapping between a name for a property and its values
             * @param new_bounding_box A list of double values describing the boundaries for a feature
             * @param upstream_features A series of raw pointers to features that lie upstream from this feature
             * @param downstream_features A series of raw pointers to features that lie downstream from this feature
             * @param members A mapping of foreign members for the feature between the member name and its value
             */
            FeatureBase(
                std::string new_id = "",
                PropertyMap new_properties = PropertyMap(),
                std::vector<double> new_bounding_box = std::vector<double>(),
                std::vector<FeatureBase*> upstream_features = std::vector<FeatureBase*>(),
                std::vector<FeatureBase*> downstream_features = std::vector<FeatureBase*>(),
                PropertyMap members = PropertyMap()
            ) {
                id = new_id;

                for (auto& feature : upstream_features) {
                    this->add_upstream_feature(feature);
                }

                for (auto& feature : downstream_features) {
                    this->add_downstream_feature(feature);
                }
                
                foreign_members = members;
                bounding_box = new_bounding_box;
                properties = new_properties;
            }

        public:
            /**
             * Copy Constructor
             */
            FeatureBase(const FeatureBase &feature) {
                this->id = feature.get_id();
                this->properties = feature.get_properties();
                
                for(std::string key : feature.keys()) {
                    this->set(key, feature.get(key));
                }

                this->type = feature.get_type();
                this->bounding_box = feature.get_bounding_box();

                if (feature.get_type() == FeatureType::GeometryCollection) {
                    for(auto collected_geometry : feature.get_geometry_collection()) {
                        this->geometry_collection.push_back(collected_geometry);
                    }
                }
                else {
                    this->geom = feature.geom;
                }

                for (FeatureBase *upstream_feature : feature.upstream_features()) {
                    this->upstream.push_back(upstream_feature);
                }

                for (FeatureBase *downstream_feature : feature.downstream_features()) {
                    this->downstream.push_back(downstream_feature);
                }
            }
            
            /**
             * Destructor
             */
            virtual ~FeatureBase(){};

            /**
             * Retrieve an indicator as to what type of feature this is
             * 
             * @return The type of feature for this instance
             */
            FeatureType get_type() const {
                return type;
            }

            /**
             * Set a separate feature as upstream from this instance
             * 
             * @param feature The feature to link to that is upstream from this instance
             * @param connect Whether or not to set this instance as downsteam from the newly attached 
             *                upstream feature
             */
            virtual void add_upstream_feature(FeatureBase *feature, bool connect = true) {
                if (std::find(upstream.begin(), upstream.end(), feature) == upstream.end()) {
                    upstream.push_back(feature);
                }

                if (connect) {
                    feature->add_downstream_feature(this, false);
                }
            }

            /**
             * Set a separate feature as downstream from this instance
             * 
             * @param feature The feature to link to that is downstream from this instance
             * @param connect Whether or not to set this instance as upstream from the newly attached 
             *                downstream feature
             */
            virtual void add_downstream_feature(FeatureBase *feature, bool connect = true) {
                if (not contains(downstream, feature)) {
                    downstream.push_back(feature);
                }

                if (connect) {
                    feature->add_upstream_feature(this, false);
                }
            }

            /**
             * Get a value from the set of properties
             * 
             * @param key The name of the property to get
             * @return The property identified by the key
             */
            virtual JSONProperty get_property(std::string key) const {
                return properties.at(key);
            }

            virtual void set_id(std::string new_id) {
                id = new_id;
            }

            /**
             * Get a foreign member value by name
             * 
             * @param key The name of the foreign member whose value to look for
             * @return The member value identified by the key
             */
            virtual JSONProperty get(std::string key) const {
                return foreign_members.at(key);
            }

            /**
             * Sets a foreign member value
             * 
             * @param key The name of the value to set
             * @param value The value to set
             */
            virtual void set(std::string key, short value) {
                foreign_members.emplace(key, JSONProperty(key, value));
            }

            /**
             * Sets a foreign member value
             * 
             * @param key The name of the value to set
             * @param value The value to set
             */
            virtual void set(std::string key, int value) {
                foreign_members.emplace(key, JSONProperty(key, value));
            }

            /**
             * Sets a foreign member value
             * 
             * @param key The name of the value to set
             * @param value The value to set
             */
            virtual void set(std::string key, long value) {
                foreign_members.emplace(key, JSONProperty(key, value));
            }

            /**
             * Sets a foreign member value
             * 
             * @param key The name of the value to set
             * @param value The value to set
             */
            virtual void set(std::string key, float value) {
                foreign_members.emplace(key, JSONProperty(key, value));
            }

            /**
             * Sets a foreign member value
             * 
             * @param key The name of the value to set
             * @param value The value to set
             */
            virtual void set(std::string key, double value) {
                foreign_members.emplace(key, JSONProperty(key, value));
            }

            /**
             * Sets a foreign member value
             * 
             * @param key The name of the value to set
             * @param value The value to set
             */
            virtual void set(std::string key, std::string value) {
                foreign_members.emplace(key, JSONProperty(key, value));
            }

            /**
             * Sets a foreign member value
             * 
             * @param key The name of the value to set
             * @param value The value to set
             */
            virtual void set(std::string key, JSONProperty property) {
                foreign_members.emplace(key, property);
            }

            /**
             * Retrieves a listing of all foreign member keys
             * 
             * @returns A list of all the names of foreign member values
             */
            virtual std::vector<std::string> keys() const {
                std::vector<std::string> member_keys;

                for (auto &pair : foreign_members) {
                    member_keys.push_back(pair.first);
                }

                return member_keys;
            }

            /**
             * Retireves a listing of all property keys
             * 
             * @returns A list of all the names of the properties for this feature
             */
            virtual std::vector<std::string> property_keys() const {
                std::vector<std::string> property_keys;

                for (auto& pair : properties) {
                    property_keys.push_back(pair.first);
                }

                return property_keys;
            }

            /**
             * Collects a collection of everything held within the inner geometry collection
             * 
             * @returns A collection of boost geometry objects
             * @throw Runtime Error if this feature isn't a collection type
             */
            virtual std::vector<geojson::geometry> get_geometry_collection() const {
                if (type == FeatureType::GeometryCollection) {
                    return geometry_collection;
                }

                throw std::runtime_error("There is no geometry collection to retrieve");
            }

            /**
             * Collects all values describing the bounds of this feature
             * 
             * @return A collection of double values describing the bounds for this feature
             */
            std::vector<double> get_bounding_box() const {
                return bounding_box;
            }

            PropertyMap get_properties() const {
                return properties;
            }

            std::string get_id() const {
                return id;
            }

            int get_number_of_downstream_features() {
                return downstream.size();
            }

            int get_number_of_upstream_features() {
                return upstream.size();
            }

            FeatureBase* get_downstream_feature(std::string id) {
                for (FeatureBase* feature : downstream) {
                    if (feature->get_id() == id) {
                        return feature;
                    }
                }
                return nullptr;
            }

            FeatureBase* get_upstream_feature(std::string id) {
                for (FeatureBase* feature : upstream) {
                    if (feature->get_id() == id) {
                        return feature;
                    }
                }
                return nullptr;
            }

            std::vector<FeatureBase*> upstream_features() const {
                return upstream;
            }

            std::vector<FeatureBase*> downstream_features() const {
                return downstream;
            }

            int get_upstream_length() {
                if (this->is_root()) {
                    return 0;
                }

                int longest = -1;

                for (FeatureBase *feature : upstream) {
                    int upstream_height = feature->get_upstream_length();
                    
                    if (upstream_height > longest) {
                        longest = upstream_height;
                    }
                }
                
                return longest + 1;
            }

            int get_downstream_length() {
                if (this->is_leaf()) {
                    return 0;
                }

                int longest = -1;

                for (FeatureBase *feature : this->downstream) {
                    int downstream_depth = feature->get_downstream_length();
                    
                    if (downstream_depth > longest) {
                        longest = downstream_depth;
                    }
                }

                return longest + 1;
            }

            int get_contributor_count() {
                int count = 0;

                std::vector<FeatureBase*> to_search;
                std::vector<FeatureBase*> total;

                FeatureBase *current = this;
                total.push_back(current);

                for (FeatureBase *upstream_feature : current->upstream) {
                    to_search.push_back(upstream_feature);
                }

                while (to_search.size() > 0) {
                    current = to_search.back();
                    to_search.pop_back();
                    total.push_back(current);
                    for (FeatureBase *upstream_feature : current->upstream) {
                        if (not contains(total, upstream_feature) and not contains(to_search, upstream_feature)) {
                            to_search.push_back(upstream_feature);
                        }
                    }
                }

                return total.size() - 1;
            }

            bool is_leaf() {
                return this->downstream.size() == 0;
            }

            bool is_root() {
                return this->upstream.size() == 0;
            }

            template<class T>
            T geometry() const {
                try {
                    return boost::get<T>(this->geom);
                }
                catch (boost::bad_get exception) {
                    std::string template_name = boost::typeindex::type_id<T>().pretty_name();
                    std::string expected_name = get_geometry_type(this->geom);
                    std::cerr << "Asked for " << template_name << ", but only " << expected_name << " is valid" << std::endl;
                    throw;
                }
            }

            ::geojson::geometry geometry();

            template<class T>
            T geometry(int index) const {
                try {
                    return boost::get<T>(this->geometry_collection[index]);
                }
                catch (boost::bad_get exception) {
                    std::string template_name = boost::typeindex::type_id<T>().pretty_name();
                    std::string expected_name = get_geometry_type(this->geometry_collection[index]);
                    std::cerr << "Asked for " << template_name << ", but only " << expected_name << " is valid" << std::endl;
                    throw;
                }
            }

            virtual void visit(FeatureVisitor &visitor) = 0;
        protected:
            FeatureType type;
            ::geojson::geometry geom;
            std::vector<::geojson::geometry> geometry_collection;

            PropertyMap properties;
            std::vector<double> bounding_box;
            PropertyMap foreign_members;
            std::string id;

            std::vector<FeatureBase*> upstream;
            std::vector<FeatureBase*> downstream;

            friend class FeatureCollection;
    };
}
#endif // GEOJSON_FEATURE_H