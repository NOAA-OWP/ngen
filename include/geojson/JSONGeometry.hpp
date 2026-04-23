#ifndef GEOJSON_GEOMETRY_H
#define GEOJSON_GEOMETRY_H

#include <exception>
#include <string>
#include <iostream>
#include <memory>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/property_tree/ptree.hpp>

namespace bg = boost::geometry;
namespace pt = boost::property_tree;

namespace geojson {
    /**
     * A two dimensional point in geographic degrees
     */
    typedef bg::model::point<double, 2, bg::cs::geographic<bg::degree>> coordinate_t;

    /**
     * A line made up of points in geographic degrees
     */
    typedef bg::model::linestring<coordinate_t> linestring_t;

    /**
     * A polygon made up of points in geographic degrees
     */
    typedef bg::model::polygon<coordinate_t> polygon_t;

    typedef bg::model::multi_point<coordinate_t> multipoint_t;

    typedef bg::model::multi_linestring<linestring_t> multilinestring_t;

    typedef bg::model::multi_polygon<polygon_t> multipolygon_t;

    using box_t = boost::geometry::model::box<geojson::coordinate_t>;

    /**
     * A two dimensional matrix of doubles
     */
    typedef std::vector<std::vector<double>> two_dimensional_coordinates;

    /**
     * A three dimensional matrix of doubles
     */
    typedef std::vector<std::vector<std::vector<double>>> three_dimensional_coordinates;

    /**
     * A four dimensional matrix of doubles
     */
    typedef std::vector<std::vector<std::vector<std::vector<double>>>> four_dimensional_coordinates;

    /**
     * Represents the type of geometry to be represented within a JSONGeometry
     */
    enum class JSONGeometryType {
        None,  /*<! Represents a non-existent geometry */
        LineString, /*<! Represents a series of interconnected points */
        Point,  /*<! Represents a point represented by an x and a y coordinate */
        Polygon, /*<! Represents multiple points that form a shape */
        MultiLineString, /*<! Represents a series of series of interconnected points */
        MultiPoint, /*<! Represents multiple points represented by an x and y coordinate */
        MultiPolygon /*<! Represents a series of shapes formed by several points */
    };

    typedef boost::variant<coordinate_t, linestring_t, polygon_t, multipoint_t, multilinestring_t, multipolygon_t> geometry;

    template<typename T>
    T get_shape(geometry geom) {
        return boost::get<T>(geom);
    }

    inline std::string get_geometry_type(const geometry& geom) {
        switch (geom.which()) {
            case 0:
                return "Point";
            case 1:
                return "LineString";
            case 2:
                return "Polygon";
            case 3:
                return "MultiPoint";
            case 4:
                return "MultiLineString";
            default:
                return "MultiPolygon";
        }
    }

    static coordinate_t point(const double x, const double y) {
        return coordinate_t(x, y);
    }

    static linestring_t linestring(const two_dimensional_coordinates &coordinates) {
        linestring_t line;
        for (auto &pair : coordinates) {
            bg::append(line, coordinate_t(pair[0], pair[1]));
        }
        return line;
    }

    static polygon_t polygon(three_dimensional_coordinates &coordinates) {
        polygon_t polygon;

        bool is_outer = true;
        int inner_index = 0;

        if (coordinates.size() > 1) {
            polygon.inners().resize(coordinates.size() - 1);
        }

        for (auto &shape : coordinates) {
            if (is_outer) {
                for (auto &shape_coordinates : shape) {
                    polygon.outer().push_back(
                        coordinate_t(shape_coordinates[0], shape_coordinates[1])
                    );
                }
                is_outer = false;
            }
            else {
                for (auto &shape_coordinates : shape) {
                    polygon.inners()[inner_index].push_back(
                        coordinate_t(shape_coordinates[0], shape_coordinates[1])
                    );
                }
                inner_index++;
            }
        }

        return polygon;
    }

    static multipoint_t multipoint(two_dimensional_coordinates &coordinates) {
        bg::model::multi_point<coordinate_t> points;

        for (auto &point: coordinates) {
            bg::append(points, coordinate_t(point[0], point[1]));
        }

        return points;
    }

    static multilinestring_t multilinestring(three_dimensional_coordinates &coordinates) {
        bg::model::multi_linestring<linestring_t> multi_linestring;

        for (auto &linestring : coordinates) {
            linestring_t line;

            for(auto &coordinate : linestring) {
                bg::append(line, coordinate_t(coordinate[0], coordinate[1]));
            }

            multi_linestring.push_back(line);
        }

        return multi_linestring;
    }

    static multipolygon_t multipolygon(four_dimensional_coordinates &coordinates) {
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

        return multi_polygon;
    }
}

#endif // GEOJSON_GEOMETRY_H
