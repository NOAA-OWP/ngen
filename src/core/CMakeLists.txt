cmake_minimum_required(VERSION 3.10)

include(${PROJECT_SOURCE_DIR}/cmake/dynamic_sourced_library.cmake)
dynamic_sourced_cxx_library(core "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(NGen::core ALIAS core)

target_include_directories(core PUBLIC
        ${PROJECT_SOURCE_DIR}/include/core
        ${PROJECT_SOURCE_DIR}/include/core/catchment
        ${PROJECT_SOURCE_DIR}/include/geojson
        ${PROJECT_SOURCE_DIR}/include/forcing
        ${PROJECT_SOURCE_DIR}/include/utilities
        )

target_link_libraries(core PUBLIC
        Boost::boost                # Headers-only Boost
        )

add_subdirectory("catchment")
add_subdirectory("nexus")
add_subdirectory("hydrolocation")
