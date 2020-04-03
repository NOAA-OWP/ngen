#include "JSONGeometry.hpp"

geojson::JSONGeometry::JSONGeometry(const geojson::JSONGeometry &json_geometry) {
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
        case JSONGeometryType::None:
            break;
    }

    type = json_geometry.get_type();
}

geojson::JSONGeometry geojson::JSONGeometry::of_point(const double x, const double y) {
    coordinate_t point(x, y);
    return JSONGeometry(point);
}

geojson::JSONGeometry geojson::JSONGeometry::of_point(pt::ptree &tree) {
    std::vector<double> point;

    for(auto &shape : tree.get_child("coordinates")) {
        point.push_back(std::stod(shape.second.data()));
    }

    return of_point(point[0], point[1]);
}

geojson::JSONGeometry geojson::JSONGeometry::of_linestring(geojson::two_dimensional_coordinates &coordinates) {
    linestring_t line;
    for (auto &pair : coordinates) {
        bg::append(line, coordinate_t(pair[0], pair[1]));
    }
    return JSONGeometry(line);
}

geojson::JSONGeometry geojson::JSONGeometry::of_linestring(pt::ptree &tree) {
    two_dimensional_coordinates coordinates;

    for (auto &point : tree.get_child("coordinates")) {
        std::vector<double> points;

        for (auto &coordinate : point.second) {
            points.push_back(std::stod(coordinate.second.data()));
        }

        coordinates.push_back(points);
    }

    return of_linestring(coordinates);
}

geojson::JSONGeometry geojson::JSONGeometry::of_polygon(geojson::three_dimensional_coordinates &coordinates) {
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

    return JSONGeometry(polygon);
}

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
geojson::JSONGeometry geojson::JSONGeometry::of_polygon(pt::ptree &tree) {
    three_dimensional_coordinates coordinates;

    /*
        Imagine that the ptree stores:

        "coordinates": [
            [
                [100.0, 0.0],
                [101.0, 0.0],
                [101.0, 1.0],
                [100.0, 1.0],
                [100.0, 0.0]
            ],
            [
                [90.0, 0.2],
                [100.0, 0.2],
                [100.0, 0.8],
                [90.0, 0.8],
                [90.0, 0.2]
            ]
        ]

        The first loop, generating `shape` will loop over the collection of coordinates,
        with each coordinate itself being a collection, so the first instance will be:

        [
            [100.0, 0.0],
            [101.0, 0.0],
            [101.0, 1.0],
            [100.0, 1.0],
            [100.0, 0.0]
        ]

        and the second:

        [
            [90.0, 0.2],
            [100.0, 0.2],
            [100.0, 0.8],
            [90.0, 0.8],
            [90.0, 0.2]
        ]
    */
    for(auto &shape : tree.get_child("coordinates")) {
        two_dimensional_coordinates shape_coordinates;

        // shape.first is the key; since this is an array, it probably won't have one
        // shape.second is an inner ptree

        /*
        Now we iterate through the inner collection. For the first shape, that means:

        [100.0, 0.0],
        [101.0, 0.0],
        [101.0, 1.0],
        [100.0, 1.0],
        [100.0, 0.0]
        */
        for (auto &group : shape.second) {
            std::vector<double> latLon;

            /**
             * The inner most set of values will be the individual coordinates, represented as arrays
             *
             * As before, group.first is the key, which is probably none, whereas group.second is an
             * inner ptree representing the inner array
             *
             * The first set of values in our example will be: [100.0, 0.0]
             */
            for (auto &coordinate : group.second) {
                latLon.push_back(std::stod(coordinate.second.data()));
            }

            // Store the coordinates
            shape_coordinates.push_back(latLon);
        }

        // Once all coordinates for the shape have been added to the shape vector, store that
        // in the overall set of coordinates
        coordinates.push_back(shape_coordinates);
    }

    /*
        Send the vector of vector of coordinate pairs to the function that will convert
        that into a polyginal geometry
    */
    return of_polygon(coordinates);
}

geojson::JSONGeometry geojson::JSONGeometry::of_multipoint(geojson::two_dimensional_coordinates &coordinates) {
    bg::model::multi_point<coordinate_t> points;

    for (auto &point: coordinates) {
        bg::append(points, coordinate_t(point[0], point[1]));
    }

    return JSONGeometry(points);
}

geojson::JSONGeometry geojson::JSONGeometry::of_multipoint(pt::ptree &tree) {
    two_dimensional_coordinates points;

    for (auto &point : tree.get_child("coordinates")) {
        std::vector<double> coordinates;
        for (auto &coordinate : point.second) {
            coordinates.push_back(std::stod(coordinate.second.data()));
        }
        points.push_back(coordinates);
    }

    return of_multipoint(points);
}

geojson::JSONGeometry geojson::JSONGeometry::of_multilinestring(geojson::three_dimensional_coordinates &coordinates) {
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

geojson::JSONGeometry geojson::JSONGeometry::of_multilinestring(pt::ptree &tree) {
    three_dimensional_coordinates coordinates;

    for (auto &line : tree.get_child("coordinates")) {
        two_dimensional_coordinates line_coordinates;

        for(auto &point : line.second) {
            std::vector<double> latLon;
            bool x_is_set = false;

            for(auto &coordinate : point.second) {
                latLon.push_back(std::stod(coordinate.second.data()));
            }

            line_coordinates.push_back(latLon);
        }

        coordinates.push_back(line_coordinates);
    }

    /*
        Send the vector of vector of coordinate pairs to the function that will convert
        that into a multiple linestring geometry
    */
    return of_multilinestring(coordinates);
}

geojson::JSONGeometry geojson::JSONGeometry::of_multipolygon(geojson::four_dimensional_coordinates &coordinates) {
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

geojson::JSONGeometry geojson::JSONGeometry::of_multipolygon(pt::ptree &tree) {
    four_dimensional_coordinates coordinates_groups;

    /*
        Imagine that the ptree stores:

        "coordinates": [
            [
                [
                    [100.0, 0.0],
                    [101.0, 0.0],
                    [101.0, 1.0],
                    [100.0, 1.0],
                    [100.0, 0.0]
                ],
                [
                    [90.0, 0.2],
                    [100.0, 0.2],
                    [100.0, 0.8],
                    [90.0, 0.8],
                    [90.0, 0.2]
                ]
            ],
            [
                [
                    [200.0, 0.0],
                    [202.0, 0.0],
                    [202.0, 2.0],
                    [200.0, 2.0],
                    [200.0, 0.0]
                ],
                [
                    [180.0, 0.4],
                    [200.0, 0.4],
                    [200.0, 1.6],
                    [180.0, 1.6],
                    [180.0, 0.4]
                ]
            ]
        ]

        The first loop, generating `shape` will loop over the collection of coordinates,
        with each coordinate itself being a collection, so the first instance will be:

        [
            [
                [100.0, 0.0],
                [101.0, 0.0],
                [101.0, 1.0],
                [100.0, 1.0],
                [100.0, 0.0]
            ],
            [
                [90.0, 0.2],
                [100.0, 0.2],
                [100.0, 0.8],
                [90.0, 0.8],
                [90.0, 0.2]
            ]
        ],

        and the second:

        [
            [
                [200.0, 0.0],
                [202.0, 0.0],
                [202.0, 2.0],
                [200.0, 2.0],
                [200.0, 0.0]
            ],
            [
                [180.0, 0.4],
                [200.0, 0.4],
                [200.0, 1.6],
                [180.0, 1.6],
                [180.0, 0.4]
            ]
        ]
    */
    for (auto &polygon : tree.get_child("coordinates")) {
        three_dimensional_coordinates polygon_coordinates;

        /*
            [
                [100.0, 0.0],
                [101.0, 0.0],
                [101.0, 1.0],
                [100.0, 1.0],
                [100.0, 0.0]
            ],
            [
                [90.0, 0.2],
                [100.0, 0.2],
                [100.0, 0.8],
                [90.0, 0.8],
                [90.0, 0.2]
            ]
        */
        for (auto &group : polygon.second) {
            two_dimensional_coordinates coordinate_group;

            /*
                [100.0, 0.0],
                [101.0, 0.0],
                [101.0, 1.0],
                [100.0, 1.0],
                [100.0, 0.0]
            */
            for (auto &point : group.second) {
                std::vector<double> latLon;

                // [100.0, 0.0]
                for(auto &coordinate : point.second) {
                    latLon.push_back(std::stod(coordinate.second.data()));
                }

                coordinate_group.push_back(latLon);
            }

            polygon_coordinates.push_back(coordinate_group);
        }

        coordinates_groups.push_back(polygon_coordinates);
    }


    return of_multipolygon(coordinates_groups);
}

geojson::JSONGeometry geojson::JSONGeometry::from_ptree(pt::ptree &tree) {
    std::string type = tree.get<std::string>("type");

    if (type == "Point") {
        return of_point(tree);
    }
    else if (type == "LineString") {
        return of_linestring(tree);
    }
    else if (type == "Polygon") {
        return of_polygon(tree);
    }
    else if (type == "MultiPoint") {
        return of_multipoint(tree);
    }
    else if (type == "MultiLineString") {
        return of_multilinestring(tree);
    }
    else if (type == "MultiPolygon") {
        return of_multipolygon(tree);
    }

    std::string message = "A geometry type of " + type + " is not supported.";
    throw std::logic_error(message);
}

geojson::JSONGeometryType geojson::JSONGeometry::get_type() const {
    return type;
}

geojson::linestring_t geojson::JSONGeometry::as_linestring() const {
    return linestring;
}

geojson::coordinate_t geojson::JSONGeometry::as_point() const {
    return point;
}

geojson::polygon_t geojson::JSONGeometry::as_polygon() const {
    return polygon;
}

bg::model::multi_point<geojson::coordinate_t> geojson::JSONGeometry::as_multipoint() const {
    return multi_point;
}

bg::model::multi_linestring<geojson::linestring_t> geojson::JSONGeometry::as_multilinestring() const {
    return multi_linestring;
}

bg::model::multi_polygon<geojson::polygon_t> geojson::JSONGeometry::as_multipolygon() const {
    return multi_polygon;
}
