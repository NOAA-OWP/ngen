add_library(mdframe handler_csv.cpp handler_netcdf.cpp)
add_library(NGen::mdframe ALIAS mdframe)
target_include_directories(mdframe PUBLIC ${PROJECT_SOURCE_DIR}/include/utilities)
target_link_libraries(mdframe PRIVATE NGen::mdarray Boost::boost)
