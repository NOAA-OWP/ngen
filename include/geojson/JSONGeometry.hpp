#ifndef GEOJSON_GEOMETRY_H
#define GEOJSON_GEOMETRY_H

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/property_tree/ptree.hpp>

namespace bg = boost::geometry;
namespace pt = boost::property_tree;

namespace geojson {
    typedef bg::model::point<double, 2, bg::cs::geographic<bg::degree>> coordinate_t;
    typedef bg::model::linestring<coordinate_t> linestring_t;
    typedef bg::model::polygon<coordinate_t> polygon_t;

    typedef std::vector<std::vector<double>> two_dimensional_coordinates;
    typedef std::vector<std::vector<std::vector<double>>> three_dimensional_coordinates;
    typedef std::vector<std::vector<std::vector<std::vector<double>>>> four_dimensional_coordinates;

    enum class JSONGeometryType {
        LineString,
        Point,
        Polygon,
        MultiLineString,
        MultiPoint,
        MultiPolygon
    };

    class JSONGeometry {
        public:
            JSONGeometry(linestring_t &linestring)
                : type(JSONGeometryType::LineString),
                linestring(linestring)
            {}

            JSONGeometry(coordinate_t &point)
                : type(JSONGeometryType::Point),
                point(point)
            {}

            JSONGeometry(polygon_t &polygon)
                : type(JSONGeometryType::Polygon),
                polygon(polygon)
            {}

            JSONGeometry(bg::model::multi_point<coordinate_t> &multipoint)
                : type(JSONGeometryType::MultiPoint),
                multi_point(multipoint)
            {}

            JSONGeometry(bg::model::multi_linestring<linestring_t> &multilinestring)
                : type(JSONGeometryType::MultiLineString),
                multi_linestring(multilinestring)
            {}

            JSONGeometry(bg::model::multi_polygon<polygon_t> &multipolygon)
                : type(JSONGeometryType::MultiPolygon),
                multi_polygon(multipolygon)
            {}

            JSONGeometry(const JSONGeometry &json_geometry) {
                switch (json_geometry.get_type()) {
                    case JSONGeometryType::Point:
                        point = json_geometry.as_point();
                        break;
                    case JSONGeometryType::LineString:
                        linestring = json_geometry.as_linestring();
                        break;
                    case JSONGeometryType::Polygon:
                        polygon = json_geometry.as_polygon();
                        break;
                    case JSONGeometryType::MultiPoint:
                        multi_point = json_geometry.as_multipoint();
                        break;
                    case JSONGeometryType::MultiLineString:
                        multi_linestring = json_geometry.as_multilinestring();
                        break;
                    case JSONGeometryType::MultiPolygon:
                        multi_polygon = json_geometry.as_multipolygon();
                        break;
                }

                type = json_geometry.get_type();
            }
            static JSONGeometry of_point(double x, double y) {
                coordinate_t point(x, y);
                return JSONGeometry(point);
            }

            static JSONGeometry of_point(pt::ptree &tree) {
                double x;
                double y;

                // TODO: Extract x and y from ptree

                return of_point(x, y);
            }

            static JSONGeometry of_linestring(two_dimensional_coordinates &coordinates) {
                linestring_t line;

                for (auto &pair : coordinates) {
                    bg::append(line, coordinate_t(pair[0], pair[1]));
                }

                return JSONGeometry(line);
            }

            static JSONGeometry of_linestring(pt::ptree &tree) {
                two_dimensional_coordinates coordinates;

                // TODO: Extract coordinates from ptree

                return of_linestring(coordinates);
            }

            static JSONGeometry of_polygon(two_dimensional_coordinates &coordinates) {
                polygon_t polygon;

                for (auto &parts : coordinates) {
                    bg::append(polygon, coordinate_t(parts[0], parts[1]));
                }

                return JSONGeometry(polygon);
            }

            static JSONGeometry of_polygon(pt::ptree &tree) {
                two_dimensional_coordinates coordinates;

                // TODO: Extract coordinates from ptree
                
                return of_polygon(coordinates);
            }

            static JSONGeometry of_multipoint(two_dimensional_coordinates &coordinates) {
                bg::model::multi_point<coordinate_t> points;

                for (auto &point: coordinates) {
                    bg::append(points, coordinate_t(point[0], point[1]));
                }

                return JSONGeometry(points);
            }

            static JSONGeometry of_multipoint(pt::ptree &tree) {
                two_dimensional_coordinates coordinates;

                // TODO: Extract coordinates from ptree
                
                return of_multipoint(coordinates);
            }

            static JSONGeometry of_multilinestring(three_dimensional_coordinates &coordinates) {
                bg::model::multi_linestring<linestring_t> multi_linestring;

                for (auto &linestring : coordinates) {
                    linestring_t line;

                    for(auto &coordinate : linestring) {
                        bg::append(line, coordinate_t(coordinate[0], coordinate[1]));
                    }

                    multi_linestring.push_back(line);
                }

                return JSONGeometry(multi_linestring);
            }

            static JSONGeometry of_multilinestring(pt::ptree &tree) {
                three_dimensional_coordinates coordinates;

                // TODO: Extract coordinates from ptree
                
                return of_multilinestring(coordinates);
            }

            static JSONGeometry of_multipolygon(four_dimensional_coordinates &coordinates) {
                bg::model::multi_polygon<polygon_t> multi_polygon;

                for (auto& polygon_group : coordinates) {
                    for (auto &polygon_points : polygon_group) {
                        polygon_t polygon;

                        for (auto &point : polygon_points) {
                            bg::append(polygon, coordinate_t(point[0], point[1]));
                        }

                        multi_polygon.push_back(polygon);
                    }
                }

                return JSONGeometry(multi_polygon);
            }

            static JSONGeometry of_multipolygon(pt::ptree &tree) {
                four_dimensional_coordinates coordinates;

                // TODO: Extract coordinates from ptree
                
                return of_multipolygon(coordinates);
            }

            static JSONGeometry from_ptree(pt::ptree &tree) {
                // TODO: Detect type of feature, then feed the ptree to the appropriate of_* function
            }

            virtual ~JSONGeometry(){};

            JSONGeometryType get_type() const {
                return type;
            };
            linestring_t as_linestring() const {
                return linestring;
            }
            coordinate_t as_point() const {
                return point;
            }
            polygon_t as_polygon() const {
                return polygon;
            }
            bg::model::multi_point<coordinate_t> as_multipoint() const {
                return multi_point;
            }
            bg::model::multi_linestring<linestring_t> as_multilinestring() const {
                return multi_linestring;
            }
            bg::model::multi_polygon<polygon_t> as_multipolygon() const {
                return multi_polygon;
            }
        private:
            JSONGeometryType type;
            linestring_t linestring;
            coordinate_t point;
            polygon_t polygon;
            bg::model::multi_point<coordinate_t> multi_point;
            bg::model::multi_linestring<linestring_t> multi_linestring;
            bg::model::multi_polygon<polygon_t> multi_polygon;
    };
}

#endif // GEOJSON_GEOMETRY_H