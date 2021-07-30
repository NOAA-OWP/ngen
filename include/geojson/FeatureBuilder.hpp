#ifndef GEOJSON_FEATURE_BUILDER_H
#define GEOJSON_FEATURE_BUILDER_H

#include <features/Features.hpp>
#include <FeatureCollection.hpp>
#include <JSONGeometry.hpp>

#include <iostream>
#include <memory>
#include <ostream>
#include <exception>
#include <string>
#include <algorithm>

#include <boost/property_tree/ptree.hpp>

namespace geojson {
    /**
     * Easy short-hand for a smart pointer to a FeatureCollection
     */
    typedef std::shared_ptr<FeatureCollection> GeoJSON;

    /**
     * Creates a single boost geometry point from the values in a boost property tree
     * 
     * @param tree A boost geometry tree that should contain the properties 
     *             of a single point
     * @return A boost geometry point specified by the property tree
     */
    static coordinate_t build_point(boost::property_tree::ptree &tree) {
        std::vector<double> point;

        // GeoJSON specifies that the details of the point will lie in a 1D array under "coordinates"
        for(auto &shape : tree.get_child("coordinates")) {
            // The specification of the points will lie within string values, so we need to convert the strings 
            // to double values
            point.push_back(std::stod(shape.second.data()));
        }

        // The specification of GeoJson specifies that points will be described within a 1D array of length 2
        // (the longitude, then the latitude). Use both in order to construct the point and return it
        return coordinate_t(point[0], point[1]);
    }

    /**
     * @brief Reads a ptree entry and converts it into a linestring
     * 
     * The input should be a boost property tree that looks like:
     * 
     * - feature:
     *     - "type": "doesn't matter here"
     *     - "coordinates": [[0,0], [1,1], [2,2]]
     * 
     * @param tree A property tree node describing a line strig
     * @return a boost geometry linestring 
     */
    static linestring_t build_linestring(boost::property_tree::ptree &tree) {
        linestring_t line;
        
        for (auto &point : tree.get_child("coordinates")) {
            std::vector<double> points;

            for (auto &coordinate : point.second) {
                points.push_back(std::stod(coordinate.second.data()));
            }
            
            bg::append(line, coordinate_t(points[0], points[1]));
        }

        return line;
    }

    static polygon_t build_polygon(boost::property_tree::ptree &tree) {
        three_dimensional_coordinates total_coordinates;
        
        for(auto &shape : tree.get_child("coordinates")) {
            
            two_dimensional_coordinates shape_coordinates;
            for (auto &group : shape.second) {
                std::vector<double> latLon;
                for (auto &coordinate : group.second) {
                    latLon.push_back(std::stod(coordinate.second.data()));
                }
                shape_coordinates.push_back(latLon);
            }
            total_coordinates.push_back(shape_coordinates);
            
        }

        polygon_t polygon;

        if (total_coordinates.size() > 1) {
            polygon.inners().resize(total_coordinates.size() - 1);
        }

        bool is_outer = true;
        int inner_index = 0;

        for (auto& shape : total_coordinates) {
            if (is_outer) {
                
                for(auto &coords : shape) {
                    polygon.outer().push_back(
                        coordinate_t(coords[0], coords[1])
                    );
                }
                
                is_outer = false;
            }
            else {
                
                for (auto &coords : shape) {
                    
                    polygon.inners()[inner_index].push_back(
                        coordinate_t(coords[0], coords[1])
                    );
                    
                }
                inner_index++;
            }
        }

        return polygon;
    }

    static multipoint_t build_multipoint(boost::property_tree::ptree &tree) {
        bg::model::multi_point<coordinate_t> coordinates;
        
        for (auto &point : tree.get_child("coordinates")) {
            std::vector<double> pair;
            for (auto &coordinate : point.second) {
                pair.push_back(std::stod(coordinate.second.data()));
            }
            bg::append(coordinates, coordinate_t(pair[0], pair[1]));
        }

        return coordinates;
    }

    static multilinestring_t build_multilinestring(boost::property_tree::ptree &tree) {
        three_dimensional_coordinates coordinates;
        bg::model::multi_linestring<linestring_t> multi_linestring;

        for (auto &line : tree.get_child("coordinates")) {
            linestring_t linestring;

            for(auto &point : line.second) {
                std::vector<double> latLon;
                bool x_is_set = false;

                for(auto &coordinate : point.second) {
                    latLon.push_back(std::stod(coordinate.second.data()));
                }

                bg::append(linestring, coordinate_t(latLon[0], latLon[1]));
            }

            multi_linestring.push_back(linestring);
        }

        return multi_linestring;
    }

    static multipolygon_t build_multipolygon(boost::property_tree::ptree &tree) {
        four_dimensional_coordinates coordinates_groups;
        for (auto &polygon : tree.get_child("coordinates")) {
            three_dimensional_coordinates polygon_coordinates;
            for (auto &group : polygon.second) {
                two_dimensional_coordinates coordinate_group;
                for (auto &point : group.second) {
                    std::vector<double> latLon;
                    for(auto &coordinate : point.second) {
                        latLon.push_back(std::stod(coordinate.second.data()));
                    }

                    coordinate_group.push_back(latLon);
                }

                polygon_coordinates.push_back(coordinate_group);
            }

            coordinates_groups.push_back(polygon_coordinates);
        }

        bg::model::multi_polygon<polygon_t> multi_polygon;

        for (auto& polygon_group : coordinates_groups) {
            for (auto &polygon_points : polygon_group) {
                polygon_t polygon;
                for (auto &point : polygon_points) {
                    bg::append(polygon, coordinate_t(point[0], point[1]));
                }
                multi_polygon.push_back(polygon);
            }
        }

        return multi_polygon;
    }

    static geometry build_geometry(boost::property_tree::ptree &tree) {
        std::string type = tree.get<std::string>("type");

        if (type == "Point") {
            
            return build_point(tree);
        }
        else if (type == "LineString") {
            
            return build_linestring(tree);
        }
        else if (type == "Polygon") {
            
            return build_polygon(tree);
        }
        else if (type == "MultiPoint") {
            
            return build_multipoint(tree);
        }
        else if (type == "MultiLineString") {
            
            return build_multilinestring(tree);
        }
        else if (type == "MultiPolygon") {            
            return build_multipolygon(tree);
        }

        throw std::invalid_argument("tree");
    }

    static Feature build_feature(boost::property_tree::ptree &tree) {
        bool has_geometry_collection = false;
        bool has_geometry = false;

        geometry geometry_object;
        std::vector<geometry> geometry_collection;
        FeatureType type = FeatureType::None;
        std::string id = "";
        std::vector<double> bounding_box;
        PropertyMap properties;
        PropertyMap foreign_members;

        for (auto& child : tree) {
            if (child.first == "geometry") {
                std::string geometry_type = child.second.get<std::string>("type");
                geometry_object = build_geometry(child.second);
                has_geometry = true;

                if (geometry_type == "Point") {
                    type = FeatureType::Point;
                }
                else if (geometry_type == "LineString") {
                    type = FeatureType::LineString;
                }
                else if (geometry_type == "Polygon") {
                    type = FeatureType::Polygon;
                }
                else if (geometry_type == "MultiPoint") {
                    type = FeatureType::MultiPoint;
                }
                else if (geometry_type == "MultiLineString") {
                    type = FeatureType::MultiLineString;
                }
                else if (geometry_type == "MultiPolygon") {
                    type = FeatureType::MultiPolygon;
                }
            }
            else if (child.first == "geometries") {
                // Since the feature can have a number of different types of geometries and the
                // type of the feature comes from the geometry, we simply set this as a collection
                type = FeatureType::GeometryCollection;
                has_geometry_collection = true;

                // Loop through the underlying collection of geometric json definitions and use
                // those to create geometric objects
                for (auto &geom : child.second) {
                    geometry_collection.push_back(build_geometry(geom.second));
                }
            }
            else if (child.first == "id") {
                id = child.second.data();
            }
            else if (child.first == "bbox") {
                for (auto &value : tree.get_child("bbox")) {
                    bounding_box.push_back(std::stod(value.second.data()));
                }
            }
            else if (child.first == "properties") {
                for (auto& property : child.second) {
                    properties.emplace(property.first, JSONProperty(property.first, property.second));
                }
            }
            else {
                foreign_members.emplace(child.first, JSONProperty(child.first, child.second));
            }
        }

        
        switch (type) {
            case FeatureType::Point:
                return std::make_shared<PointFeature>(PointFeature(
                    boost::get<coordinate_t>(geometry_object),
                    id,
                    properties,
                    bounding_box,
                    std::vector<FeatureBase*>(),
                    std::vector<FeatureBase*>(),
                    foreign_members
                ));
            case FeatureType::LineString:
                return std::make_shared<LineStringFeature>(LineStringFeature(
                    boost::get<linestring_t>(geometry_object),
                    id,
                    properties,
                    bounding_box,
                    std::vector<FeatureBase*>(),
                    std::vector<FeatureBase*>(),
                    foreign_members
                ));
            case FeatureType::Polygon:
                return std::make_shared<PolygonFeature>(PolygonFeature(
                    boost::get<polygon_t>(geometry_object),
                    id,
                    properties,
                    bounding_box,
                    std::vector<FeatureBase*>(),
                    std::vector<FeatureBase*>(),
                    foreign_members
                ));
            case FeatureType::MultiPoint:
                return std::make_shared<MultiPointFeature>(MultiPointFeature(
                    boost::get<multipoint_t>(geometry_object),
                    id,
                    properties,
                    bounding_box,
                    std::vector<FeatureBase*>(),
                    std::vector<FeatureBase*>(),
                    foreign_members
                ));
            case FeatureType::MultiLineString:
                return std::make_shared<MultiLineStringFeature>(MultiLineStringFeature(
                    boost::get<multilinestring_t>(geometry_object),
                    id,
                    properties,
                    bounding_box,
                    std::vector<FeatureBase*>(),
                    std::vector<FeatureBase*>(),
                    foreign_members
                ));
            case FeatureType::MultiPolygon:
                return std::make_shared<MultiPolygonFeature>(MultiPolygonFeature(
                    boost::get<multipolygon_t>(geometry_object),
                    id,
                    properties,
                    bounding_box,
                    std::vector<FeatureBase*>(),
                    std::vector<FeatureBase*>(),
                    foreign_members
                ));
            default:                
                return std::make_shared<CollectionFeature>(CollectionFeature(
                    geometry_collection,
                    id,
                    properties,
                    bounding_box,
                    std::vector<FeatureBase*>(),
                    std::vector<FeatureBase*>(),
                    foreign_members
                ));
        }
    }

    /**
     * @brief helper function to build a GeoJSON FeatureCollection from a property tree
     * @param tree boost::property_tree::ptree holding the parsed GeoJSON
     * @param ids optional subset of string feature ids, only features in tree with these ids will be in the collection
     */
    static GeoJSON build_collection(const boost::property_tree::ptree tree, const std::vector<std::string> ids={}) {
        std::vector<double> bbox_values;
        std::vector<Feature> features;
        PropertyMap foreign_members;
        std::string tmp_id;  //a temporary string to hold feature identities
        
        for (auto& child : tree) {
            if (child.first == "bbox") {
                std::vector<std::string> bounding_box;
                for (auto &feature : tree.get_child("bbox")) {
                    bounding_box.push_back(feature.second.data());
                }

                for (int point_index = 0; point_index < bounding_box.size(); point_index++) {
                    bbox_values.push_back(std::stod(bounding_box[point_index]));
                }
            }
            else if (child.first == "features") {
                boost::optional<const boost::property_tree::ptree&> e = tree.get_child_optional("features");

                if (e) {
                    for(auto feature_tree : *e) {
                        Feature feature = build_feature(feature_tree.second);
                        tmp_id = feature->get_id();
                        //TODO feature identity isn't 100% spec compliant.  GeoJSON allows for a feature to have an
                        //optional id, but the input files set id under the 'property' key, so when a feature is constructed
                        //feature->get_id() returns '' because the feature itself doesn't have an id, so we hae to read
                        //from the associated properties to find the id.  If the inputs change, we will need to adjust
                        //this line to read feature->get_id()
                        if( tmp_id == "" ) {
                          try {
                            tmp_id = feature->get_property("id").as_string();
                            feature->set_id(tmp_id);
                          }
                          catch (const std::out_of_range& error) {
                            tmp_id = "";
                          }
                        }
                        
                        if( ids.empty() || std::find(ids.begin(), ids.end(), tmp_id) != ids.end() ) {
                          //ids is empty, meaning we want all features,
                          //or feature id was found in the provided ids vector
                          //so hold the feature to add to collection later

                          features.push_back(feature);
                        }
                    }
                }
                else {
                    std::cout << "No features were found" << std::endl;
                }
            }
            else {
                foreign_members.emplace(child.first, JSONProperty(child.first, child.second));
            }
        }

        GeoJSON collection = std::make_shared<FeatureCollection>(FeatureCollection(features, bbox_values));

        for (Feature feature : features) {
            if (feature->get_id() != "") {
                collection->add_feature_id(feature->get_id(), feature);
            }
        }

        return collection;
    }

    static GeoJSON read(const std::string &file_path, const std::vector<std::string> &ids = {}) {
        boost::property_tree::ptree tree;
        boost::property_tree::json_parser::read_json(file_path, tree);
        return build_collection(tree, ids);
    }

    static GeoJSON read(std::stringstream &data, const std::vector<std::string> &ids = {}) {
        boost::property_tree::ptree tree;
        boost::property_tree::json_parser::read_json(data, tree);
        return build_collection(tree, ids);
    }


}

#endif // GEOJSON_FEATURE_BUILDER_H
