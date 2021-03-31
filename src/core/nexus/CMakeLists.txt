cmake_minimum_required(VERSION 3.10)

include(${PROJECT_SOURCE_DIR}/cmake/dynamic_sourced_library.cmake)
dynamic_sourced_cxx_library(core_nexus "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(NGen::core_nexus ALIAS core_nexus)

target_include_directories(core_nexus PUBLIC
        ${PROJECT_SOURCE_DIR}/include/core
        ${PROJECT_SOURCE_DIR}/include/core/nexus
        )

# TODO: consider setting a minimum or required version
find_package(Boost)

target_link_libraries(core_nexus PUBLIC
        Boost::boost                # Headers-only Boost
        )

if(MPI_ACTIVE)
    add_compile_definitions(NGEN_MPI_ACTIVE)
endif()

