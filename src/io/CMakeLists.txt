add_library(io mdframe.cpp)
add_library(NGen::io ALIAS io)
target_include_directories(io PUBLIC ${PROJECT_SOURCE_DIR}/include/io)
