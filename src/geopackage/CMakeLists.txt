cmake_minimum_required(VERSION 3.10)


string(COMPARE EQUAL "${CMAKE_CXX_COMPILER_ID}" "IntelLLVM" _cmp)
if (NOT _cmp)
    message(WARNING "[NGen::geopackage] geometry.cpp cannot be optimized with "
                    "${CMAKE_CXX_COMPILER_ID} due to a suspected compiler/boost issue. "
                    "Use IntelLLVM if optimization for this source file is required.")
    # !! Required due to optimizer issue with either clang or
    # !! boost::geometry::srs::transformation
    set_source_files_properties(geometry.cpp PROPERTIES COMPILE_FLAGS -O0)
endif()

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
target_link_libraries(geopackage PUBLIC NGen::geojson Boost::boost sqlite3)
