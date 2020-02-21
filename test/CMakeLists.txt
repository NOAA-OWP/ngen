project(test)

# TODO: once this file is finished, and there is a top-level CMakeLists.txt, add test/ as a
# TODO:     subdirectory to that file (i.e., a line with "add_subdirectory(test)")

add_subdirectory(googletest)
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

# TODO: add test source files (e.g., test1.cpp)
#add_executable(tests_run test1.cpp)

# TODO: add once executable is actually added, as per above
#target_link_libraries(tests_run gtest gtest_main)
