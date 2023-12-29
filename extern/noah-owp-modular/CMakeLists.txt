cmake_minimum_required(VERSION 3.12...3.26)
include(NGenModules)

# NGen NOAH-OWP-MODULAR listfile shim
#
# Requires:
# - NGEN_WITH_BMI_FORTRAN=ON
#
# Output Targets:
# - `surfacebmi`

project(noah-owp-modular VERSION 1.0.0 LANGUAGES Fortran)

set(_source_dir "${CMAKE_CURRENT_LIST_DIR}/noah-owp-modular")

add_fortran_library(surfacebmi SHARED)
file(GLOB _surfacebmi_objects CONFIGURE_DEPENDS
    "${_source_dir}/src/*.f90"
    "${_source_dir}/bmi/*.f90"
    "${_source_dir}/driver/*.f90")
target_sources(surfacebmi PRIVATE ${_surfacebmi_objects})

if(NGEN_IS_MAIN_PROJECT)
    target_link_libraries(surfacebmi PUBLIC iso_c_bmi)
    target_compile_options(surfacebmi PUBLIC -cpp -ffree-line-length-none)
    target_compile_definitions(surfacebmi
        PUBLIC
            BMI_ACTIVE
            NGEN_FORCING_ACTIVE
            NGEN_OUTPUT_ACTIVE
            NGEN_ACTIVE)
endif()

include(GNUInstallDirs)

install(TARGETS surfacebmi
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

configure_file(surfacebmi.pc.in surfacebmi.pc @ONLY)

install(FILES ${CMAKE_BINARY_DIR}/surfacebmi.pc
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
