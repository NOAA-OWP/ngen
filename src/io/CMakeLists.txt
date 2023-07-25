set(io_sources mdframe.cpp mdframe_handler_csv.cpp)

if(NETCDF_ACTIVE)
    list(APPEND io_sources mdframe_handler_netcdf.cpp)
endif()

add_library(io ${io_sources})
add_library(NGen::io ALIAS io)
target_include_directories(io PUBLIC ${PROJECT_SOURCE_DIR}/include/io)
target_include_directories(io PUBLIC ${PROJECT_SOURCE_DIR}/include/utilities)
target_link_libraries(io Boost::boost)

if(NETCDF_ACTIVE)
    target_link_libraries(io ${NETCDF_LIBRARIES})
endif()
