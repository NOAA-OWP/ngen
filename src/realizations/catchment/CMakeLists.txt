cmake_minimum_required(VERSION 3.10)

include(${PROJECT_SOURCE_DIR}/cmake/dynamic_sourced_library.cmake)
dynamic_sourced_cxx_library(realizations_catchment "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(NGen::realizations_catchment ALIAS realizations_catchment)

target_include_directories(realizations_catchment PUBLIC
        ${PROJECT_SOURCE_DIR}/include/core
        ${PROJECT_SOURCE_DIR}/include/core/catchment
        ${PROJECT_SOURCE_DIR}/include/realizations/catchment
        ${PROJECT_SOURCE_DIR}/include/forcing
        ${PROJECT_SOURCE_DIR}/include/simulation_time
        ${PROJECT_SOURCE_DIR}/include/utilities
        ${PROJECT_SOURCE_DIR}/include/geojson
        ${PROJECT_SOURCE_DIR}/models
        ${PROJECT_SOURCE_DIR}/models/kernels
        ${PROJECT_SOURCE_DIR}/models/tshirt/include
        )
        
target_link_libraries(realizations_catchment PUBLIC
        NGen::core_catchment
        NGen::core_catchment_giuh
        NGen::models_tshirt
        NGen::geojson
        )
