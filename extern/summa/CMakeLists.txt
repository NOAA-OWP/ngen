cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
enable_language(C Fortran)
set (CMAKE_Fortran_COMPILER $ENV{FC})  #for NexGen need, works for others
#set (CMAKE_Fortran_COMPILER gfortran) # for others may work


#=========================================================================================
# When compiling Summa each configuration can be compiled in a Cluster or Debug mode,
# which is set by the CMAKE_BUILD_TYPES variables. The default is BE, release mode no
# cluster no Sundials no Actors no NexGen which runs the code with BE based off Numerical
# Recipes only.
# Note, Sundials, NexGen, and Actors will all require extra libraries to be downloaded
# and compiled. Currently, NexGen cannot work with Actors.
# If you install Sundials, you can compile Summa with Sundials, which allows you to run
# Summa with a solver of BE based off Numerical Recipes, Kinsol, or IDA. The solver choice
# is then set in the code decisions file.
# Other options include how to drive the code, NexGen (with BMI) or Actors (without BMI)
# framework or no extra framework (without BMI).
# If you change the build type, you will need to delete the CMakeCache.txt file and
# rerun cmake.
#=========================================================================================

# Set default configuration type to Release
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE BE)
endif()

# Add options for build type
set(CMAKE_CONFIGURATION_TYPES BE BE_Debug BE_Cluster BE_Cluster_Debug
                              BE_NexGen BE_NexGen_Debug BE_NexGen_Cluster BE_NexGen_Cluster_Debug
                              BE_Actors BE_Actors_Debug BE_Actors_Cluster BE_Actors_Cluster_Debug
                              Sundials Sundials_Debug Sundials_Cluster Sundials_Cluster_Debug
                              Sundials_NexGen Sundials_NexGen_Debug Sundials_NexGen_Cluster Sundials_NexGen_Cluster_Debug
                              Sundials_Actors Sundials_Actors_Debug Sundials_Actors_Cluster Sundials_Actors_Cluster_Debug)
message("\nSelected Bulid Type: ${CMAKE_BUILD_TYPE}\n")

# Give executable names to be backwards compatible with old versions
if(CMAKE_BUILD_TYPE MATCHES Sundials)
    add_compile_definitions(SUNDIALS_ACTIVE)
    set(EXEC_NAME summa_sundials.exe)
elseif(CMAKE_BUILD_TYPE MATCHES BE)
    set(EXEC_NAME summa.exe)
endif()
if(CMAKE_BUILD_TYPE MATCHES Actors)
    set(EXEC_NAME summa_actors)
endif()

if(CMAKE_BUILD_TYPE MATCHES NexGen)
    message("\nUsing NexGen Framework, should have been installed previously")
    add_compile_definitions(NGEN_ACTIVE BMI_ACTIVE NGEN_FORCING_ACTIVE NGEN_OUTPUT_ACTIVE)

    project(summabmi VERSION 1.0.0 DESCRIPTION "Summa-Sundials-BE BMI Module Shared Library")

    # NexGen needs to have CMakeLists one directory above summa submodule GIT,
    #    at ${NexGen GIT directory}/extern/summa
    set(F_MASTER ./summa/) # directory of summa source code
    set(PARENT_DIR ${F_MASTER}) # directory of summa source code

    # Get the iso_c_fortran binding module to build as part of this build
    add_subdirectory(../iso_c_fortran_bmi ${CMAKE_BINARY_DIR}/iso_c_bmi)

    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../cmake/")
    set( CMAKE_Fortran_MODULE_DIRECTORY ${CMAKE_BINARY_DIR}/fortran)

    #### Add variables for individual libraries that are used within the *.pc.in files
    set(SUMMA_LIB_NAME_CMAKE summabmi)
    set(SUMMA_LIB_DESC_CMAKE "Summa-Sundials BMI Module Shared Library")

else()
    set(F_MASTER ../../) # directory of summa source code

    if(CMAKE_BUILD_TYPE MATCHES Actors)
        message("\nUsing Actors Framework, should have been installed previously")
        add_compile_definitions(ACTORS_ACTIVE)

        project(summaactors DESCRIPTION "Summa-Sundials-BE Actors")

        include(FortranCInterface)
        FortranCInterface_VERIFY(CXX)

        # CMakeLists in ${summa-actors GIT directory}/build/summa/build/cmake
        set(PARENT_DIR ${F_MASTER}../../) # directory of summa actors source code
        set(EXEC_DIR ${PARENT_DIR}../bin)

    else()
        project(summa DESCRIPTION "Summa-Sundials-BE")

        # CMakeLists in ${summa-actors GIT directory}/build/cmake
        set(PARENT_DIR ${F_MASTER}) # directory of summa source code
        set(EXEC_DIR ${F_MASTER}/bin)

    endif()

    # set the output directory for executables
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${EXEC_DIR})

endif()

#=========================================================================================
# SET THE PATHS TO THE LIBRARIES AND INCLUDE FILES
#=========================================================================================

# Set compiler flags
if(CMAKE_BUILD_TYPE MATCHES Debug)
    message("\nSetting Debug Options")
    add_compile_definitions(DEBUG)
    set(FLAGS_NOAH -g -O0 -ffree-form -ffree-line-length-none -fmax-errors=0 -fPIC -Wfatal-errors)
    set(FLAGS_ALL  -g -O0 -ffree-line-length-none -fmax-errors=0 -fPIC -Wfatal-errors -cpp)
    set(FLAGS_CXX  -g -O0 -Wfatal-errors -std=c++17)
else()
    message("\nSetting Release Options")
    set(FLAGS_NOAH -O3 -ffree-form -ffree-line-length-none -fmax-errors=0 -fPIC -Wfatal-errors)
    set(FLAGS_ALL  -O3 -ffree-line-length-none -fmax-errors=0 -fPIC -Wfatal-errors -cpp)
    set(FLAGS_CXX  -O3 -Wfatal-errors -std=c++17)
endif()

# Find Sundials if exists, add path here if FATAL_ERROR "Did not find Sundials directory"
if(CMAKE_BUILD_TYPE MATCHES Sundials)
    set(DIR_SUNDIALS "")
    foreach(dir IN ITEMS
        "${F_MASTER}../sundials/instdir"
        "${PARENT_DIR}../sundials/instdir"
        "../../../../SummaSundials/sundials/instdir")
        if(EXISTS ${dir})
            set(DIR_SUNDIALS ${dir})
            message("\nFound Sundials directory at ${DIR_SUNDIALS}")
            break()
        endif()
        message(FATAL_ERROR "\nDid not find Sundials directory, edit CMakeLists.txt to include path")
    endforeach()
endif()

# Set libraries for all builds
if(CMAKE_BUILD_TYPE MATCHES Cluster)
    message("\nBuilding for cluster")

    # MKL needs BLA_VENDOR set with cmake
    set(BLA_VENDOR OpenBLAS)

    # Packages that are required
    find_package(NetCDF REQUIRED)
    find_package(LAPACK REQUIRED)

    # Set include directories
    set(INCLUDES $ENV{EBROOTNETCDFMINFORTRAN}/include ${netCDF_INCLUDES} ${LAPACK_INCLUDES})
    set(LIBRARIES SUMMA_NOAHMP ${netCDF_LIBRARIES} ${LAPACK_LIBRARIES} -lnetcdff -lopenblas)

    if(CMAKE_BUILD_TYPE MATCHES Sundials)
        link_directories(${DIR_SUNDIALS}/lib64)
        set(CMAKE_BUILD_RPATH "${DIR_SUNDIALS}/lib64")
        set(INC_SUNDIALS ${DIR_SUNDIALS}/include ${DIR_SUNDIALS}/fortran)
    endif()

    if(CMAKE_BUILD_TYPE MATCHES Actors)
        find_package(CAF REQUIRED)
        set(INC_ACTORS ${CAF_INCLUDES} ${PARENT_DIR}/build/includes/global ${PARENT_DIR}/build/includes/summa_actor ${PARENT_DIR}/build/includes/gru_actor ${PARENT_DIR}/build/includes/job_actor ${PARENT_DIR}/build/includes/file_access_actor ${PARENT_DIR}/build/includes/hru_actor)
        set(LIB_ACTORS ${CAF_LIBRARIES} -lcaf_core -lcaf_io)
    endif()


else()
    message("\nBuilding for personal computer")

    set(SDKROOT "$(xcrun --show-sdk-path)")

    link_directories(/opt/local/lib)
    set(INCLUDES /opt/local/include /opt/local/lib)
    set(LIBRARIES SUMMA_NOAHMP -llapack -lgfortran -lnetcdff -lnetcdf)

    if(CMAKE_BUILD_TYPE MATCHES Sundials)
        link_directories(${DIR_SUNDIALS}/lib)
        set(CMAKE_BUILD_RPATH "${DIR_SUNDIALS}/lib")
        set(INC_SUNDIALS ${DIR_SUNDIALS}/include ${DIR_SUNDIALS}/fortran)
    endif()

    # Actors is on Cluster, if using on personal computer might need to link directory-- not working on Mac right now
    if(CMAKE_BUILD_TYPE MATCHES Actors)
        #link_directories(${DIR_ACTORS}/lib)
        find_package(CAF REQUIRED)
        set(INC_ACTORS ${CAF_INCLUDES} ${PARENT_DIR}/build/includes/global ${PARENT_DIR}/build/includes/summa_actor ${PARENT_DIR}/build/includes/gru_actor ${PARENT_DIR}/build/includes/job_actor ${PARENT_DIR}/build/includes/file_access_actor ${PARENT_DIR}/build/includes/hru_actor)
       set(LIB_ACTORS ${CAF_LIBRARIES} -lcaf_core -lcaf_io)
    endif()

endif()


if(CMAKE_BUILD_TYPE MATCHES Sundials)
    message("\nUsing SUNDIALS libraries for IDA and Kinsol, should have been installed previously")
    set(LIB_SUNDIALS -lsundials_fida_mod -lsundials_fnvecmanyvector_mod -lsundials_fnvecserial_mod -lsundials_fsunlinsoldense_mod -lsundials_fsunmatrixdense_mod -lsundials_fkinsol_mod -lsundials_fsunnonlinsolnewton_mod)
elseif(CMAKE_BUILD_TYPE MATCHES BE)
    message("\nUsing Backward Euler based off free versions of Numerical Recipes only")
else()
    message("\nValid build types are: ${CMAKE_CONFIGURATION_TYPES}")
    message(FATAL_ERROR "\nUnknown build type: ${CMAKE_BUILD_TYPE}")
endif()

message("\nBuilding SUMMA\n")


#=========================================================================================
# COMPILE PART 1: Define directory paths
#=========================================================================================

# Define directories that contains source code
set(DRIVER_DIR ${F_MASTER}/build/source/driver)
set(DSHARE_DIR ${F_MASTER}/build/source/dshare)
set(ENGINE_DIR ${F_MASTER}/build/source/engine)
set(HOOKUP_DIR ${F_MASTER}/build/source/hookup)
set(NETCDF_DIR ${F_MASTER}/build/source/netcdf)
set(NOAHMP_DIR ${F_MASTER}/build/source/noah-mp)

# Define directories for source files that might be replaced by actors (identical if not using actors)
set(SUB_DRIVER_DIR ${PARENT_DIR}/build/source/driver)
set(SUB_DSHARE_DIR ${PARENT_DIR}/build/source/dshare)
set(SUB_ENGINE_DIR ${PARENT_DIR}/build/source/engine)
set(SUB_HOOKUP_DIR ${PARENT_DIR}/build/source/hookup)
set(SUB_NETCDF_DIR ${PARENT_DIR}/build/source/netcdf)

# Define Actors specific directories
set(ACTORS_DIR ${PARENT_DIR}/build/source/actors)
set(FILE_ACCESS_DIR ${ACTORS_DIR}/file_access_actor)
set(JOB_ACTOR_DIR   ${ACTORS_DIR}/job_actor)
set(HRU_ACTOR_DIR   ${ACTORS_DIR}/hru_actor)
set(GRU_ACTOR_DIR   ${ACTORS_DIR}/gru_actor)


#=========================================================================================
# COMPILE PART 2: Assemble all of the SUMMA sub-routines
#=========================================================================================

# NOAHMP modules
set(NOAHMP
    ${NOAHMP_DIR}/module_model_constants.F
    ${NOAHMP_DIR}/module_sf_noahutl.F
    ${NOAHMP_DIR}/module_sf_noahlsm.F
    ${NOAHMP_DIR}/module_sf_noahmplsm.F)

# Free versions of numerical recipes utilities for NOAH-MP modules
set(NRUTIL
    ${ENGINE_DIR}/f2008funcs.f90
    ${ENGINE_DIR}/nr_utility.f90
    ${ENGINE_DIR}/nrtype.f90)

# Free versions of numerical recipes procedures for SUMMA modules
set(NRPROC
    ${ENGINE_DIR}/expIntegral.f90
    ${ENGINE_DIR}/spline_int.f90)

# Hook-up modules
set(HOOKUP
    ${HOOKUP_DIR}/ascii_util.f90
    ${SUB_HOOKUP_DIR}/summaFileManager.f90)

# Data modules
set(DATAMS
    ${SUB_DSHARE_DIR}/data_types.f90
    ${DSHARE_DIR}/flxMapping.f90
    ${DSHARE_DIR}/get_ixname.f90
    ${DSHARE_DIR}/globalData.f90
    ${DSHARE_DIR}/multiconst.f90
    ${DSHARE_DIR}/outpt_stat.f90
    ${DSHARE_DIR}/popMetadat.f90
    ${DSHARE_DIR}/var_lookup.f90)
set(DATAMS_SUNDIALS
    ${DSHARE_DIR}/type4ida.f90
    ${DSHARE_DIR}/type4kinsol.f90)

# Utility modules
set(UTILMS
    ${ENGINE_DIR}/matrixOper.f90
    ${ENGINE_DIR}/mDecisions.f90
    ${ENGINE_DIR}/snow_utils.f90
    ${ENGINE_DIR}/soil_utils.f90
    ${ENGINE_DIR}/time_utils.f90
    ${ENGINE_DIR}/updatState.f90)
set(UTILMS_SUNDIALS
    ${ENGINE_DIR}/soil_utilsAddSundials.f90
    ${ENGINE_DIR}/updatStateSundials.f90)

# NetCDF routines
set(NETCDF
    ${SUB_NETCDF_DIR}/def_output.f90
    ${SUB_NETCDF_DIR}/modelwrite.f90
    ${NETCDF_DIR}/netcdf_util.f90
    ${SUB_NETCDF_DIR}/read_icond.f90)

# Preliminary modules
set(PRELIM
    ${SUB_ENGINE_DIR}/allocspace.f90
    ${SUB_ENGINE_DIR}/check_icond.f90
    ${ENGINE_DIR}/checkStruc.f90
    ${ENGINE_DIR}/childStruc.f90
    ${ENGINE_DIR}/conv_funcs.f90
    ${ENGINE_DIR}/convE2Temp.f90
    ${SUB_ENGINE_DIR}/ffile_info.f90
    ${ENGINE_DIR}/read_attrb.f90
    ${ENGINE_DIR}/read_pinit.f90
    ${ENGINE_DIR}/paramCheck.f90
    ${ENGINE_DIR}/pOverwrite.f90
    ${ENGINE_DIR}/read_param.f90
    ${ENGINE_DIR}/sunGeomtry.f90)
set(PRELIM_ACTOR
    ${SUB_ENGINE_DIR}/alloc_fileAccess.f90)

# Model run support modules
set(MODRUN
    ${ENGINE_DIR}/canopySnow.f90
    ${ENGINE_DIR}/derivforce.f90
    ${ENGINE_DIR}/getVectorz.f90
    ${ENGINE_DIR}/indexState.f90
    ${ENGINE_DIR}/layerMerge.f90
    ${ENGINE_DIR}/layerDivide.f90
    ${ENGINE_DIR}/qTimeDelay.f90
    ${ENGINE_DIR}/read_force.f90
    ${ENGINE_DIR}/snowAlbedo.f90
    ${ENGINE_DIR}/snwCompact.f90
    ${ENGINE_DIR}/t2enthalpy.f90
    ${ENGINE_DIR}/tempAdjust.f90
    ${ENGINE_DIR}/updateVars.f90
    ${ENGINE_DIR}/var_derive.f90
    ${ENGINE_DIR}/volicePack.f90)
set(MODRUN_SUNDIALS
    ${ENGINE_DIR}/tol4ida.f90
    ${ENGINE_DIR}/updateVarsSundials.f90)

# Solver main modules
set(SOLVER
    ${ENGINE_DIR}/bigAquifer.f90
    ${ENGINE_DIR}/computEnthalpy.f90
    ${ENGINE_DIR}/computFlux.f90
    ${ENGINE_DIR}/computHeatCap.f90
    ${ENGINE_DIR}/computJacob.f90
    ${ENGINE_DIR}/computResid.f90
    ${ENGINE_DIR}/computSnowDepth.f90
    ${ENGINE_DIR}/computThermConduct.f90
    ${ENGINE_DIR}/coupled_em.f90
    ${ENGINE_DIR}/diagn_evar.f90
    ${ENGINE_DIR}/eval8summa.f90
    ${ENGINE_DIR}/groundwatr.f90
    ${ENGINE_DIR}/opSplittin.f90
    ${ENGINE_DIR}/snowLiqFlx.f90
    ${ENGINE_DIR}/soilLiqFlx.f90
    ${ENGINE_DIR}/ssdNrgFlux.f90
    ${ENGINE_DIR}/stomResist.f90
    ${ENGINE_DIR}/summaSolve.f90
    ${ENGINE_DIR}/systemSolv.f90
    ${ENGINE_DIR}/varSubstep.f90
    ${ENGINE_DIR}/vegLiqFlux.f90
    ${ENGINE_DIR}/vegNrgFlux.f90
    ${ENGINE_DIR}/vegPhenlgy.f90
    ${ENGINE_DIR}/vegSWavRad.f90)
set(SOLVER_SUNDIALS
    ${ENGINE_DIR}/computJacobSundials.f90
    ${ENGINE_DIR}/computResidSundials.f90
    ${ENGINE_DIR}/eval8summaSundials.f90
    ${ENGINE_DIR}/summaSolveSundials4ida.f90
    ${ENGINE_DIR}/systemSolvSundials.f90)
set(SOLVER_NOT_ACTORS
    ${ENGINE_DIR}/run_oneGRU.f90
    ${ENGINE_DIR}/run_oneHRU.f90)

# Driver support modules
set(DRIVER
    ${SUB_DRIVER_DIR}/summa_type.f90
    ${SUB_DRIVER_DIR}/summa_util.f90
    ${DRIVER_DIR}/summa_alarms.f90
    ${SUB_DRIVER_DIR}/summa_globalData.f90)
set(DRIVER_NOT_ACTORS
    ${DRIVER_DIR}/summa_defineOutput.f90
    ${DRIVER_DIR}/summa_init.f90
    ${DRIVER_DIR}/summa_setup.f90
    ${DRIVER_DIR}/summa_restart.f90
    ${DRIVER_DIR}/summa_forcing.f90
    ${DRIVER_DIR}/summa_modelRun.f90
    ${DRIVER_DIR}/summa_writeOutput.f90)
set(DRIVER_NEXGEN
    ${DRIVER_DIR}/summa_bmi.f90)

# Actors interface modules
set(INTERFACE
    ${ACTORS_DIR}/global/cppwrap_datatypes.f90
    ${ACTORS_DIR}/global/cppwrap_auxiliary.f90
    ${ACTORS_DIR}/global/cppwrap_metadata.f90)
set(FILE_ACCESS_INTERFACE
    ${FILE_ACCESS_DIR}/fortran_code/output_structure.f90
    ${FILE_ACCESS_DIR}/fortran_code/cppwrap_fileAccess.f90
    ${FILE_ACCESS_DIR}/fortran_code/read_attribute.f90
    ${FILE_ACCESS_DIR}/fortran_code/read_forcing.f90
    ${FILE_ACCESS_DIR}/fortran_code/read_param.f90
    ${FILE_ACCESS_DIR}/fortran_code/read_initcond.f90
    ${FILE_ACCESS_DIR}/fortran_code/writeOutputFromOutputStructure.f90
    ${FILE_ACCESS_DIR}/fortran_code/write_to_netcdf.f90)
set(JOB_INTERFACE
    ${JOB_ACTOR_DIR}/job_actor.f90)
set(HRU_INTERFACE
    ${HRU_ACTOR_DIR}/fortran_code/model_run.f90
    ${HRU_ACTOR_DIR}/fortran_code/setup_hru.f90
    ${HRU_ACTOR_DIR}/fortran_code/restart.f90
    ${HRU_ACTOR_DIR}/fortran_code/hru_actor.f90
    ${HRU_ACTOR_DIR}/fortran_code/init_hru_actor.f90
    ${HRU_ACTOR_DIR}/fortran_code/outputStrucWrite.f90
    ${HRU_ACTOR_DIR}/fortran_code/hru_writeOutput.f90)

# Actors actual actor modules
set(ACTORS_GLOBAL
    ${ACTORS_DIR}/global/global.cpp
    ${ACTORS_DIR}/global/timing_info.cpp
    ${ACTORS_DIR}/global/message_atoms.cpp
    ${ACTORS_DIR}/global/settings_functions.cpp
    ${ACTORS_DIR}/global/auxiliary.cpp)
set(SUMMA_ACTOR
    ${ACTORS_DIR}/summa_actor/summa_actor.cpp
    ${ACTORS_DIR}/summa_actor/summa_client.cpp
    ${ACTORS_DIR}/summa_actor/summa_server.cpp
    ${ACTORS_DIR}/summa_actor/summa_backup_server.cpp
    ${ACTORS_DIR}/summa_actor/batch/batch.cpp
    ${ACTORS_DIR}/summa_actor/batch/batch_container.cpp
    ${ACTORS_DIR}/summa_actor/client/client.cpp
    ${ACTORS_DIR}/summa_actor/client/client_container.cpp)
set(HRU_ACTOR
    ${ACTORS_DIR}/hru_actor/cpp_code/hru_actor.cpp)
set(GRU_ACTOR
    ${ACTORS_DIR}/gru_actor/gru_actor.cpp)
set(FILE_ACCESS_ACTOR
    ${ACTORS_DIR}/file_access_actor/cpp_code/file_access_actor.cpp
    ${ACTORS_DIR}/file_access_actor/cpp_code/forcing_file_info.cpp
    ${ACTORS_DIR}/file_access_actor/cpp_code/output_container.cpp)
set(JOB_ACTOR
    ${ACTORS_DIR}/job_actor/job_actor.cpp
    ${ACTORS_DIR}/job_actor/GRU.cpp)


#=========================================================================================
# COMPILE PART 3: Collect the subroutines into build groups depending on build type
#=========================================================================================

set(COMM_ALL
    ${NRPROC}
    ${HOOKUP}
    ${DATAMS}
    ${UTILMS})
if(CMAKE_BUILD_TYPE MATCHES Actors)
    set(COMM_ALL ${COMM_ALL}
        ${INTERFACE})
endif()
if(CMAKE_BUILD_TYPE MATCHES Sundials)
    set(COMM_ALL ${COMM_ALL}
        ${DATAMS_SUNDIALS}
        ${UTILMS_SUNDIALS})
endif()

set(SUMMA_ALL
    ${NETCDF}
    ${PRELIM}
    ${MODRUN}
    ${SOLVER}
    ${DRIVER})

if(CMAKE_BUILD_TYPE MATCHES Actors)
    set(SUMMA_ALL ${SUMMA_ALL}
        ${PRELIM_ACTOR}
        ${JOB_INTERFACE}
        ${FILE_ACCESS_INTERFACE}
        ${HRU_INTERFACE}
        ${GRU_INTERFACE})
else()
    set(SUMMA_ALL ${SUMMA_ALL}
        ${SOLVER_NOT_ACTORS}
        ${DRIVER_NOT_ACTORS})
endif()
if(CMAKE_BUILD_TYPE MATCHES NexGen)
    set(SUMMA_ALL ${SUMMA_ALL}
        ${DRIVER_NEXGEN})
endif()
if(CMAKE_BUILD_TYPE MATCHES Sundials)
    set(SUMMA_ALL ${SUMMA_ALL}
        ${MODRUN_SUNDIALS}
        ${SOLVER_SUNDIALS})
endif()

# Run program files
set(MAIN_ACTOR ${ACTORS_DIR}/main.cpp)
#set(MAIN_BMI  ${DRIVER_DIR}/summa_driver4bmi.f90) # do not need a driver in NexGen so do not use
set(MAIN_SUMMA ${DRIVER_DIR}/summa_driver.f90)

# Define version number, not working correctly
set(VERSIONFILE     ${DRIVER_DIR}/summaversion.inc)
execute_process(COMMAND "    ${GIT_EXECUTABLE} tag | tail -n 1" OUTPUT_VARIABLE VERSION)
execute_process(COMMAND "date" OUTPUT_VARIABLE BULTTIM)
execute_process(COMMAND "    ${GIT_EXECUTABLE} describe --long --all --always | sed -e's/heads\///'" OUTPUT_VARIABLE GITBRCH)
execute_process(COMMAND "    ${GIT_EXECUTABLE} rev-parse HEAD" OUTPUT_VARIABLE GITHASH)


#=========================================================================================
# COMPILE PART 4: Do the compilation
#=========================================================================================

# update version information, not working correctly
file(WRITE  ${VERSIONFILE} "character(len=64), parameter     :: summaVersion = '${VERSION}'\n")
file(APPEND ${VERSIONFILE} "character(len=64), parameter     :: buildTime = ''\n")
file(APPEND ${VERSIONFILE} "character(len=64), parameter     :: gitBranch = '${GITBRCH}'\n")
file(APPEND ${VERSIONFILE} "character(len=64), parameter     :: gitHash = '${GITHASH}'")

# Build NOAH_MP Object
add_library(SUMMA_NOAHMP OBJECT ${NOAHMP} ${NRUTIL})
target_compile_options(SUMMA_NOAHMP PRIVATE ${FLAGS_NOAH})
#target_link_options(SUMMA_NOAHMP PRIVATE ${FLAGS_NOAH})

# Build SUMMA_COMM Object
add_library(SUMMA_COMM OBJECT ${COMM_ALL})
target_compile_options(SUMMA_COMM PRIVATE ${FLAGS_ALL})
target_link_options(SUMMA_COMM PRIVATE ${FLAGS_ALL})
target_include_directories(SUMMA_COMM PRIVATE ${INCLUDES})
#target_link_libraries(SUMMA_COMM PUBLIC ${LIBRARIES})

# For NexGen, build SUMMA Shared Library and add the outside BMI libraries
if(CMAKE_BUILD_TYPE MATCHES NexGen)
    if(WIN32)
        add_library(summabmi ${SUMMA_ALL})
    else()
        add_library(summabmi SHARED ${SUMMA_ALL})
    endif()
    target_compile_options(summabmi PRIVATE ${FLAGS_ALL})

    if(CMAKE_BUILD_TYPE MATCHES Kinsol OR CMAKE_BUILD_TYPE MATCHES IDA)
        target_include_directories(summabmi PUBLIC ${INCLUDES} ${INC_SUNDIALS})
        target_link_libraries(summabmi PUBLIC ${LIBRARIES} ${LIB_SUNDIALS} SUMMA_COMM)
    else()
        target_include_directories(summabmi PUBLIC ${INCLUDES})
        target_link_libraries(summabmi PUBLIC ${LIBRARIES} SUMMA_COMM)
    endif()
    target_link_libraries(summabmi PUBLIC iso_c_bmi)
    set_target_properties(summabmi PROPERTIES VERSION ${PROJECT_VERSION})
    include(GNUInstallDirs)

    install(TARGETS summabmi
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    configure_file(summabmi.pc.in summabmi.pc @ONLY)
    install(FILES ${CMAKE_BINARY_DIR}/summabmi.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)

# For Actors, build SUMMA Shared Library add the actors libraries and add the executable
elseif(CMAKE_BUILD_TYPE MATCHES Actors)
    add_library(summaactors SHARED ${SUMMA_ALL})
    target_compile_options(summaactors PRIVATE ${FLAGS_ALL})

    if(CMAKE_BUILD_TYPE MATCHES Sundials)
        target_include_directories(summaactors PUBLIC ${INCLUDES} ${INC_SUNDIALS})
        target_link_libraries(summaactors PUBLIC ${LIBRARIES} ${LIB_SUNDIALS} SUMMA_COMM)
    else()
        target_include_directories(summaactors PUBLIC ${INCLUDES})
        target_link_libraries(summaactors PUBLIC ${LIBRARIES} SUMMA_COMM)
    endif()

    add_executable(${EXEC_NAME} ${MAIN_ACTOR}
        ${ACTORS_GLOBAL}
        ${HRU_ACTOR}
        ${GRU_ACTOR}
        ${FILE_ACCESS_ACTOR}
        ${JOB_ACTOR}
        ${SUMMA_ACTOR}
        ${SUMMA_CLIENT}
        ${SUMMA_SERVER})
    set_property(TARGET ${EXEC_NAME} PROPERTY LINKER_LANGUAGE Fortran)
    target_include_directories(${EXEC_NAME} PUBLIC ${INC_ACTORS})
    target_link_libraries( ${EXEC_NAME} ${LIB_ACTORS})

# For other cases, build SUMMA Shared Library and add the executable
else()
    add_library(summa SHARED ${SUMMA_ALL})
    target_compile_options(summa PRIVATE ${FLAGS_ALL})

    if(CMAKE_BUILD_TYPE MATCHES Sundials)
        target_include_directories(summa PUBLIC ${INCLUDES} ${INC_SUNDIALS})
        target_link_libraries(summa PUBLIC ${LIBRARIES} ${LIB_SUNDIALS} SUMMA_COMM)
    else()
        target_include_directories(summa PUBLIC ${INCLUDES})
        target_link_libraries(summa PUBLIC ${LIBRARIES} SUMMA_COMM)
    endif()

    add_executable(${EXEC_NAME} ${MAIN_SUMMA})
    set_property(TARGET ${EXEC_NAME} PROPERTY LINKER_LANGUAGE Fortran)
    target_link_libraries(${EXEC_NAME} summa)
endif()
