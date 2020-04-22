cmake_minimum_required(VERSION 3.10)
add_library(geojson STATIC
        JSONGeometry.cpp
        JSONProperty.cpp
        FeatureCollection.cpp
        )
add_library(NGen::geojson ALIAS geojson)
target_include_directories(geojson PUBLIC
        ${PROJECT_SOURCE_DIR}/include/geojson
        )
# TODO: consider setting a minimum or required version
find_package(Boost)
target_link_libraries(geojson PUBLIC
        Boost::boost                # Headers-only Boost
        )
