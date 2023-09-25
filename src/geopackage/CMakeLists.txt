string(COMPARE EQUAL "${CMAKE_CXX_COMPILER_ID}" "IntelLLVM" _cmp)
if (NOT _cmp)
    message(WARNING "[NGen::geopackage] wkb.cpp cannot be optimized with "
                    "${CMAKE_CXX_COMPILER_ID} due to a suspected optimizer/boost issue:\n"
                    "https://github.com/NOAA-OWP/ngen/issues/567.\n"
                    "Use IntelLLVM if optimization for this source file is required.")
    # !! Required due to optimizer issue with either clang or
    # !! boost::geometry::srs::transformation
    set_source_files_properties(wkb.cpp PROPERTIES COMPILE_OPTIONS "-O0;-fno-sanitize=all")
endif()

add_library(geopackage geometry.cpp
                       properties.cpp
                       feature.cpp
                       read.cpp
                       wkb.cpp
                       sqlite/iterator.cpp
                       sqlite/database.cpp
)
add_library(NGen::geopackage ALIAS geopackage)
target_include_directories(geopackage PUBLIC ${PROJECT_SOURCE_DIR}/include/geopackage)
target_include_directories(geopackage PUBLIC ${PROJECT_SOURCE_DIR}/include/utilities)
target_link_libraries(geopackage PUBLIC NGen::geojson Boost::boost sqlite3)
