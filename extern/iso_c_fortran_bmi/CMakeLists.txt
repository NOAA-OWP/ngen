cmake_minimum_required(VERSION 3.12...3.26)
include(NGenModules)

# NGen iso_c_fortran_bmi listfile shim
#
# Requires:
# - NGEN_WITH_BMI_FORTRAN=ON
#
# Output Targets:
# - `iso_c_bmi`

project(iso_c_fortran_bmi VERSION 1.0.0 LANGUAGES Fortran)

set(_source_dir "${CMAKE_CURRENT_LIST_DIR}")

add_fortran_library(iso_c_bmi SHARED)
file(GLOB _ISO_C_BMI_OBJECTS CONFIGURE_DEPENDS "${_source_dir}/src/*.f90")
target_sources(iso_c_bmi PRIVATE ${_ISO_C_BMI_OBJECTS})
target_compile_options(iso_c_bmi PUBLIC -cpp -ffree-line-length-none)

include(GNUInstallDirs)
install(TARGETS iso_c_bmi
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(DIRECTORY ${CMAKE_Fortran_MODULE_DIRECTORY}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

configure_file(iso_c_bmi.pc.in iso_c_bmi.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/iso_c_bmi.pc
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
