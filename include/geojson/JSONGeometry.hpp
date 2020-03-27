#ifndef GEOJSON_GEOMETRY_H
#define GEOJSON_GEOMETRY_H

#include <exception>
#include <string>
#include <typeinfo>

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
        None,
        LineString,
        Point,
        Polygon,
        MultiLineString,
        MultiPoint,
        MultiPolygon
    };

    /**
     * A wrapper for boost geometry objects
     */
    class JSONGeometry {
        public:
            /**
             * The default constructor; creates a geometry with no real contents
             */
            JSONGeometry(): type(JSONGeometryType::None) {}

            /**
             * A constructor for LineStrings
             * 
             * @param new_linestring A boost geometry LineString
             */
            JSONGeometry(const linestring_t &new_linestring)
            {
                type = JSONGeometryType::LineString;
                linestring = new_linestring;
            }

            /**
             * A constructor for Points
             * 
             * @param point A boost geometry point
             */
            JSONGeometry(const coordinate_t &point)
                : type(JSONGeometryType::Point),
                point(point)
            {}

            /**
             * A constructor for polygons
             * 
             * @param A boost geometry polygon
             */
            JSONGeometry(const polygon_t &polygon)
                : type(JSONGeometryType::Polygon),
                polygon(polygon)
            {}

            /**
             * A constructor for MultiPoints
             * 
             * @param A boost geometry MultiPoint
             */
            JSONGeometry(const bg::model::multi_point<coordinate_t> &multipoint)
                : type(JSONGeometryType::MultiPoint),
                multi_point(multipoint)
            {}

            /**
             * A constructor for MultiLineStrings
             * 
             * @param a boost geometry MultiLineString
             */
            JSONGeometry(const bg::model::multi_linestring<linestring_t> &multilinestring)
                : type(JSONGeometryType::MultiLineString),
                multi_linestring(multilinestring)
            {}

            JSONGeometry(const bg::model::multi_polygon<polygon_t> &multipolygon)
                : type(JSONGeometryType::MultiPolygon),
                multi_polygon(multipolygon)
            {}

            JSONGeometry(const JSONGeometry &json_geometry);

            static JSONGeometry of_point(const double x, const double y);

            static JSONGeometry of_point(pt::ptree &tree);

            static JSONGeometry of_linestring(two_dimensional_coordinates &coordinates);

            static JSONGeometry of_linestring(pt::ptree &tree);

            static JSONGeometry of_polygon(three_dimensional_coordinates &coordinates);

            /**
             * Generate a polyginal geometry from the details of coordinates passed in shaped like:
             * 
             * "coordinates": [
             *          [
             *              [100.0, 0.0],
             *              [101.0, 0.0],
             *              [101.0, 1.0],
             *              [100.0, 1.0],
             *              [100.0, 0.0]
             *          ],
             *          [
             *              [90.0, 0.2],
             *              [100.0, 0.2],
             *              [100.0, 0.8],
             *              [90.0, 0.8],
             *              [90.0, 0.2]
             *          ]
             *      ]
             * 
             * @param tree: A ptree containing the json data describing the polygon.
             *              Must have a 3D array keyed to "coordinates"
             * @return A JSONGeometry representing the polygon
             */
            static JSONGeometry of_polygon(pt::ptree &tree);

            static JSONGeometry of_multipoint(two_dimensional_coordinates &coordinates);

            static JSONGeometry of_multipoint(pt::ptree &tree);

            static JSONGeometry of_multilinestring(three_dimensional_coordinates &coordinates);

            static JSONGeometry of_multilinestring(pt::ptree &tree);

            static JSONGeometry of_multipolygon(four_dimensional_coordinates &coordinates);

            static JSONGeometry of_multipolygon(pt::ptree &tree);

            static JSONGeometry from_ptree(pt::ptree &tree);

            virtual ~JSONGeometry(){};

            JSONGeometryType get_type() const;;

            linestring_t as_linestring() const;

            coordinate_t as_point() const;

            polygon_t as_polygon() const;

            bg::model::multi_point<coordinate_t> as_multipoint() const;

            bg::model::multi_linestring<linestring_t> as_multilinestring() const;

            bg::model::multi_polygon<polygon_t> as_multipolygon() const;
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