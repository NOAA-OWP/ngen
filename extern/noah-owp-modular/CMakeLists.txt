cmake_minimum_required(VERSION 3.12...3.26)

# NGen NOAH-OWP-MODULAR listfile shim
#
# Requires:
# - NGEN_WITH_BMI_FORTRAN=ON
#
# Output Targets:
# - `surfacebmi`

project(noah-owp-modular VERSION 1.0.0 LANGUAGES Fortran)

set(_source_dir "${CMAKE_CURRENT_LIST_DIR}/noah-owp-modular")

add_library(surfacebmi SHARED)
file(GLOB _SURFACEBMI_OBJECTS CONFIGURE_DEPENDS
    "${_source_dir}/src/*.f90"
    "${_source_dir}/bmi/*.f90"
    "${_source_dir}/driver/*.f90")
get_target_property(_SURFACEBMI_BINARY_DIR surfacebmi BINARY_DIR)
set_target_properties(surfacebmi
    PROPERTIES
        Fortran_MODULE_DIRECTORY "${_SURFACEBMI_BINARY_DIR}/mod")

target_sources(surfacebmi PRIVATE ${_SURFACEBMI_OBJECTS})
target_compile_options(surfacebmi PRIVATE -cpp -ffree-line-length-none)
target_include_directories(surfacebmi INTERFACE "${_SURFACEBMI_BINARY_DIR}/mod")
target_compile_definitions(surfacebmi PRIVATE BMI_ACTIVE)

if(NGEN_IS_MAIN_PROJECT)
    target_link_libraries(surfacebmi PUBLIC iso_c_bmi)
    target_compile_definitions(surfacebmi PRIVATE NGEN_FORCING_ACTIVE NGEN_OUTPUT_ACTIVE NGEN_ACTIVE)
else()
    find_path(NETCDF_MODULE_DIR netcdf.mod
        PATHS "${NETCDF_ROOT}/include"
        REQUIRED)
    mark_as_advanced(NETCDF_MODULE_DIR)
    
    find_library(NETCDF_LIBRARY NAMES netcdff
        PATHS "${NETCDF_ROOT}/lib"
        REQUIRED)
    mark_as_advanced(NETCDF_LIBRARY)

    find_program(_NFCONFIG nf-config)
    if(_NFCONFIG)
        execute_process(
            COMMAND ${_NFCONFIG} "--version"
            OUTPUT_VARIABLE _NFVERSION)
            string(STRIP "${_NFVERSION}" NETCDF_VERSION)
            message(${NETCDF_VERSION})
    endif()

    target_include_directories(surfacebmi PUBLIC "${NETCDF_MODULE_DIR}")
    target_link_libraries(surfacebmi PUBLIC "${NETCDF_LIBRARY}")
endif()

unset(_SURFACEBMI_BINARY_DIR)
unset(_SURFACEBMI_OBJECTS)

include(GNUInstallDirs)
install(TARGETS surfacebmi
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
configure_file(surfacebmi.pc.in surfacebmi.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/surfacebmi.pc
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
