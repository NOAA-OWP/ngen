#ifndef GEOJSON_COLLECTION_FEATURE_H
#define GEOJSON_COLLECTION_FEATURE_H

#include "FeatureBase.hpp"
#include <FeatureVisitor.hpp>
#include <JSONGeometry.hpp>

#include <string>
#include <vector>
#include <map>
#include <exception>

namespace geojson {
    class CollectionFeature : public FeatureBase {
        public:
            CollectionFeature(const FeatureBase& feature) : FeatureBase(feature) {}

            CollectionFeature(const CollectionFeature& feature) : FeatureBase(feature) {
                this->geometry_collection = feature.get_geometry_collection();
                this->type = geojson::FeatureType::GeometryCollection;
            }

            CollectionFeature(
                std::vector<geojson::geometry> geometry_collection,
                std::string new_id = "",
                PropertyMap new_properties = PropertyMap(),
                std::vector<double> new_bounding_box = std::vector<double>(),
                std::vector<FeatureBase*> upstream_features = std::vector<FeatureBase*>(),
                std::vector<FeatureBase*> downstream_features = std::vector<FeatureBase*>(),
                std::map<std::string, JSONProperty> members = std::map<std::string, JSONProperty>()
            ) : FeatureBase(new_id, new_properties, new_bounding_box, upstream_features, downstream_features, members) {
                this->geometry_collection = geometry_collection;
                this->type = geojson::FeatureType::GeometryCollection;
            }

            coordinate_t point(int index) {
                try {
                    return boost::get<coordinate_t>(this->geometry_collection[index]);
                }
                catch (boost::bad_get &exception) {
                    std::string template_name = "Point";
                    std::string expected_name = get_geometry_type(this->geometry_collection[index]);
                    std::cerr << "Asked for " << template_name << ", but only " << expected_name << " is valid" << std::endl;
                    throw;
                }
            }

            std::vector<coordinate_t*> points() {
                std::vector<coordinate_t*> point_geometries;

                for (geojson::geometry& geometry : this->geometry_collection) {
                    if (geometry.which() == 0) {
                        try {
                            point_geometries.push_back(&boost::get<coordinate_t>(geometry));
                        }
                        catch (boost::bad_get &exception) {
                            std::string template_name = "Point";
                            std::string expected_name = get_geometry_type(geometry);
                            std::cerr << "Asked for " << template_name << ", but only " << expected_name << " is valid" << std::endl;
                            throw;
                        }
                    }
                }

                return point_geometries;
            }

            linestring_t linestring(int index) {
                try {
                    return boost::get<linestring_t>(this->geometry_collection[index]);
                }
                catch (boost::bad_get &exception) {
                    std::string template_name = "LineString";
                    std::string expected_name = get_geometry_type(this->geometry_collection[index]);
                    std::cerr << "Asked for " << template_name << ", but only " << expected_name << " is valid" << std::endl;
                    throw;
                }
            }
            
            std::vector<linestring_t*> linestrings() {
                std::vector<linestring_t*> linestring_geometries;

                for (geojson::geometry& geometry : this->geometry_collection) {
                    if (geometry.which() == 1) {
                        try {
                            linestring_geometries.push_back(&boost::get<linestring_t>(geometry));
                        }
                        catch (boost::bad_get &exception) {
                            std::string template_name = "LineString";
                            std::string expected_name = get_geometry_type(geometry);
                            std::cerr << "Asked for " << template_name << ", but only " << expected_name << " is valid" << std::endl;
                            throw;
                        }
                    }
                }

                return linestring_geometries;
            }

            polygon_t polygon(int index) {
                try {
                    return boost::get<polygon_t>(this->geometry_collection[index]);
                }
                catch (boost::bad_get &exception) {
                    std::string template_name = "Polygon";
                    std::string expected_name = get_geometry_type(this->geometry_collection[index]);
                    std::cerr << "Asked for " << template_name << ", but only " << expected_name << " is valid" << std::endl;
                    throw;
                }
            }

            std::vector<polygon_t*> polygons() {
                std::vector<polygon_t*> polygon_geometries;

                for (geojson::geometry& geometry : this->geometry_collection) {
                    if (geometry.which() == 2) {
                        try {
                            polygon_geometries.push_back(&boost::get<polygon_t>(geometry));
                        }
                        catch (boost::bad_get &exception) {
                            std::string template_name = "Polygon";
                            std::string expected_name = get_geometry_type(geometry);
                            std::cerr << "Asked for " << template_name << ", but only " << expected_name << " is valid" << std::endl;
                            throw;
                        }
                    }
                }

                return polygon_geometries;
            }

            multipoint_t multipoint(int index) {
                try {
                    return boost::get<multipoint_t>(this->geometry_collection[index]);
                }
                catch (boost::bad_get &exception) {
                    std::string template_name = "MultiPoint";
                    std::string expected_name = get_geometry_type(this->geometry_collection[index]);
                    std::cerr << "Asked for " << template_name << ", but only " << expected_name << " is valid" << std::endl;
                    throw;
                }
            }

            std::vector<multipoint_t*> multipoints() {
                std::vector<multipoint_t*> multipoint_geometries;

                for (geojson::geometry& geometry : this->geometry_collection) {
                    if (geometry.which() == 2) {
                        multipoint_geometries.push_back(&boost::get<multipoint_t>(geometry));
                    }
                }

                return multipoint_geometries;
            }

            multilinestring_t multilinestring(int index) {
                try {
                    return boost::get<multilinestring_t>(this->geometry_collection[index]);
                }
                catch (boost::bad_get &exception) {
                    std::string template_name = "MultiLineString";
                    std::string expected_name = get_geometry_type(this->geometry_collection[index]);
                    std::cerr << "Asked for " << template_name << ", but only " << expected_name << " is valid" << std::endl;
                    throw;
                }
            }

            std::vector<multilinestring_t*> multilinestrings() {
                std::vector<multilinestring_t*> multilinestring_geometries;

                for(geojson::geometry& geometry : this->geometry_collection) {
                    if (geometry.which() == 4) {
                        multilinestring_geometries.push_back(&boost::get<multilinestring_t>(geometry));
                    }
                }

                return multilinestring_geometries;
            }

            multipolygon_t multipolygon(int index) {
                try {
                    return boost::get<multipolygon_t>(this->geometry_collection[index]);
                }
                catch (boost::bad_get &exception) {
                    std::string template_name = "MultiPolygon";
                    std::string expected_name = get_geometry_type(this->geometry_collection[index]);
                    std::cerr << "Asked for " << template_name << ", but only " << expected_name << " is valid" << std::endl;
                    throw;
                }
            }

            std::vector<multipolygon_t*> multipolygons() {
                std::vector<multipolygon_t*> multipolygon_geometries;

                for(geojson::geometry& geometry : this->geometry_collection) {
                    if (geometry.which() == 5) {
                        multipolygon_geometries.push_back(&boost::get<multipolygon_t>(geometry));
                    }
                }
                
                return multipolygon_geometries;
            }

            std::vector<geojson::geometry>::const_iterator begin() {
                return this->geometry_collection.cbegin();
            }

            std::vector<geojson::geometry>::const_iterator end() {
                return this->geometry_collection.cend();
            }

            void visit(FeatureVisitor& visitor) override {
                visitor.visit(this);
            }
    };
}

#endif // GEOJSON_COLLECTION_FEATURE_H
