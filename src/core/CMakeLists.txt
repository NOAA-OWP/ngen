cmake_minimum_required(VERSION 3.10)

include(${PROJECT_SOURCE_DIR}/cmake/dynamic_sourced_library.cmake)
dynamic_sourced_cxx_library(core "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(NGen::core ALIAS core)

target_include_directories(core PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include/simulation_time
        ${PROJECT_SOURCE_DIR}/include/core
        ${PROJECT_SOURCE_DIR}/include/core/catchment
        ${PROJECT_SOURCE_DIR}/include/core/catchment/giuh
        ${PROJECT_SOURCE_DIR}/include/core/nexus
        ${PROJECT_SOURCE_DIR}/include/core/hydrolocation
        ${PROJECT_SOURCE_DIR}/include/core/mediator
        ${PROJECT_SOURCE_DIR}/include/realizations/catchment
        ${PROJECT_SOURCE_DIR}/models/kernels
        ${PROJECT_SOURCE_DIR}/models/kernels/evapotranspiration
        ${PROJECT_SOURCE_DIR}/models/
        ${PROJECT_SOURCE_DIR}/models/tshirt/include
        ${PROJECT_SOURCE_DIR}/include/geojson
        ${PROJECT_SOURCE_DIR}/include/forcing
        ${PROJECT_SOURCE_DIR}/include/utilities
        ${PROJECT_SOURCE_DIR}/extern/pybind11/include
        )

if (${NGEN_ACTIVATE_PYTHON})
    target_include_directories(core PUBLIC
            ${PROJECT_SOURCE_DIR}/include
            ${PROJECT_SOURCE_DIR}/include/simulation_time
            ${PROJECT_SOURCE_DIR}/include/core
            ${PROJECT_SOURCE_DIR}/include/core/catchment
            ${PROJECT_SOURCE_DIR}/include/core/catchment/giuh
            ${PROJECT_SOURCE_DIR}/include/core/nexus
            ${PROJECT_SOURCE_DIR}/include/core/hydrolocation
            ${PROJECT_SOURCE_DIR}/include/core/mediator
            ${PROJECT_SOURCE_DIR}/include/realizations/catchment
            ${PROJECT_SOURCE_DIR}/models/kernels
            ${PROJECT_SOURCE_DIR}/models/kernels/evapotranspiration
            ${PROJECT_SOURCE_DIR}/models/
            ${PROJECT_SOURCE_DIR}/models/tshirt/include
            ${PROJECT_SOURCE_DIR}/include/geojson
            ${PROJECT_SOURCE_DIR}/include/forcing
            ${PROJECT_SOURCE_DIR}/include/utilities
            ${PROJECT_SOURCE_DIR}/extern/pybind11/include
            )
    target_link_libraries(core PUBLIC
            Boost::boost                # Headers-only Boost
            pybind11::embed
            )
else ()
    target_include_directories(core PUBLIC
            ${PROJECT_SOURCE_DIR}/include
            ${PROJECT_SOURCE_DIR}/include/simulation_time
            ${PROJECT_SOURCE_DIR}/include/core
            ${PROJECT_SOURCE_DIR}/include/core/catchment
            ${PROJECT_SOURCE_DIR}/include/core/catchment/giuh
            ${PROJECT_SOURCE_DIR}/include/core/nexus
            ${PROJECT_SOURCE_DIR}/include/core/hydrolocation
            ${PROJECT_SOURCE_DIR}/include/core/mediator
            ${PROJECT_SOURCE_DIR}/include/realizations/catchment
            ${PROJECT_SOURCE_DIR}/models/kernels
            ${PROJECT_SOURCE_DIR}/models/kernels/evapotranspiration
            ${PROJECT_SOURCE_DIR}/models/
            ${PROJECT_SOURCE_DIR}/models/tshirt/include
            ${PROJECT_SOURCE_DIR}/include/geojson
            ${PROJECT_SOURCE_DIR}/include/forcing
            ${PROJECT_SOURCE_DIR}/include/utilities
            )
    target_link_libraries(core PUBLIC
            Boost::boost                # Headers-only Boost
            )
endif ()

add_subdirectory("catchment")
add_subdirectory("nexus")
add_subdirectory("hydrolocation")
add_subdirectory("mediator")
