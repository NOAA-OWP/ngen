#pragma once

#include <chrono>
#include <string>
#include <boost/variant.hpp>

struct AllPoints {};
static AllPoints all_points;

struct MeshPointsSelector
{
    std::string variable_name;
    std::chrono::time_point<std::chrono::system_clock> init_time;
    std::chrono::seconds duration;
    std::string output_units;
    boost::variant<AllPoints, std::vector<int>> points;
};
