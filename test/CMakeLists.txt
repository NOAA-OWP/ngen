project(test)

add_subdirectory(googletest)
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
include_directories(${PROJ_ROOT_INCLUDE_DIR})

if(MPI_ACTIVE)
    add_compile_definitions(NGEN_MPI_TESTS_ACTIVE)
endif()

if(BMI_C_LIB_ACTIVE)
    add_compile_definitions(NGEN_BMI_C_LIB_TESTS_ACTIVE)
endif()

if(LSTM_TORCH_LIB_ACTIVE)
    add_compile_definitions(LSTM_TORCH_LIB_TESTS_ACTIVE)
endif()

if(NGEN_ACTIVATE_ROUTING)
    add_compile_definitions(ROUTING_PYBIND_TESTS_ACTIVE)
endif()

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
            NGen::core_catchment
            NGen::core_catchment_giuh
            NGen::core_nexus
            NGen::geojson
            NGen::models_tshirt
            NGen::kernels_reservoir
            NGen::realizations_catchment
            )
    gtest_discover_tests(${TESTNAME}
            WORKING_DIRECTORY ${PROJECT_DIR}
            PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_DIR}"
            )
    set_target_properties(${TESTNAME} PROPERTIES FOLDER test)
endmacro()

macro(add_automated_test_w_routing TESTNAME)
    add_executable(${TESTNAME} ${ARGN})
    # This should include the same as the main ngen executable
    target_link_libraries(${TESTNAME}
            gtest
            gtest_main
            NGen::core
            NGen::core_catchment
            NGen::core_catchment_giuh
            NGen::core_nexus
            NGen::geojson
            NGen::models_tshirt
            NGen::kernels_reservoir
            NGen::realizations_catchment
            NGen::routing
            )
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
            NGen::core_catchment
            NGen::core_catchment_giuh
            NGen::core_nexus
            NGen::geojson
            NGen::models_tshirt
            NGen::realizations_catchment
            NGen::kernels_reservoir
            )
    gtest_discover_tests(${TESTNAME}
            WORKING_DIRECTORY ${PROJECT_DIR}
            PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_DIR}"
            )
    set_target_properties(${TESTNAME} PROPERTIES FOLDER test)
endmacro()

# Individual tests
add_automated_test(test_hymod models/hymod/include/HymodTest.cpp)

add_automated_test(
        test_tshirt
        models/tshirt/include/TshirtTest.cpp
        realizations/catchments/Tshirt_C_Realization_Test.cpp
)

add_automated_test(
    test_geojson
    geojson/JSONProperty_Test.cpp
    geojson/JSONGeometry_Test.cpp
    geojson/Feature_Test.cpp
    geojson/FeatureCollection_Test.cpp
)

add_automated_test(
    test_realization_config
    realizations/Formulation_Manager_Test.cpp
)

add_automated_test(
        test_giuh
        core/catchment/giuh/GIUH_Test.cpp
)

add_automated_test(
  test_nexus
  core/nexus/NexusTests.cpp
)

if(MPI_ACTIVE)
   add_automated_test(
           test_remote_nexus
           core/nexus/NexusRemoteTests.cpp
   )
endif()


add_automated_test(
        test_bmi_c
        realizations/catchments/Bmi_C_Adapter_Test.cpp
        realizations/catchments/Bmi_C_Formulation_Test.cpp
)

add_automated_test(
        compare_cfe
        # TODO: this probably needs to be added to integration testing also
        realizations/catchments/Bmi_C_Cfe_IT.cpp
)

if(LSTM_TORCH_LIB_ACTIVE)
   add_automated_test(
           test_lstm
           models/lstm/include/LSTM_Test.cpp
           realizations/catchments/LSTM_Realization_Test.cpp
   )
endif()

if(NGEN_ACTIVATE_ROUTING)
   add_automated_test_w_routing(
           test_routing_pybind
           routing/Routing_Py_Bind_Test.cpp
   )
endif()

add_automated_test(
        test_network
        core/NetworkTests.cpp
)

add_automated_test(
        test_partition
        utils/Partition_Test.cpp
)

# Unit tests
# TODO: Make sure the geojson tests don't need to be added here also
add_automated_test(
        test_unit
        models/hymod/include/HymodTest.cpp
        models/hymod/include/Reservoir_Test.cpp
        models/hymod/include/Reservoir_Timeless_Test.cpp
        models/tshirt/include/TshirtTest.cpp
        realizations/catchments/Tshirt_C_Realization_Test.cpp
        geojson/JSONProperty_Test.cpp
        geojson/JSONGeometry_Test.cpp
        geojson/Feature_Test.cpp
        geojson/FeatureCollection_Test.cpp
        forcing/Forcing_Test.cpp
        simulation_time/Simulation_Time_Test.cpp
        core/catchment/giuh/GIUH_Test.cpp
        core/NetworkTests.cpp
        utils/include/StreamOutputTest.cpp
        realizations/Formulation_Manager_Test.cpp
)

# Add for integration tests once we have some
#add_automated_test(test_it models/hymod/include/HymodTest.cpp)

# All automated tests
add_automated_test_w_mock(
        test_all
        models/hymod/include/HymodTest.cpp
        models/hymod/include/Reservoir_Test.cpp
        models/hymod/include/Reservoir_Timeless_Test.cpp
        models/tshirt/include/TshirtTest.cpp
        realizations/catchments/Tshirt_C_Realization_Test.cpp
        geojson/JSONProperty_Test.cpp
        geojson/JSONGeometry_Test.cpp
        geojson/Feature_Test.cpp
        geojson/FeatureCollection_Test.cpp
        forcing/Forcing_Test.cpp
        simulation_time/Simulation_Time_Test.cpp
        core/catchment/giuh/GIUH_Test.cpp
        core/NetworkTests.cpp
        realizations/Formulation_Manager_Test.cpp
)
