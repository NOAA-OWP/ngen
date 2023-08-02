cmake_minimum_required(VERSION 3.10)

include(${PROJECT_SOURCE_DIR}/cmake/dynamic_sourced_library.cmake)
dynamic_sourced_cxx_library(core_catchment "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(NGen::core_catchment ALIAS core_catchment)

target_include_directories(core_catchment PUBLIC
        ${PROJECT_SOURCE_DIR}/include/core
        ${PROJECT_SOURCE_DIR}/include/core/catchment
        ${PROJECT_SOURCE_DIR}/include/forcing
        ${PROJECT_SOURCE_DIR}/include/utilities
        )

target_link_libraries(core_catchment PUBLIC
        Boost::boost                # Headers-only Boost
        NGen::core
        )

# TODO: this may not be correct, need to see
#add_dependencies(core_catchment NGen::core)
