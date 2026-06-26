#pragma once

#include <string>

struct GriddedDataSelector {
    std::string variable_name;
    time_t      init_time;
    long        duration;
    std::string output_units;
};
