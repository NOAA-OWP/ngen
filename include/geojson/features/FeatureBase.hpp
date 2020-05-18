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

namespace bg = boost::geometry;

namespace geojson {
    template<typename T>
    static bool contains(const std::vector<T*> &container, const T *value) {
        return not (std::find(container.begin(), container.end(), value) == container.end());
    }
    template<typename T>
    static bool contains(const std::vector<T> &container, const T value) {
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
             * @param origination_features A series of raw pointers to features that lie upstream from this feature
             * @param destination_features A series of raw pointers to features that lie downstream from this feature
             * @param members A mapping of foreign members for the feature between the member name and its value
             */
            FeatureBase(
                std::string new_id = "",
                PropertyMap new_properties = PropertyMap(),
                std::vector<double> new_bounding_box = std::vector<double>(),
                std::vector<FeatureBase*> origination_features = std::vector<FeatureBase*>(),
                std::vector<FeatureBase*> destination_features = std::vector<FeatureBase*>(),
                PropertyMap members = PropertyMap()
            ) {
                id = new_id;

                for (auto& feature : origination_features) {
                    this->add_origination_feature(feature);
                }

                for (auto& feature : destination_features) {
                    this->add_destination_feature(feature);
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

                for (FeatureBase *origination_feature : feature.origination_features()) {
                    this->origination.push_back(origination_feature);
                }

                for (FeatureBase *destination_feature : feature.destination_features()) {
                    this->destination.push_back(destination_feature);
                }
            }
            
            /**
             * Destructor
             */
            virtual ~FeatureBase(){
                this->break_links();
            }

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
            virtual void add_origination_feature(FeatureBase *feature, bool connect = true) {
                if (std::find(origination.begin(), origination.end(), feature) == origination.end()) {
                    origination.push_back(feature);
                }

                if (connect) {
                    feature->add_destination_feature(this, false);
                }
            }

            /**
             * Set a separate feature as destination from this instance
             * 
             * @param feature The feature to link to that is destination from this instance
             * @param connect Whether or not to set this instance as upstream from the newly attached 
             *                destination feature
             */
            virtual void add_destination_feature(FeatureBase *feature, bool connect = true) {
                if (not contains(destination, feature)) {
                    destination.push_back(feature);
                }

                if (connect) {
                    feature->add_origination_feature(this, false);
                }
            }

            /**
             * Searches for features that share common destinations and links them
             */
            virtual void assign_neighbors() {
                for (auto destination : this->destination_features()) {
                    for (auto destination_source : destination->origination_features()) {
                        if (destination_source != this) {
                            this->add_neighbor_feature(destination_source);
                        }
                    }
                }
            }

            /**
             * Links this feature to a feature that shares a common destination
             * 
             * @param feature A feature with a common destination
             * @param connect Whether or not this feature should be linked to the neighbor as well
             */
            virtual void add_neighbor_feature(FeatureBase *feature, bool connect = true) {
                if (not contains(neighbors, feature)) {
                    neighbors.push_back(feature);
                }

                if (connect) {
                    feature->add_neighbor_feature(this, false);
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

            /**
             * Sets the ID for the Feature
             * 
             * @param new_id The new identifier for this feature
             */
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

            virtual bool has_key(std::string key) {
                std::vector<std::string> all_keys = this->keys();

                for(auto member_key : all_keys) {
                    if (member_key == key) {
                        return true;
                    }
                }

                return false;
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

            virtual bool has_property(std::string property_name) const {
                std::vector<std::string> all_properties = this->property_keys();

                for (auto name : all_properties) {
                    if (property_name == name) {
                        return true;
                    }
                }

                return false;
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

            int get_number_of_destination_features() {
                return destination.size();
            }

            int get_number_of_origination_features() {
                return origination.size();
            }

            int get_number_of_neighbors() {
                return neighbors.size();
            }

            FeatureBase* get_destination_feature(std::string id) {
                for (FeatureBase* feature : destination) {
                    if (feature->get_id() == id) {
                        return feature;
                    }
                }
                return nullptr;
            }

            FeatureBase* get_origination_feature(std::string id) {
                for (FeatureBase* feature : origination) {
                    if (feature->get_id() == id) {
                        return feature;
                    }
                }
                return nullptr;
            }

            std::vector<FeatureBase*> origination_features() const {
                return origination;
            }

            std::vector<FeatureBase*> destination_features() const {
                return destination;
            }

            std::vector<FeatureBase*> neighbor_features() const {
                return neighbors;
            }

            int get_origination_length() {
                if (this->is_root()) {
                    return 0;
                }

                int longest = -1;

                for (FeatureBase *feature : origination) {
                    int origination_height = feature->get_origination_length();
                    
                    if (origination_height > longest) {
                        longest = origination_height;
                    }
                }
                
                return longest + 1;
            }

            int get_destination_length() {
                if (this->is_leaf()) {
                    return 0;
                }

                int longest = -1;

                for (FeatureBase *feature : this->destination) {
                    int destination_depth = feature->get_destination_length();
                    
                    if (destination_depth > longest) {
                        longest = destination_depth;
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

                for (FeatureBase *origination_feature : current->origination) {
                    to_search.push_back(origination_feature);
                }

                while (to_search.size() > 0) {
                    current = to_search.back();
                    to_search.pop_back();
                    total.push_back(current);
                    for (FeatureBase *origination_feature : current->origination) {
                        if (not contains(total, origination_feature) and not contains(to_search, origination_feature)) {
                            to_search.push_back(origination_feature);
                        }
                    }
                }

                return total.size() - 1;
            }

            bool is_leaf() {
                return this->destination.size() == 0;
            }

            bool is_root() {
                return this->origination.size() == 0;
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

            inline bool operator==(const FeatureBase& rhs) {
                if (this->get_id() != rhs.get_id()) {
                    return false;
                }

                if (not (this->get_type() == rhs.get_type())) {
                    return false;
                }

                if (this->bounding_box != rhs.bounding_box) {
                    return false;
                }

                if (this->get_type() == FeatureType::GeometryCollection) {
                    if (this->geometry_collection.size() != rhs.geometry_collection.size()) {
                        return false;
                    }

                    for (int geometry_index = 0; geometry_index < this->geometry_collection.size(); geometry_index++) {
                        if (not bg::equals(this->geometry_collection[geometry_index], rhs.geometry_collection[geometry_index])) {
                            return false;
                        }
                    }
                }
                else if (not bg::equals(this->geom, rhs.geom)) {
                    return false;
                }

                if (this->keys().size() != rhs.keys().size() or this->property_keys().size() != rhs.property_keys().size()) {
                    return false;
                }

                std::vector<std::string> these_keys = this->keys();
                std::vector<std::string> these_property_keys = this->property_keys();

                for (std::string key : rhs.keys()) {
                    if (not contains(these_keys, key) or this->get(key) != rhs.get(key)) {
                        return false;
                    }
                }
                
                for (std::string key : rhs.property_keys()) {
                    if (not contains(these_keys, key) or this->get_property(key) != rhs.get_property(key)) {
                        return false;
                    }
                }

                if (
                    not (
                        this->origination.size() == rhs.origination.size() 
                            or this->destination.size() == rhs.destination.size() 
                            or this->neighbors.size() == rhs.neighbors.size()
                    )
                ) {
                    return false;
                }

                for (auto feature : rhs.origination) {
                    if (not contains(this->origination, feature)) {
                        return false;
                    }
                }

                for (auto feature : rhs.destination) {
                    if (not contains(this->destination, feature)) {
                        return false;
                    }
                }

                for (auto feature : rhs.neighbors) {
                    if (not contains(this->neighbors, feature)) {
                        return false;
                    }
                }
                
                return true; 
            }

            inline bool operator!=(const FeatureBase &other) {
                return not this->operator==(other);
            }

        protected:
            virtual void break_links() {
                for (auto originator : this->origination) {
                    originator->remove_destination(this);
                }

                this->origination.clear();

                for (auto target : this->destination) {
                    target->remove_origination(this);
                }

                this->destination.clear();

                for (auto neighbor : this->neighbors) {
                    neighbor->remove_neighbor(this);
                }

                this->neighbors.clear();
            }

            virtual void remove_destination(FeatureBase* feature) {
                int feature_index = 0;

                for (; feature_index < this->destination.size(); feature_index++) {
                    if (feature == this->destination[feature_index]) {
                        break;
                    }
                }

                this->destination.erase(this->destination.begin() + feature_index);
            }

            virtual void remove_origination(FeatureBase* feature) {
                int feature_index = 0;

                for (; feature_index < this->origination.size(); feature_index++) {
                    if (feature == this->origination[feature_index]) {
                        break;
                    }
                }

                this->origination.erase(this->origination.begin() + feature_index);
            }

            virtual void remove_neighbor(FeatureBase* feature) {
                int feature_index = 0;

                for (; feature_index < this->neighbors.size(); feature_index++) {
                    if (feature == this->neighbors[feature_index]) {
                        break;
                    }
                }

                this->neighbors.erase(this->neighbors.begin() + feature_index);
            }

            FeatureType type;
            ::geojson::geometry geom;
            std::vector<::geojson::geometry> geometry_collection;

            PropertyMap properties;
            std::vector<double> bounding_box;
            PropertyMap foreign_members;
            std::string id;

            std::vector<FeatureBase*> origination;
            std::vector<FeatureBase*> destination;
            std::vector<FeatureBase*> neighbors;

            friend class FeatureCollection;
    };
}
#endif // GEOJSON_FEATURE_H