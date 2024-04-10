include(${PROJECT_SOURCE_DIR}/cmake/dynamic_sourced_library.cmake)
dynamic_sourced_cxx_library(output "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(NGen::output ALIAS output)

target_include_directories(output PUBLIC
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
        ${PROJECT_SOURCE_DIR}/include/output
        )

target_link_libraries(output PUBLIC
        Boost::boost                # Headers-only Boost
)

if(NGEN_WITH_NETCDF)
    target_link_libraries(output PUBLIC NetCDF)
endif()

if(NGEN_WITH_MPI)
    target_link_libraries(output PUBLIC MPI::MPI_C MPI::MPI_CXX)
endif()
