include(${PROJECT_SOURCE_DIR}/cmake/dynamic_sourced_library.cmake)
dynamic_sourced_cxx_library(core "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(NGen::core ALIAS core)
target_link_libraries(core PUBLIC
                           NGen::config_header          # Headers-only Boost
                           Boost::boost                # Headers-only Boost
                           )

target_link_libraries(core PRIVATE
                           NGen::parallel
                           )

target_include_directories(core PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include/simulation_time
        ${PROJECT_SOURCE_DIR}/include/core
        ${PROJECT_SOURCE_DIR}/include/core/catchment
        ${PROJECT_SOURCE_DIR}/include/core/nexus
        ${PROJECT_SOURCE_DIR}/include/core/hydrolocation
        ${PROJECT_SOURCE_DIR}/include/realizations/catchment
        ${PROJECT_SOURCE_DIR}/include/geojson
        ${PROJECT_SOURCE_DIR}/include/forcing
        ${PROJECT_SOURCE_DIR}/include/utilities
        ${PROJECT_SOURCE_DIR}/include/realizations/catchment
        ${PROJECT_SOURCE_DIR}/include/bmi
        )

# Layer.cpp routes through the abstract CatchmentOutputsMgr interface (header-only), so core needs
# the output headers on its include path -- but must NOT link ngen_output. Linking it would pull the
# concrete managers (and their NetCDF/MPI dependencies) into every core consumer, including the
# routing tests. PRIVATE keeps this to core's own translation units; Layer.hpp only forward-declares
# the type, and consumers that use the concrete backends link NGen::output directly.
target_include_directories(core PRIVATE ${PROJECT_SOURCE_DIR}/include/utilities/output)

if (NGEN_WITH_PYTHON)
    target_link_libraries(core PUBLIC NGen::python)
endif ()

if (NGEN_WITH_ROUTING)
    target_link_libraries(core PUBLIC NGen::routing)
endif ()

if(NGEN_WITH_MPI)
    target_link_libraries(core PUBLIC MPI::MPI_CXX)
endif()

add_subdirectory("catchment")
add_subdirectory("nexus")
add_subdirectory("hydrolocation")
