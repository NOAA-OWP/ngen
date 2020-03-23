cmake_minimum_required(VERSION 3.10)

add_library(core STATIC
        GM_Object.cpp
        HY_HydroFeature.cpp
        )

add_library(NGen::core ALIAS core)

target_include_directories(core PUBLIC
        ../../include/core
        ../../include/core/catchment
        )

add_subdirectory("catchment")
add_subdirectory("nexus")