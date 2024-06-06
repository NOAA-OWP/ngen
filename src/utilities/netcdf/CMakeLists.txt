# library to contain all source files in this projects
include(${PROJECT_SOURCE_DIR}/cmake/dynamic_sourced_library.cmake)
dynamic_sourced_cxx_library(netcdf_utils "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(NGen::netcdf_utils ALIAS netcdf_utils)

# set include paths
target_include_directories(netcdf_utils PUBLIC ${PROJECT_SOURCE_DIR}/include/)

target_include_directories(netcdf_utils PUBLIC ${PROJECT_SOURCE_DIR}/include/bmi)

target_include_directories(netcdf_utils PUBLIC ${PROJECT_SOURCE_DIR}/include/core)
target_include_directories(netcdf_utils PUBLIC ${PROJECT_SOURCE_DIR}/include/core/catchment)
target_include_directories(netcdf_utils PUBLIC ${PROJECT_SOURCE_DIR}/include/core/hydrolocation)
target_include_directories(netcdf_utils PUBLIC ${PROJECT_SOURCE_DIR}/include/core/mediator)
target_include_directories(netcdf_utils PUBLIC ${PROJECT_SOURCE_DIR}/include/core/nexus)

target_include_directories(netcdf_utils PUBLIC ${PROJECT_SOURCE_DIR}/include/forcing)

target_include_directories(netcdf_utils PUBLIC ${PROJECT_SOURCE_DIR}/include/geojson)
target_include_directories(netcdf_utils PUBLIC ${PROJECT_SOURCE_DIR}/include/output)

target_include_directories(netcdf_utils PUBLIC ${PROJECT_SOURCE_DIR}/include/realizations/catchment)

target_include_directories(netcdf_utils PUBLIC ${PROJECT_SOURCE_DIR}/include/simulation_time)

target_include_directories(netcdf_utils PUBLIC ${PROJECT_SOURCE_DIR}/include/utilities)
target_include_directories(netcdf_utils PUBLIC ${PROJECT_SOURCE_DIR}/include/utilities/netcdf)


target_include_directories(netcdf_utils PUBLIC ${Boost_INCLUDE_DIRS}) 