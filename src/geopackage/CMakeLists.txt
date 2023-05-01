cmake_minimum_required(VERSION 3.10)
add_library(geopackage STATIC GeoPackage/geometry.cpp
                              GeoPackage/properties.cpp
                              GeoPackage/feature.cpp
                              GeoPackage/read.cpp
)
add_library(NGen::geopackage ALIAS geopackage)
target_include_directories(geopackage PUBLIC ${PROJECT_SOURCE_DIR}/include/geopackage)
target_link_libraries(geopackage PRIVATE NGen::geojson Boost::boost libsqlite3)
