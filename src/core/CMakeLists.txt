cmake_minimum_required(VERSION 3.10)

add_library(NGen::core STATIC
        GM_Object.cpp,
        HY_HydroFeature.cpp
        )

target_include_directories(NGen::core PUBLIC
        ../../include/core
        ../../include/core/catchment
        )

add_subdirectory("catchment")
add_subdirectory("nexus")