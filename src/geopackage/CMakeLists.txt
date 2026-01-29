add_library(geopackage proj.cpp
                       geometry.cpp
                       properties.cpp
                       feature.cpp
                       read.cpp
                       wkb.cpp
                       ngen_sqlite.cpp
)
add_library(NGen::geopackage ALIAS geopackage)
target_include_directories(geopackage PUBLIC ${PROJECT_SOURCE_DIR}/include/geopackage)
target_include_directories(geopackage PUBLIC ${PROJECT_SOURCE_DIR}/include/utilities)
target_link_libraries(geopackage PUBLIC NGen::geojson Boost::boost SQLite::SQLite3)
