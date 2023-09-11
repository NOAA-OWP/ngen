# Searches for ISO C BMI library, defaults to
# known development directories in the event the BMI_FORTRAN_ISO_C_*
# variables are not set.
find_library(BMI_FORTRAN_ISO_C_LIB_PATH
    NAMES
        "${BMI_FORTRAN_ISO_C_LIB_NAME}"
        "iso_c_bmi"
    HINTS # Searched before paths
        "${BMI_FORTRAN_ISO_C_LIB_DIR}" 
    PATHS
        ENV BMI_FORTRAN_ISO_C_LIB_DIR
        "${NGEN_ROOT_DIR}/extern/iso_c_fortran_bmi/cmake_build"
        "${NGEN_ROOT_DIR}/extern/iso_c_fortran_bmi/build"
    DOC "Path to middleware Fortran shared lib handling iso_c_binding"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(BMI_FORTRAN_ISO_C DEFAULT_MSG BMI_FORTRAN_ISO_C_LIB_PATH)

if(BMI_FORTRAN_ISO_C_FOUND)
    add_library(bmi_fortran_iso_c SHARED IMPORTED)
    set_target_properties(bmi_fortran_iso_c PROPERTIES IMPORTED_LOCATION "${BMI_FORTRAN_ISO_C_LIB_PATH}")
endif()
