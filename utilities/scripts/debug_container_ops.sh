#!/bin/bash

# A utility script intended to be run on a "remote" machine (where at present, this needs to be a Docker container)
# to facilitate remote debugging via gdbserver on the "remote."

# TODO: add usage and about functions

# TODO: formalize either for exclusive container usage, or modify for better generality

# Its expected that, when running script inside a container, this is a mount of the Docker host's ngen directory
# TODO: support parameterizing theses
HOST_SOURCE_CONTAINER_MOUNTS="/host_ngen"
# ... and this is the main operational ngen repo dir where, e.g., things are built
CONTAINER_NGEN_DIR="/ngen"
CONTAINER_CMAKE_FLAGS_PARENT_DIR="${HOST_SOURCE_CONTAINER_MOUNTS}/docker"
CONTAINER_CMAKE_FLAGS_CONFIG="${CONTAINER_CMAKE_FLAGS_PARENT_DIR}/debug_cmake_flags.env"

# The "source" ngen repo dir, in particular when syncing to the "operational" directory (the host mount by default)
SOURCE_NGEN_DIR="${HOST_SOURCE_CONTAINER_MOUNTS}"

# This is the "operational" (or "destination") ngen repo directory, synced to but likely not edited directly
# It is, however, the source for builds (by default, the container ngen dir)
NGEN_DIR="${CONTAINER_NGEN_DIR}"

SYNC_SENTINEL="${SOURCE_NGEN_DIR}/docker/debug_sync_sentinel.txt"
SYNC_LOG="${SOURCE_NGEN_DIR}/docker/log_debug_sync.txt"
RSYNC_RUNNING_SENTINEL="/.sentinel_rsync_running.txt"

check_ok_for_sync()
{
    # Create and remove (or just remove if exists) this to make sure now we can write later
    # This also cleans it up if the last rsync got interrupted somehow
    if [ ! -e ${RSYNC_RUNNING_SENTINEL} ]; then
        touch ${RSYNC_RUNNING_SENTINEL}
    fi
    rm ${RSYNC_RUNNING_SENTINEL}

    if [ ! -d ${SOURCE_NGEN_DIR} ]; then
        >&2 echo "ERROR: bad \"source\" syncing src code dir ${SOURCE_NGEN_DIR} (perhaps not inside container or volume not mounted?)"
        exit 1
    elif [ ! -d ${NGEN_DIR} ]; then
        >&2 echo "ERROR: bad ngen src dir ${NGEN_DIR}"
        exit 1
    elif [ -e ${SYNC_SENTINEL} ]; then
        echo "Code sync currently running in PID $(cat ${SYNC_SENTINEL})."
        exit 0
    fi
}

do_one_sync()
{
    check_ok_for_sync

    # Do this to log a new process started
    echo "${BASHPID}" > ${SYNC_SENTINEL}
    echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" >> ${SYNC_LOG}
    echo "----------------------------------------------------------------------------------------" >> ${SYNC_LOG}
    echo "$(date) " >> ${SYNC_LOG}
    echo "Starting new one-time sync process ${BASHPID} " >> ${SYNC_LOG}
    echo "----------------------------------------------------------------------------------------" >> ${SYNC_LOG}
    echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" >> ${SYNC_LOG}

    touch ${RSYNC_RUNNING_SENTINEL}
    echo "******************************************************************" >> ${SYNC_LOG}
    echo "Syncing at $(date)" >> ${SYNC_LOG}
    echo "******************************************************************" >> ${SYNC_LOG}
    rsync -rltd --stats \
        --exclude=/.git \
        --exclude=/cmake-* \
        --exclude=**cmake_* \
        ${SOURCE_NGEN_DIR}/ ${NGEN_DIR} >> ${SYNC_LOG} 2>&1
    local _R=$?
    rm ${RSYNC_RUNNING_SENTINEL}
    rm ${SYNC_SENTINEL}
    local _R_RM=$?
    return $((_R+_R_RM))
}

do_background_sync()
{
    check_ok_for_sync

    # Do this the first time to log a new process started
    echo "${BASHPID}" > ${SYNC_SENTINEL}
    echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" >> ${SYNC_LOG}
    echo "----------------------------------------------------------------------------------------" >> ${SYNC_LOG}
    echo "$(date) " >> ${SYNC_LOG}
    echo "Starting new sync process ${BASHPID} " >> ${SYNC_LOG}
    echo "----------------------------------------------------------------------------------------" >> ${SYNC_LOG}
    echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" >> ${SYNC_LOG}

    # While the sentinel file exists periodically sync
    while [ -e ${SYNC_SENTINEL} ]; do
        touch ${RSYNC_RUNNING_SENTINEL}
        echo "******************************************************************" >> ${SYNC_LOG}
        echo "Syncing at $(date)" >> ${SYNC_LOG}
        echo "******************************************************************" >> ${SYNC_LOG}
        rsync -rltd --stats \
            --exclude=/.git \
            --exclude=/cmake-* \
            --exclude=**cmake_* \
            ${SOURCE_NGEN_DIR}/ ${NGEN_DIR} >> ${SYNC_LOG} 2>&1
        rm ${RSYNC_RUNNING_SENTINEL}
        sleep 10
    done
}

init_ngen_cmake_flags_config()
{
    local _FLAGS_CFG=${1}
    local _PARENT_DIR=${2}

    if [ ! -e ${_FLAGS_CFG} ]; then
        # Try to create the file with some defaults if it does not already exist but it's directory does
        if [ -d ${_PARENT_DIR} ]; then
            echo "IS_MPI=ON" >> ${_FLAGS_CFG}
            echo "IS_BMI_C=ON" >> ${_FLAGS_CFG}
            echo "IS_BMI_FORTRAN=ON" >> ${_FLAGS_CFG}
            echo "IS_PYTHON=ON" >> ${_FLAGS_CFG}
            echo "IS_ROUTING=OFF" >> ${_FLAGS_CFG}
        else
            >&2 echo "ERROR: cannot create new CMake flags config file ${_FLAGS_CFG}: parent directory does not exist"
            exit 1
        fi
    else
        >&2 echo "WARNING: cannot create new CMake flags config file: ${_FLAGS_CFG} already exists"
    fi
}

init_ngen_build()
{
    local _FLAGS_CONFIG=${1}
    local _FLAGS_CONFIG_PARENT_DIR=${2}
    # Check a first time for the flag config file, to potentially create it
    if [ ! -e ${_FLAGS_CONFIG} ]; then
        init_ngen_cmake_flags_config ${_FLAGS_CONFIG} ${_FLAGS_CONFIG_PARENT_DIR}
    fi
    # Check a second time for the flag config file; it must be here now, or we must error out
    if [ ! -e ${_FLAGS_CONFIG} ]; then
        >&2 echo "ERROR: no available config for initial CMake build flags at path ${_FLAGS_CONFIG}"
        exit 1
    fi
    # As long as the file is there, source it, then init the build
    source ${_FLAGS_CONFIG}
    cmake -B ${NGEN_DIR}/cmake_build \
        -DCMAKE_BUILD_TYPE=Debug \
        -DMPI_ACTIVE:BOOL=${IS_MPI} \
        -DBMI_C_LIB_ACTIVE:BOOL=${IS_BMI_C} \
        -DBMI_FORTRAN_ACTIVE:BOOL=${IS_BMI_FORTRAN} \
        -DNGEN_ACTIVATE_PYTHON:BOOL=${IS_PYTHON} \
        -DNGEN_ACTIVATE_ROUTING:BOOL=${IS_ROUTING} \
        -S ${NGEN_DIR}
}

# Intended for running inside a started container
init_container_ngen_build()
{
    init_ngen_build ${CONTAINER_CMAKE_FLAGS_CONFIG} ${CONTAINER_CMAKE_FLAGS_PARENT_DIR}
}

# Intended for running during an image build (i.e., clean up flags config)
init_image_ngen_build()
{
    init_ngen_build "./.debug_cmake_flags.env" "./"
    rm "./.debug_cmake_flags.env"
}

stop_background_sync()
{
    if [ -e ${SYNC_SENTINEL} ]; then
        if [ -e ${${RSYNC_RUNNING_SENTINEL}} ]; then
            echo "Waiting for current rsync process to finish"
            while [ -e ${${RSYNC_RUNNING_SENTINEL}} ]; do
                sleep 5
            done
        fi
        echo "Stopping sync process $(cat ${SYNC_SENTINEL})"
        echo "Stopping sync process $(cat ${SYNC_SENTINEL})" >> ${SYNC_LOG}
        echo "==================================================================" >> ${SYNC_LOG}
        echo "==================================================================" >> ${SYNC_LOG}
        echo "    " >> ${SYNC_LOG}
        # Check again since we waited
        if [ -e ${SYNC_SENTINEL} ]; then
            rm ${SYNC_SENTINEL}
        else
            >&2 echo "Sync sentinel file ${SYNC_SENTINEL} unexpectedly removed during wait for last rsync"
            echo "Sync sentinel file ${SYNC_SENTINEL} unexpectedly removed during wait for last rsync" >> ${SYNC_LOG}
        fi
    else
        >&2 echo "ERROR: No sync sentinel file ${SYNC_SENTINEL} found; sync must not be running."
        exit 1
    fi
}

try_background_sync()
{
    echo "Starting background sync process"
    do_background_sync &
    _T=0
    while [ ${_T} -lt 10 ]; do
        if [ -e ${SYNC_SENTINEL} ]; then
            echo "Sync PID: $(cat ${SYNC_SENTINEL})"
            exit 0
        else
            _T=$((_T + 1))
            sleep 1
        fi
    done
    >&2 echo "ERROR: exiting after background sync process took too long to start"
    exit 1
}

cmake_and_build_shared_lib_dir()
{

    if [ ! -d ${1:?ERROR: no shared lib directory passed to init and build} ]; then
        >&2 echo "ERROR: given shared lib ${1} is not an existing directory."
        exit 1
    fi

    if [ ! -f ${1}/CMakeLists.txt ]; then
        >&2 echo "ERROR: ${1} does not appear to be a shared lib directory."
        exit 1
    fi

    #echo "Initializing and building shared library at ${1}"
    cmake -B ${1:?}/cmake_build -S ${1:?}
    cmake --build ${1:?}/cmake_build ${_TARGET_STRING:-}
    #echo "Built shared library for ${1:?}"
}

do_shared_libs()
{
    for d in $(find extern -type d -maxdepth 1); do
        # Skip this one
        if [ "${d}" == "extern/pybind11" ]; then
            continue
        fi
         test
        if [ -f ${d}/CMakeLists.txt ]; then
            cmake_and_build_shared_lib_dir ${d}
        fi
    done
}


case "${1}" in
    sync|-s)
        try_background_sync
        # Putting this exit here, but current function will already always exit on its own.
        exit 0
        ;;
    one_time_sync|-o)
        do_one_sync
        exit $?
        ;;
    stop-sync|-S)
        stop_background_sync
        exit $?
        ;;
    init-build|init-cmake|cmake|-b)
        init_container_ngen_build
        exit $?
        ;;
    shared_libs)
        do_shared_libs
        exit $?
        ;;
    image-cmake)
        init_image_ngen_build
        exit $?
        ;;
    *)
        >&2 echo "ERROR: unsupported action '${1}'"
        exit 1
        ;;
esac
