cmake_minimum_required(VERSION 3.10)
enable_language( Fortran )

#Get the iso_c_fortran binding module to build as part of this build
add_subdirectory(../iso_c_fortran_bmi ${CMAKE_BINARY_DIR}/iso_c_bmi)


list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../cmake/")

project(summabmi VERSION 1.0.0 DESCRIPTION "Summa-Sundials BMI Module Shared Library")

set( CMAKE_Fortran_MODULE_DIRECTORY ${CMAKE_BINARY_DIR}/fortran)

#### Add variables for individual libraries that are used within the *.pc.in files
set(SUMMA_LIB_NAME_CMAKE summabmi)
set(SUMMA_LIB_DESC_CMAKE "Summa-Sundials BMI Module Shared Library")

# Make sure these are compiled with this directive UNCLEAR IF WE NEED THESE
add_compile_definitions(BMI_ACTIVE NGEN_FORCING_ACTIVE NGEN_OUTPUT_ACTIVE NGEN_ACTIVE)


set(ROOT_DIR ./)
set(F_MASTER ${ROOT_DIR}/summa)

SET (CMAKE_Fortran_COMPILER gfortran)

#include(bmi.cmake)
# --------------------------------------------------------------------------------------------
# When compiling Summa-Sundials it can be compiled in a Cluster or Debug mode, which is set by
# the CMAKE_BUILD_TYPES variables. The default is Release, cluster mode.

######### SET THE PATHS TO THE SUNDIALS LIBRARIES AND INCLUDE FILES #########
#############################################################################

# Add options for build type
set(CMAKE_BUILD_TYPES Debug Release Cluster_Debug Cluster)

# Set default configuration type to Release
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Cluster)
endif()

# Set compiler flags
if(CMAKE_BUILD_TYPE MATCHES Debug OR CMAKE_BUILD_TYPE MATCHES Cluster_Debug)
    message("Debug build.")
    add_definitions(-DDEBUG)
    set(FLAGS_NOAH -g -O0 -ffree-form -ffree-line-length-none -fmax-errors=0 -fPIC -Wfatal-errors)
    set(FLAGS_ALL -g -O0 -ffree-line-length-none -fmax-errors=0 -fPIC -Wfatal-errors)
else()
    message("Release build.")
    set(FLAGS_NOAH -O3 -ffree-form -ffree-line-length-none -fmax-errors=0 -fPIC -Wfatal-errors)
    set(FLAGS_ALL -O3 -ffree-line-length-none -fmax-errors=0 -fPIC -Wfatal-errors)
endif()

set(DIR_SUNDIALS ${ROOT_DIR}/sundials/instdir)

if(CMAKE_BUILD_TYPE MATCHES Cluster OR CMAKE_BUILD_TYPE MATCHES Cluster_Debug)

    # MKL needs BLA_VENDOR set with cmake
    set(BLA_VENDOR OpenBLAS)

    # Packages that are required
    find_package(NetCDF REQUIRED)
    find_package(LAPACK REQUIRED)

    # Set include directories
    set(INCLUDES $ENV{EBROOTNETCDFMINFORTRAN}/include ${netCDF_INCLUDES} ${LAPACK_INCLUDES})
    set(LIBRARIES SUMMA_NOAHMP ${netCDF_LIBRARIES} ${LAPACK_LIBRARIES} -lnetcdff -lopenblas)

    if(SUNDIALS_ACTIVE)
        message("\nBuilding IDA\n")

        link_directories(${DIR_SUNDIALS}/lib64)
        set(CMAKE_BUILD_RPATH "${DIR_SUNDIALS}/lib64")
        set(INC_SUNDIALS ${DIR_SUNDIALS}/include ${DIR_SUNDIALS}/fortran)
        set(LIB_SUNDIALS -lsundials_fnvecmanyvector_mod -lsundials_fida_mod -lsundials_fnvecserial_mod -lsundials_fsunlinsoldense_mod -lsundials_fsunmatrixdense_mod)
    endif()

else()

    set(SDKROOT "$(xcrun --show-sdk-path)")

    link_directories(/opt/local/lib)
    set(INCLUDES /opt/local/include /opt/local/lib)
    set(LIBRARIES SUMMA_NOAHMP -llapack -lgfortran -lnetcdff -lnetcdf)

    if(SUNDIALS_ACTIVE)
        message("\nBuilding IDA\n")

        link_directories(${DIR_SUNDIALS}/lib)
        set(CMAKE_BUILD_RPATH "${DIR_SUNDIALS}/lib")
        set(INC_SUNDIALS ${DIR_SUNDIALS}/include ${DIR_SUNDIALS}/fortran)
        set(LIB_SUNDIALS -lsundials_fnvecmanyvector_mod -lsundials_fida_mod -lsundials_fnvecserial_mod -lsundials_fsunlinsoldense_mod -lsundials_fsunmatrixdense_mod)
    endif()

endif()


message("\nBuilding SUMMA\n")


#compile_with_bmi(${F_MASTER}, ${DIR_SUNDIALS}, ${INCLUDES}, ${LIBRARIES}, ${INC_SUNDIALS}, ${LIB_SUNDIALS}, ${FLAGS_ALL}, ${FLAGS_NOAH})
#function(compile_with_bmi F_MASTER, DIR_SUNDIALS, INCLUDES, LIBRARIES, INC_SUNDIALS, LIB_SUNDIALS, FLAGS_ALL, FLAGS_NOAH)

#========================================================================
# PART 1: Define directory paths
#========================================================================

# Core directory that contains source code
    set(F_KORE_DIR ${F_MASTER}/build/source)

# Define directories
    set(DRIVER_DIR ${F_KORE_DIR}/driver)
    set(HOOKUP_DIR ${F_KORE_DIR}/hookup)
    set(NETCDF_DIR ${F_KORE_DIR}/netcdf)
    set(DSHARE_DIR ${F_KORE_DIR}/dshare)
    set(NUMREC_DIR ${F_KORE_DIR}/numrec)
    set(NOAHMP_DIR ${F_KORE_DIR}/noah-mp)
    set(ENGINE_DIR ${F_KORE_DIR}/engine)

#========================================================================
# PART 2: Assemble all of the SUMMA sub-routines
#========================================================================

# utilities
    set(NRUTIL
        ${ENGINE_DIR}/nrtype.f90
        ${ENGINE_DIR}/f2008funcs.f90
        ${ENGINE_DIR}/nr_utility.f90)

# Numerical recipes procedures
# NOTE: all numerical recipes procedures are now replaced with free versions
    set(NRPROC
        ${ENGINE_DIR}/expIntegral.f90
        ${ENGINE_DIR}/spline_int.f90)

# Hook-up modules
    set(HOOKUP
        ${HOOKUP_DIR}/ascii_util.f90
        ${HOOKUP_DIR}/summaFileManager.f90)

# Data modules
    set(DATAMS
        ${DSHARE_DIR}/multiconst.f90
        ${DSHARE_DIR}/var_lookup.f90
        ${DSHARE_DIR}/data_types.f90
        ${DSHARE_DIR}/globalData.f90
        ${DSHARE_DIR}/flxMapping.f90
        ${DSHARE_DIR}/get_ixname.f90
        ${DSHARE_DIR}/popMetadat.f90
        ${DSHARE_DIR}/outpt_stat.f90)

# utility modules
    set(UTILMS
        ${ENGINE_DIR}/time_utils.f90
        ${ENGINE_DIR}/mDecisions.f90
        ${ENGINE_DIR}/snow_utils.f90
        ${ENGINE_DIR}/soil_utils.f90
        ${ENGINE_DIR}/updatState.f90
        ${ENGINE_DIR}/matrixOper.f90
        ${ENGINE_DIR}/soil_utilsAddSundials.f90
        ${ENGINE_DIR}/updatStateSundials.f90)

# Solver
    set(SOLVER
        ${ENGINE_DIR}/vegPhenlgy.f90
        ${ENGINE_DIR}/diagn_evar.f90
        ${ENGINE_DIR}/stomResist.f90
        ${ENGINE_DIR}/groundwatr.f90
        ${ENGINE_DIR}/vegSWavRad.f90
        ${ENGINE_DIR}/vegNrgFlux.f90
        ${ENGINE_DIR}/ssdNrgFlux.f90
        ${ENGINE_DIR}/vegLiqFlux.f90
        ${ENGINE_DIR}/snowLiqFlx.f90
        ${ENGINE_DIR}/soilLiqFlx.f90
        ${ENGINE_DIR}/bigAquifer.f90
        ${ENGINE_DIR}/computFlux.f90
        ${ENGINE_DIR}/computEnthalpy.f90
        ${ENGINE_DIR}/computHeatCap.f90
        ${ENGINE_DIR}/computThermConduct.f90
        ${ENGINE_DIR}/computResid.f90
        ${ENGINE_DIR}/computJacob.f90
        ${ENGINE_DIR}/eval8summa.f90
        ${ENGINE_DIR}/summaSolve.f90
        ${ENGINE_DIR}/systemSolv.f90
        ${ENGINE_DIR}/computSnowDepth.f90
        ${ENGINE_DIR}/varSubstep.f90
        ${ENGINE_DIR}/opSplittin.f90
        ${ENGINE_DIR}/coupled_em.f90
        ${ENGINE_DIR}/run_oneHRU.f90
        ${ENGINE_DIR}/run_oneGRU.f90)
    set(SOLVER_SUN
        ${ENGINE_DIR}/type4IDA.f90
        ${ENGINE_DIR}/tol4IDA.f90
        ${ENGINE_DIR}/computResidSundials.f90
        ${ENGINE_DIR}/eval8summaSundials.f90
        ${ENGINE_DIR}/computJacobSundials.f90
        ${ENGINE_DIR}/summaSolveSundialsIDA.f90
        ${ENGINE_DIR}/systemSolvSundials.f90)


# Define routines for SUMMA preliminaries
    set(PRELIM
        ${ENGINE_DIR}/conv_funcs.f90
        ${ENGINE_DIR}/sunGeomtry.f90
        ${ENGINE_DIR}/convE2Temp.f90
        ${ENGINE_DIR}/allocspace.f90
        ${ENGINE_DIR}/checkStruc.f90
        ${ENGINE_DIR}/childStruc.f90
        ${ENGINE_DIR}/ffile_info.f90
        ${ENGINE_DIR}/read_attrb.f90
        ${ENGINE_DIR}/read_pinit.f90
        ${ENGINE_DIR}/pOverwrite.f90
        ${ENGINE_DIR}/read_param.f90
        ${ENGINE_DIR}/paramCheck.f90
        ${ENGINE_DIR}/check_icond.f90)

    set(NOAHMP
        ${NOAHMP_DIR}/module_model_constants.F
        ${NOAHMP_DIR}/module_sf_noahutl.F
        ${NOAHMP_DIR}/module_sf_noahlsm.F
        ${NOAHMP_DIR}/module_sf_noahmplsm.F)

# Define routines for the SUMMA model runs
    set(MODRUN
        ${ENGINE_DIR}/indexState.f90
        ${ENGINE_DIR}/getVectorz.f90
        ${ENGINE_DIR}/t2enthalpy.f90
        ${ENGINE_DIR}/updateVars.f90
        ${ENGINE_DIR}/var_derive.f90
        ${ENGINE_DIR}/read_force.f90
        ${ENGINE_DIR}/derivforce.f90
        ${ENGINE_DIR}/snowAlbedo.f90
        ${ENGINE_DIR}/canopySnow.f90
        ${ENGINE_DIR}/tempAdjust.f90
        ${ENGINE_DIR}/snwCompact.f90
        ${ENGINE_DIR}/layerMerge.f90
        ${ENGINE_DIR}/layerDivide.f90
        ${ENGINE_DIR}/volicePack.f90
        ${ENGINE_DIR}/qTimeDelay.f90
        ${ENGINE_DIR}/updateVarsSundials.f90)


# Define NetCDF routines
    set(NETCDF
        ${NETCDF_DIR}/netcdf_util.f90
        ${NETCDF_DIR}/def_output.f90
        ${NETCDF_DIR}/modelwrite.f90
        ${NETCDF_DIR}/read_icond.f90)

# Define the driver routine
    set(DRIVER
        ${DRIVER_DIR}/summa_type.f90
        ${DRIVER_DIR}/summa_util.f90
        ${DRIVER_DIR}/summa_alarms.f90
        ${DRIVER_DIR}/summa_globalData.f90
        ${DRIVER_DIR}/summa_defineOutput.f90
        ${DRIVER_DIR}/summa_init.f90
        ${DRIVER_DIR}/summa_setup.f90
        ${DRIVER_DIR}/summa_restart.f90
        ${DRIVER_DIR}/summa_forcing.f90
        ${DRIVER_DIR}/summa_modelRun.f90
        ${DRIVER_DIR}/summa_writeOutput.f90
        ${DRIVER_DIR}/summa_driver.f90)

    # run program files, do not use in ngen
    # set(SUMMA ${DRIVER_DIR}/summa_run.f90)
    # set(BMI ${DRIVER_DIR}/summa_runBMI.f90)

# Define version number, not working correctly
    set(VERSIONFILE ${DRIVER_DIR}/summaversion.inc)
    execute_process(COMMAND "${GIT_EXECUTABLE} tag | tail -n 1" OUTPUT_VARIABLE VERSION)
    execute_process(COMMAND "date" OUTPUT_VARIABLE BULTTIM)
    execute_process(COMMAND "${GIT_EXECUTABLE} describe --long --all --always | sed -e's/heads\///'" OUTPUT_VARIABLE GITBRCH)
    execute_process(COMMAND "${GIT_EXECUTABLE} rev-parse HEAD" OUTPUT_VARIABLE GITHASH)

    set(COMM_ALL
        ${NRPROC}
        ${HOOKUP}
        ${DATAMS}
        ${UTILMS})

    if(SUNDIALS_ACTIVE)
        add_compile_definitions(NGEN_SUNDIALS_ACTIVE)
        set(SUMMA_ALL
            ${NETCDF}
            ${PRELIM}
            ${MODRUN}
            ${SOLVER}
            ${DRIVER}
            ${SOLVER_SUN})
    else()
        set(SUMMA_ALL
            ${NETCDF}
            ${PRELIM}
            ${MODRUN}
            ${SOLVER}
            ${DRIVER})
    endif()

#========================================================================
# PART 4: compilation
#======================================================================

# update version information, not working correctly
    file(WRITE  ${VERSIONFILE} "character(len=64), parameter     :: summaVersion = '${VERSION}'\n")
    file(APPEND ${VERSIONFILE} "character(len=64), parameter     :: buildTime = ''\n")
    file(APPEND ${VERSIONFILE} "character(len=64), parameter     :: gitBranch = '${GITBRCH}'\n")
    file(APPEND ${VERSIONFILE} "character(len=64), parameter     :: gitHash = '${GITHASH}'")

    add_library(SUMMA_NOAHMP OBJECT ${NOAHMP} ${NRUTIL})
    target_compile_options(SUMMA_NOAHMP PRIVATE ${FLAGS_NOAH})
    add_library(SUMMA_COMM OBJECT ${COMM_ALL})
    target_compile_options(SUMMA_COMM PRIVATE ${FLAGS_ALL})
    target_include_directories(SUMMA_COMM PRIVATE ${INCLUDES})
    target_link_libraries(SUMMA_COMM PUBLIC ${LIBRARIES})

# Build SUMMA Shared Library, add BMI libraries outside this function
    if(WIN32)
        add_library(summabmi ${SUMMA_ALL})
    else()
        add_library(summabmi SHARED ${SUMMA_ALL})
    endif()
    target_compile_options(summabmi PRIVATE ${FLAGS_ALL})

    if(SUNDIALS_ACTIVE)
        target_include_directories(summabmi PUBLIC ${INCLUDES} ${INC_SUNDIALS})
        target_link_libraries(summabmi PUBLIC ${LIBRARIES} ${LIB_SUNDIALS} SUMMA_COMM)
    else()
        target_include_directories(summabmi PUBLIC ${INCLUDES})
        target_link_libraries(summabmi PUBLIC ${LIBRARIES} SUMMA_COMM)
    endif()

#endfunction()


target_link_libraries(summabmi PUBLIC iso_c_bmi)
target_compile_options(summabmi PUBLIC -cpp -DNGEN_ACTIVE)

set_target_properties(summabmi PROPERTIES VERSION ${PROJECT_VERSION})

include(GNUInstallDirs)

install(TARGETS summabmi
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

configure_file(summabmi.pc.in summabmi.pc @ONLY)

install(FILES ${CMAKE_BINARY_DIR}/summabmi.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
