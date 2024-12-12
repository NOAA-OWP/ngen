add_library(forcing)
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
        NGen::config_header
        Threads::Threads
)

target_sources(forcing PRIVATE "${CMAKE_CURRENT_LIST_DIR}/NullForcingProvider.cpp")

if(NGEN_WITH_NETCDF)
    target_sources(forcing PRIVATE "${CMAKE_CURRENT_LIST_DIR}/NetCDFPerFeatureDataProvider.cpp")
    target_link_libraries(forcing PUBLIC NetCDF)
endif()

if(NGEN_WITH_PYTHON)
    target_sources(forcing
      PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/ForcingsEngineDataProvider.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/ForcingsEngineLumpedDataProvider.cpp"
    )
    target_link_libraries(forcing PUBLIC pybind11::embed NGen::ngen_bmi)
endif()

#target_compile_options(forcing PUBLIC -std=c++14 -Wall)
