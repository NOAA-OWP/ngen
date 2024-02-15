include(${PROJECT_SOURCE_DIR}/cmake/dynamic_sourced_library.cmake)
dynamic_sourced_cxx_library(core "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(NGen::core ALIAS core)

target_include_directories(core PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include/simulation_time
        ${PROJECT_SOURCE_DIR}/include/core
        ${PROJECT_SOURCE_DIR}/include/core/catchment
        ${PROJECT_SOURCE_DIR}/include/core/nexus
        ${PROJECT_SOURCE_DIR}/include/core/hydrolocation
        ${PROJECT_SOURCE_DIR}/include/core/mediator
        ${PROJECT_SOURCE_DIR}/include/realizations/catchment
        ${PROJECT_SOURCE_DIR}/include/geojson
        ${PROJECT_SOURCE_DIR}/include/forcing
        ${PROJECT_SOURCE_DIR}/include/utilities
        ${PROJECT_SOURCE_DIR}/extern/pybind11/include
        ${PROJECT_SOURCE_DIR}/include/realizations/catchment
        ${PROJECT_SOURCE_DIR}/include/bmi
        )

if (NGEN_WITH_PYTHON)
    target_include_directories(core PUBLIC
            ${PROJECT_SOURCE_DIR}/include
            ${PROJECT_SOURCE_DIR}/include/simulation_time
            ${PROJECT_SOURCE_DIR}/include/core
            ${PROJECT_SOURCE_DIR}/include/core/catchment
            ${PROJECT_SOURCE_DIR}/include/core/nexus
            ${PROJECT_SOURCE_DIR}/include/core/hydrolocation
            ${PROJECT_SOURCE_DIR}/include/core/mediator
            ${PROJECT_SOURCE_DIR}/include/realizations/catchment
            ${PROJECT_SOURCE_DIR}/include/geojson
            ${PROJECT_SOURCE_DIR}/include/forcing
            ${PROJECT_SOURCE_DIR}/include/utilities
            ${PROJECT_SOURCE_DIR}/extern/pybind11/include
            )
    configure_file(
        ${PROJECT_SOURCE_DIR}/src/core/NGen_Python_Build_Versions.in
        ${CMAKE_CURRENT_BINARY_DIR}/src/core/NGen_Python_Build_Versions.cpp
    )
    target_sources(core PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/src/core/NGen_Python_Build_Versions.cpp)
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
            ${PROJECT_SOURCE_DIR}/include/core/nexus
            ${PROJECT_SOURCE_DIR}/include/core/hydrolocation
            ${PROJECT_SOURCE_DIR}/include/core/mediator
            ${PROJECT_SOURCE_DIR}/include/realizations/catchment
            ${PROJECT_SOURCE_DIR}/include/geojson
            ${PROJECT_SOURCE_DIR}/include/forcing
            ${PROJECT_SOURCE_DIR}/include/utilities
            )
    target_link_libraries(core PUBLIC
            Boost::boost                # Headers-only Boost
            )
endif ()

if(NGEN_WITH_MPI)
    target_link_libraries(core PUBLIC MPI::MPI_C MPI::MPI_CXX)
endif()

add_subdirectory("catchment")
add_subdirectory("nexus")
add_subdirectory("hydrolocation")
add_subdirectory("mediator")
