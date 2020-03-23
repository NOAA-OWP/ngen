cmake_minimum_required(VERSION 3.10)

add_library(core_nexus STATIC
        HY_HydroNexus.cpp
        HY_PointHydroNexus.cpp
        )

add_library(NGen::core_nexus ALIAS core_nexus)

target_include_directories(core_nexus PUBLIC
        ../../../include/core
        )

# TODO: consider setting a minimum or required version
find_package(Boost)

target_link_libraries(core_nexus PUBLIC
        Boost::boost                # Headers-only Boost
        )