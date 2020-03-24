#ifndef GEOJSON_COORDINATES_HPP
#define GEOJSON_COORDINATES_HPP

#include <vector>

namespace geojson {
    class Coordinates {
        private:
            std::vector<double> single_dimension;
            std::vector<std::vector<double>> two_dimensional;
            std::vector<std::vector<std::vector<double>>> three_dimensional;
    }
}

#endif