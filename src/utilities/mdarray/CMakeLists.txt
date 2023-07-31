add_library(mdarray INTERFACE)
add_library(NGen::mdarray ALIAS mdarray)
target_include_directories(mdarray INTERFACE ${PROJECT_SOURCE_DIR}/include/utilities)
