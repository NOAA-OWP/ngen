add_subdirectory(googletest)
add_compile_definitions(NGEN_BMI_CPP_LIB_TESTS_ACTIVE)

# =============================================================================

# If ${NGEN_ROOT_DIR} and ${PROJECT_BINARY_DIR} are the same, then we have an in-source build
# and this is not necessary.
if(NOT NGEN_ROOT_DIR STREQUAL PROJECT_BINARY_DIR)
    # Create a symlink between the root data directory in the source tree and the build tree
    file(CREATE_LINK "${NGEN_ROOT_DIR}/data" "${PROJECT_BINARY_DIR}/data" SYMBOLIC)

    # Create a symlink between the test data directory in the source tree and the build tree
    file(CREATE_LINK "${CMAKE_CURRENT_LIST_DIR}/data" "${PROJECT_BINARY_DIR}/test/data" SYMBOLIC)

    # Create a symlink between the extern directory in the source tree and the build tree
    file(CREATE_LINK "${NGEN_ROOT_DIR}/extern" "${PROJECT_BINARY_DIR}/extern" SYMBOLIC)
    file(CREATE_LINK "${NGEN_ROOT_DIR}/extern" "${PROJECT_BINARY_DIR}/test/extern" SYMBOLIC)
endif()

# =============================================================================

git_update_submodule("${NGEN_EXT_DIR}/bmi-cxx")

add_external_subdirectory(
    SOURCE "${NGEN_EXT_DIR}/test_bmi_cpp"
    OUTPUT "${NGEN_EXT_DIR}/test_bmi_cpp/cmake_build"
    IMPORTS testbmicppmodel
)

if (NGEN_WITH_PYTHON)
    add_compile_definitions(NGEN_BMI_PY_TESTS_ACTIVE)
endif()

if(NGEN_WITH_MPI)
    add_compile_definitions(NGEN_MPI_TESTS_ACTIVE)
endif()

if(NGEN_WITH_BMI_C)
    add_compile_definitions(NGEN_BMI_C_LIB_TESTS_ACTIVE)
    add_external_subdirectory(
        SOURCE "${NGEN_EXT_DIR}/test_bmi_c"
        OUTPUT "${NGEN_EXT_DIR}/test_bmi_c/cmake_build"
        IMPORTS testbmicmodel
    )
endif()

if(NGEN_WITH_BMI_FORTRAN)
    add_compile_definitions(NGEN_BMI_FORTRAN_LIB_TESTS_ACTIVE)
    add_external_subdirectory(
        SOURCE "${NGEN_EXT_DIR}/test_bmi_fortran"
        OUTPUT "${NGEN_EXT_DIR}/test_bmi_fortran/cmake_build"
        IMPORTS testbmifortranmodel
    )
endif()

if(NGEN_WITH_ROUTING)
    add_compile_definitions(ROUTING_PYBIND_TESTS_ACTIVE)
endif()

if(NGEN_WITH_NETCDF)
    add_compile_definitions(NETCDF_ACTIVE)
endif()

# =============================================================================

add_executable(test_all)
target_link_libraries(test_all PRIVATE gtest gtest_main)
set_target_properties(test_all PROPERTIES FOLDER test)
target_include_directories(test_all PRIVATE ${CMAKE_CURRENT_LIST_DIR}/bmi)

# =============================================================================

#[==[
ngen_add_test(<test name> OBJECTS <source>...
                          LIBRARIES <library>...
                          [REQUIRES <varname>...])

Adds an executable test (i.e. a target) called <test name>.

`OBJECTS`
    Source files containing the test cases/fixtures.

`LIBRARIES`
    Library targets that this test requires linking to.

`REQUIRES`
    Variable names that this test's creation is dependent on.
    If a given variable is not defined or is FALSE/OFF/etc. then
    the corresponding <test name> target is not created.
#]==]
function(ngen_add_test TESTNAME)
    set(multiValueArgs OBJECTS LIBRARIES REQUIRES DEPENDS)
    cmake_parse_arguments(NGEN_TEST "" "" "${multiValueArgs}" ${ARGN})

    list(REMOVE_DUPLICATES NGEN_TEST_LIBRARIES)

    set(NGEN_TEST_CREATE ON)
    foreach(requirement_var IN LISTS NGEN_TEST_REQUIRES)
        if(NOT ${requirement_var})
            set(NGEN_TEST_CREATE OFF)
            break()
        endif()
    endforeach()

    if(NGEN_TEST_CREATE)
        add_executable(${TESTNAME} ${NGEN_TEST_OBJECTS})
        target_include_directories(${TESTNAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/bmi)
        target_link_libraries(${TESTNAME} PRIVATE gtest gtest_main ${NGEN_TEST_LIBRARIES})
        set_target_properties(${TESTNAME} PROPERTIES FOLDER test)

        # Iterate over each dependency, and add as a dependency for the test
        # IF the dependency is a target.
        foreach(dependency IN LISTS NGEN_TEST_DEPENDS)
            if(TARGET ${dependency})
                add_dependencies(${TESTNAME} ${dependency})
            endif()
        endforeach()

        gtest_discover_tests(
            ${TESTNAME}
            WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
            PROPERTIES
                VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
        )

        # ---------------------------------------------------------------------
        target_sources(test_all PRIVATE ${NGEN_TEST_OBJECTS})
        target_link_libraries(test_all PRIVATE ${NGEN_TEST_LIBRARIES})
        # ---------------------------------------------------------------------
    endif()
endfunction()

########################## Individual tests ##########################

########################## GeoJSON Unit Tests
ngen_add_test(
    test_geojson
    OBJECTS
        geojson/JSONProperty_Test.cpp
        geojson/JSONGeometry_Test.cpp
        geojson/Feature_Test.cpp
        geojson/FeatureCollection_Test.cpp
    LIBRARIES
        NGen::geojson
)

########################## GeoPackage Unit Tests
ngen_add_test(
    test_geopackage
    OBJECTS
        geopackage/WKB_Test.cpp
        geopackage/SQLite_Test.cpp
        geopackage/GeoPackage_Test.cpp
    LIBRARIES
        NGen::geopackage
    REQUIRES
        NGEN_WITH_SQLITE
)

########################## Realization Config Unit Tests
ngen_add_test(
    test_realization_config
    OBJECTS
        realizations/Formulation_Manager_Test.cpp
    LIBRARIES
        NGen::core
        NGen::realizations_catchment
        NGen::core_mediator
        NGen::forcing
        NGen::ngen_bmi
    DEPENDS
        testbmicppmodel
)

########################## Series Unit Tests
ngen_add_test(
    test_mdarray
    OBJECTS
        utils/mdarray_Test.cpp
        utils/mdframe_Test.cpp
        utils/mdframe_netcdf_Test.cpp
        utils/mdframe_csv_Test.cpp
    LIBRARIES
        NGen::mdarray
        NGen::mdframe
        
)

########################## Logging Unit Tests
ngen_add_test(
    test_logging
    OBJECTS
        utils/logging_Test.cpp
    LIBRARIES
        NGen::logging

)

########################## Nexus Tests
ngen_add_test(
    test_nexus
    OBJECTS
        core/nexus/NexusTests.cpp
    LIBRARIES
        NGen::core_nexus
)

########################## MPI Remote Nexus Tests
ngen_add_test(
    test_remote_nexus
    OBJECTS
        core/nexus/NexusRemoteTests.cpp
    LIBRARIES
        NGen::core_nexus
    REQUIRES
        NGEN_WITH_MPI
)

########################## Partitioning Tests
ngen_add_test(
    test_partition
    OBJECTS
        utils/Partition_Test.cpp
    LIBRARIES
        gmock
        NGen::core
        NGen::geojson
    # REQUIRES
    #   NGEN_WITH_MPI
)

########################## Partition_One Tests
ngen_add_test(
    test_partition_one
    OBJECTS
        utils/Partition_One_Test.cpp
    LIBRARIES
        gmock
        NGen::core
        NGen::geojson
        NGen::geopackage
    REQUIRES
        NGEN_WITH_SQLITE
)

########################## MultiLayer Tests
ngen_add_test(
    test_multilayer
    OBJECTS
        core/multilayer/MultiLayerParserTest.cpp
    LIBRARIES
        NGen::core
        NGen::geojson
        NGen::realizations_catchment
        NGen::core_mediator
        NGen::forcing
    REQUIRES
        NGEN_WITH_NETCDF
)


########################## BMI C++ Tests
ngen_add_test(
    test_bmi_cpp
    OBJECTS
        bmi/Bmi_Cpp_Adapter_Test.cpp
        realizations/catchments/Bmi_Cpp_Formulation_Test.cpp
    LIBRARIES
        NGen::core
        NGen::realizations_catchment
        NGen::core_mediator
        NGen::forcing
        NGen::ngen_bmi
    DEPENDS
        testbmicppmodel
)

########################## BMI C Tests
ngen_add_test(
    test_bmi_c
    OBJECTS
        bmi/Bmi_C_Adapter_Test.cpp
        realizations/catchments/Bmi_C_Formulation_Test.cpp
    LIBRARIES
        gmock
        NGen::core
        NGen::realizations_catchment
        NGen::core_mediator
        NGen::forcing
        NGen::ngen_bmi
    REQUIRES
        NGEN_WITH_BMI_C
    DEPENDS
        testbmicmodel
)

########################## BMI Fortran Tests
ngen_add_test(
    test_bmi_fortran
    OBJECTS
        bmi/Bmi_Fortran_Adapter_Test.cpp
        realizations/catchments/Bmi_Fortran_Formulation_Test.cpp
    LIBRARIES
        NGen::core
        NGen::realizations_catchment
        NGen::core_mediator
        NGen::forcing
        NGen::ngen_bmi
    REQUIRES
        NGEN_WITH_BMI_FORTRAN
    DEPENDS
        testbmifortranmodel
)

########################## BMI Python Tests
ngen_add_test(
    test_bmi_python
    OBJECTS
        bmi/Bmi_Py_Adapter_Test.cpp
        realizations/catchments/Bmi_Py_Formulation_Test.cpp
    LIBRARIES
        NGen::core
        NGen::realizations_catchment
        NGen::core_mediator
        NGen::forcing
        NGen::ngen_bmi
    REQUIRES
        NGEN_WITH_PYTHON
)

########################## BMI Multi Tests
ngen_add_test(
    test_bmi_multi
    OBJECTS
        realizations/catchments/Bmi_Multi_Formulation_Test.cpp
        realizations/catchments/Bmi_Cpp_Multi_Array_Test.cpp
    LIBRARIES
        NGen::core
        NGen::realizations_catchment
        NGen::core_mediator
        NGen::forcing
        NGen::ngen_bmi
    REQUIRES
        NGEN_WITH_BMI_C
        NGEN_WITH_BMI_FORTRAN
        NGEN_WITH_PYTHON
    DEPENDS
        testbmicppmodel
        testbmicmodel
        testbmifortranmodel
)

########################## All BMI Unit Tests
ngen_add_test(
    test_bmi_unit_all
    OBJECTS
        bmi/Bmi_C_Adapter_Test.cpp
        realizations/catchments/Bmi_C_Formulation_Test.cpp
        bmi/Bmi_Fortran_Adapter_Test.cpp
        realizations/catchments/Bmi_Fortran_Formulation_Test.cpp
        bmi/Bmi_Py_Adapter_Test.cpp
        realizations/catchments/Bmi_Py_Formulation_Test.cpp
        realizations/catchments/Bmi_Multi_Formulation_Test.cpp
    LIBRARIES
        gmock
        NGen::core
        NGen::realizations_catchment
        NGen::core_mediator
        NGen::forcing
        NGen::ngen_bmi
    REQUIRES
        NGEN_WITH_BMI_C
        NGEN_WITH_BMI_FORTRAN
        NGEN_WITH_PYTHON
)

########################## Comparison tests for the BMI PET implementation
# TODO: this probably needs to be added to integration testing also
ngen_add_test(
    compare_pet
    OBJECTS
        realizations/catchments/Bmi_C_Pet_IT.cpp
    LIBRARIES
        NGen::core
        NGen::realizations_catchment
        NGen::core_mediator
        NGen::forcing
        NGen::ngen_bmi
    DEPENDS
        petbmi
)

########################## Routing Tests
ngen_add_test(
    test_routing_pybind
    OBJECTS
        routing/Routing_Py_Bind_Test.cpp
    LIBRARIES
        NGen::core # for filechecker utility
        NGen::routing
        pybind11::embed
    REQUIRES
        NGEN_WITH_ROUTING
)

########################## Network Class Tests
ngen_add_test(
    test_network
    OBJECTS
        core/NetworkTests.cpp
    LIBRARIES
        NGen::core
        NGen::geojson
)

########################### Netcdf Forcing Tests
#if(NETCDF_ACTIVE)
ngen_add_test(
    test_netcdf_forcing
    OBJECTS
        forcing/NetCDFPerFeatureDataProvider_Test.cpp
    LIBRARIES
        NGen::core
        NGen::core_nexus
        NGen::core_mediator
        NGen::forcing
        NGen::geojson
        NGen::realizations_catchment
    REQUIRES
        NGEN_WITH_NETCDF
)
#endif()

########################## Primary Combined Unit Test Target
ngen_add_test(
    test_unit
    OBJECTS
        geojson/JSONProperty_Test.cpp
        geojson/JSONGeometry_Test.cpp
        geojson/Feature_Test.cpp
        geojson/FeatureCollection_Test.cpp
        forcing/CsvPerFeatureForcingProvider_Test.cpp
        forcing/OptionalWrappedDataProvider_Test.cpp
        forcing/NetCDFPerFeatureDataProvider_Test.cpp
        core/mediator/UnitsHelper_Tests.cpp
        simulation_time/Simulation_Time_Test.cpp
        core/NetworkTests.cpp
        utils/include/StreamOutputTest.cpp
        realizations/Formulation_Manager_Test.cpp
        utils/Partition_Test.cpp
        utils/mdarray_Test.cpp
        utils/mdframe_Test.cpp
        utils/mdframe_netcdf_Test.cpp
        utils/mdframe_csv_Test.cpp
        utils/logging_Test.cpp
    LIBRARIES
        gmock
        NGen::core
        NGen::core_nexus
        NGen::core_mediator
        NGen::forcing
        NGen::geojson
        NGen::realizations_catchment
        NGen::mdarray
        NGen::mdframe
        NGen::logging
        NGen::ngen_bmi
        testbmicppmodel
)

# Discover for test_all
gtest_discover_tests(
    test_all
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
)

if(NGEN_WITH_COVERAGE)
    setup_target_for_coverage_gcovr_html(
        NAME
            ngen_coverage_html
        EXECUTABLE
            test/test_all
        DEPENDENCIES
            test_all
        EXCLUDE
            "${BOOST_ROOT}"
            "${CMAKE_CURRENT_LIST_DIR}"
            "${NGEN_EXT_DIR}"
    )

    add_custom_command(
        TARGET ngen_coverage_html
        PRE_BUILD
        COMMAND 
    )

    setup_target_for_coverage_gcovr_xml(
        NAME
            ngen_coverage_xml
        EXECUTABLE
            test/test_all
        DEPENDENCIES
            test_all
        EXCLUDE
            "${BOOST_ROOT}"
            "${CMAKE_CURRENT_LIST_DIR}"
            "${NGEN_EXT_DIR}"
    )
endif()
