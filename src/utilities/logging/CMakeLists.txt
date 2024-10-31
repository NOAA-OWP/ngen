add_library(logging logging_utils.cpp Logger.cpp)
add_library(NGen::logging ALIAS logging)
target_include_directories(logging PUBLIC ${PROJECT_SOURCE_DIR}/include/utilities)
