project(test)

add_subdirectory(googletest)
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

# Add test source files (e.g., test1.cpp)
add_executable(test_hymod models/hymod/include/HymodTest.cpp)

target_link_libraries(test_hymod gtest gtest_main)
