project(test)

add_subdirectory(googletest)
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

# Add test source files (e.g., test1.cpp)
#add_executable(test_hymod models/hymod/include/HymodTest.cpp)
#target_link_libraries(test_hymod gtest gtest_main)

macro(add_automated_test TESTNAME)
    add_executable(${TESTNAME} ${ARGN})
    # This should include the same as the main ngen executable
    target_link_libraries(${TESTNAME}
            gtest
            gtest_main
            NGen::core
            NGen::realizations_catchment)
    gtest_discover_tests(${TESTNAME}
            WORKING_DIRECTORY ${PROJECT_DIR}
            PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_DIR}"
            )
    set_target_properties(${TESTNAME} PROPERTIES FOLDER test)
endmacro()

macro(add_automated_test_w_mock TESTNAME)
    add_executable(${TESTNAME} ${ARGN})
    # This should include the same as the main ngen executable
    target_link_libraries(${TESTNAME}
            gtest
            gmock
            gtest_main
            NGen::core
            NGen::realizations_catchment
            )
    gtest_discover_tests(${TESTNAME}
            WORKING_DIRECTORY ${PROJECT_DIR}
            PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_DIR}"
            )
    set_target_properties(${TESTNAME} PROPERTIES FOLDER test)
endmacro()

# Individual tests
add_automated_test(test_hymod models/hymod/include/HymodTest.cpp)

# Unit tests
add_automated_test(test_unit models/hymod/include/HymodTest.cpp models/hymod/include/Nonlinear_Reservoir_Test.cpp)

# Add for integration tests once we have some
#add_automated_test(test_it models/hymod/include/HymodTest.cpp)

# All automated tests
add_automated_test_w_mock(test_all models/hymod/include/HymodTest.cpp models/hymod/include/Nonlinear_Reservoir_Test.cpp)
