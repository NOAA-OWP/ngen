cmake_minimum_required(VERSION 3.10)
add_library(geopackage STATIC geometry.cpp
                              properties.cpp
                              feature.cpp
                              read.cpp
                              sqlite/iterator.cpp
                              sqlite/database.cpp
)
add_library(NGen::geopackage ALIAS geopackage)
target_include_directories(geopackage PUBLIC ${PROJECT_SOURCE_DIR}/include/geopackage)
target_include_directories(geopackage PRIVATE ${PROJECT_SOURCE_DIR}/include/utilities)
target_link_libraries(geopackage PUBLIC NGen::geojson Boost::boost libsqlite3)
