add_library(io mdframe.cpp
               mdframe_handler_netcdf.cpp
               mdframe_handler_csv.cpp)
add_library(NGen::io ALIAS io)
target_include_directories(io PUBLIC ${PROJECT_SOURCE_DIR}/include/io)
