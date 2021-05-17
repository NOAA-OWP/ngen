cmake_minimum_required(VERSION 3.10)

include(${PROJECT_SOURCE_DIR}/cmake/dynamic_sourced_library.cmake)
dynamic_sourced_cxx_library(kernels_evapotranspiration "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(NGen::kernels_evapotranspiration ALIAS kernels_evapotranspiration)

target_include_directories(kernels_evapotranspiration PUBLIC
        ${PROJECT_SOURCE_DIR}/models/kernels/evapotranspiration
        )