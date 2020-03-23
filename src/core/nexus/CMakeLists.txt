cmake_minimum_required(VERSION 3.10)

add_library(NGen::core_nexus STATIC
        HY_HydroNexus.cpp,
        HY_PointHydroNexus.cpp
        )

target_include_directories(NGen:core_nexus PUBLIC
        ../../../include/core
        )

target_link_libraries(NGen::core_nexus PUBLIC
        Boost::boost                # Headers-only Boost
        )