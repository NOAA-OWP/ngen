add_library(geojson STATIC
        JSONGeometry.cpp
        JSONProperty.cpp
        FeatureCollection.cpp
        )
add_library(NGen::geojson ALIAS geojson)
target_include_directories(geojson PUBLIC
        ${PROJECT_SOURCE_DIR}/include/geojson
        ${PROJECT_SOURCE_DIR}/include/utilities
        )
target_link_libraries(geojson PUBLIC
        Boost::boost                # Headers-only Boost
        NGen::logging
        )
