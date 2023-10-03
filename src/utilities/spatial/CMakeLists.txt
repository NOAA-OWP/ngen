add_library(spatial feature.cpp)
add_library(NGen::spatial ALIAS spatial)
target_include_directories(spatial PUBLIC ${PROJECT_SOURCE_DIR}/include/utilities)
