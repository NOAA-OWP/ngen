add_library(io INTERFACE)
add_library(NGen::io ALIAS io)
target_include_directories(io INTERFACE ${PROJECT_SOURCE_DIR}/include/io)
