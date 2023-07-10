cmake_minimum_required(VERSION 3.10)

include(${PROJECT_SOURCE_DIR}/cmake/dynamic_sourced_library.cmake)
dynamic_sourced_cxx_library(core_hydrolocation "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(NGen::core_hydrolocation ALIAS core_hydrolocation)

target_include_directories(core_hydrolocation PUBLIC
        ${PROJECT_SOURCE_DIR}/include/core
        ${PROJECT_SOURCE_DIR}/include/core/hydrolocation
        ${PROJECT_SOURCE_DIR}/include/core/catchment
        ${PROJECT_SOURCE_DIR}/include/forcing
        ${PROJECT_SOURCE_DIR}/include/utilities
        )

target_link_libraries(core_hydrolocation PUBLIC
        Boost::boost                # Headers-only Boost
        )

if(MPI_ACTIVE)
    add_compile_definitions(NGEN_MPI_ACTIVE)
endif()
