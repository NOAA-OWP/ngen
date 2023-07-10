cmake_minimum_required(VERSION 3.10)

include(${PROJECT_SOURCE_DIR}/cmake/dynamic_sourced_library.cmake)
dynamic_sourced_cxx_library(core_catchment_giuh "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(NGen::core_catchment_giuh ALIAS core_catchment_giuh)

target_include_directories(core_catchment_giuh PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include/utilities
        ${PROJECT_SOURCE_DIR}/include/core
        ${PROJECT_SOURCE_DIR}/include/core/catchment
        ${PROJECT_SOURCE_DIR}/include/core/catchment/giuh
        )

target_link_libraries(core_catchment_giuh PUBLIC
        Boost::boost                # Headers-only Boost
        )
