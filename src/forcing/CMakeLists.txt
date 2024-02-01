include(${PROJECT_SOURCE_DIR}/cmake/dynamic_sourced_library.cmake)
dynamic_sourced_cxx_library(forcing "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(NGen::forcing ALIAS forcing)

find_package(Threads REQUIRED)

target_include_directories(forcing PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include/core
        ${PROJECT_SOURCE_DIR}/include/core/mediator
        ${PROJECT_SOURCE_DIR}/include/utilities
        ${PROJECT_SOURCE_DIR}/include/forcing
        )

target_link_libraries(forcing PUBLIC
        NGen::config_header
        NGen::core
        Boost::boost                # Headers-only Boost
        Threads::Threads
)

if(NGEN_WITH_NETCDF)
    target_link_libraries(forcing PUBLIC NetCDF)
endif()

if(NGEN_WITH_PYTHON)
    target_link_libraries(forcing PUBLIC pybind11::embed NGen::ngen_bmi)
endif()

#target_compile_options(forcing PUBLIC -std=c++14 -Wall)
