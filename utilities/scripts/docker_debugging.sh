#!/bin/bash

NAME="`basename ${0}`"
INFO='Perform tasks to debug ngen using a Docker container as the environment for
building, executing, and attaching a debug server.'

CONTAINER_REPO_DIR="/ngen"
CONTAINER_HOST_MOUNT_REPO_DIR="/host_ngen"
CONTAINER_HOST_MOUNT_DATA_DIR="/test_data"

DEFAULT_DOCKER_IMAGE="$(whoami)_dev_debug_ubuntu:latest"
CONTAINER_NAME="$(whoami)_ngen_dev_debug"

CONTAINER_DEBUG_UTIL="${CONTAINER_HOST_MOUNT_REPO_DIR}/utilities/scripts/debug_container_ops.sh"

# Note that this has to be consisten with the driver code
PID_FILE_BASE=".ngen_pid"
GDB_WAIT_FILE=".ngen_gdb.wait"
START_PORT=9800

NGEN_BUILD_DIR_NAME="cmake_build"

set_dev_ptrace_for_gdbserver()
{
    # Need to do this to support gdbserver attaching to processes
    docker exec --privileged ${CONTAINER_NAME} /bin/bash -c 'echo 0 > /proc/sys/kernel/yama/ptrace_scope'
}

# Look for PID file(s) produced by ngen, either named PID_FILE_BASE or named PID_FILE_BASE.<rank> (e.g., .ngen_pid.0),
# and have gdbserver within the container attach to the PID within each on the next available gdbserver port.
#
# Also delete each PID file after its process is attach to gdbserver.
#
# Note that specific ports accessible to the host are limited and controlled by the create_debug_container function.
setup_container_gdbserver()
{
    local _CONTAINER=${1}
    local _PORT=${START_PORT}

    if [ $(docker ps -q -f "name=${CONTAINER_NAME}" | wc -l) -eq 0 ]; then
        >&2 echo "ERROR: no container '${CONTAINER_NAME}' is currently running."
        exit 1
    fi

    # Wait until files show up
    while [ $(docker exec ${CONTAINER_NAME} ls -a ${CONTAINER_REPO_DIR} | grep ${PID_FILE_BASE} | wc -l) \
            -lt \
            ${NUM_PROCS:?ERROR: number of expected processes not provided but required for gdbserver setup} ]; do
        sleep 5
    done

    # First start all the gdbserver instances
    for basename in $(docker exec ${CONTAINER_NAME} ls -a ${CONTAINER_REPO_DIR} | grep ${PID_FILE_BASE}); do
        _PID=$(docker exec ${CONTAINER_NAME} cat ${CONTAINER_REPO_DIR}/${basename})
        echo "${_PID}"
        docker exec --privileged -d ${CONTAINER_NAME} gdbserver --attach :${_PORT} ${_PID}
        _PORT=$((_PORT + 1))
    done

    # Then delete the PID files to let the Nextgen driver know it should proceed
    for basename in $(docker exec ${CONTAINER_NAME} ls -a ${CONTAINER_REPO_DIR} | grep ${PID_FILE_BASE}); do
        docker exec ${CONTAINER_NAME} rm ${CONTAINER_REPO_DIR}/${basename}
    done
}

# Remove the final wait file that indicates to the driver  it should (temporarily) block while the remote debugging
# setup is initialized.
#
# The idea is for the driver to allow time (separately from the time it allows for gdbserver in the container is
# attached to all desired driver processes) for all required remote debugging connections to be made from a development
# machine to the container.  There is a timeout that will eventually happen, but this lets a user signal when things are
# ready, so that the driver may proceed without waiting excessively.
gdb_start()
{
    if [ $(docker ps -q -f "name=${CONTAINER_NAME}" | wc -l) -eq 0 ]; then
        >&2 echo "ERROR: no container '${CONTAINER_NAME}' is currently running."
        exit 1
    fi

    if [ $(docker exec ${CONTAINER_NAME} ls -a ${GDB_WAIT_FILE} 2>/dev/null | wc -l) -eq 0 ]; then
        >&2 echo "WARNING: expected wait-file for GDB '${GDB_WAIT_FILE}' not found - make sure debug setup is initialized"
    else
        docker exec ${CONTAINER_NAME} rm ${GDB_WAIT_FILE} 2>/dev/null
    fi
}

host_code_sync()
{
    if [ $(docker ps -q -f "name=${CONTAINER_NAME}" | wc -l) -eq 0 ]; then
        >&2 echo "ERROR: no container '${CONTAINER_NAME}' is currently running."
        exit 1
    fi

    docker exec ${CONTAINER_NAME} test -e ${CONTAINER_DEBUG_UTIL:?ERROR: container debug util not configured}
    if [ $? -ne 0 ]; then
        >&2 echo -n "ERROR: container debug util '${CONTAINER_DEBUG_UTIL}' not found; "
        >&2 echo "container likely was not set up with host repo bind mount."
        exit 1
    fi

    docker exec ${CONTAINER_NAME} ${CONTAINER_DEBUG_UTIL} one_time_sync
}

refresh_cmake()
{
    if [ $(docker ps -q -f "name=${CONTAINER_NAME}" | wc -l) -eq 0 ]; then
        >&2 echo "ERROR: no container '${CONTAINER_NAME}' is currently running."
        exit 1
    fi

    set_dev_ptrace_for_gdbserver

    docker exec --privileged -t ${CONTAINER_NAME} ${CONTAINER_DEBUG_UTIL} init-cmake
}

build_ngen()
{
    if [ $(docker ps -q -f "name=${CONTAINER_NAME}" | wc -l) -eq 0 ]; then
        >&2 echo "ERROR: no container '${CONTAINER_NAME}' is currently running."
        exit 1
    fi

    set_dev_ptrace_for_gdbserver

    docker exec --privileged -t ${CONTAINER_NAME} \
            cmake --build ${CONTAINER_REPO_DIR}/${NGEN_BUILD_DIR_NAME} \
                  --target ${CMAKE_TARGET:-ngen} \
                  -- -j ${NUM_COMPILER_JOBS:-2}
}

ngen_mpi_run()
{
    local _NUM_PROCS=${1:?}
    local _CONTAINER_CATCHMENT_DATA=${2:?}
    local _CONTAINER_NEXUS_DATA=${3:?}
    local _CONTAINER_REALIZATION_CONFIG=${4:?}
    local _CONTAINER_PARTITION_CONFIG=${5:?}

    if [ $(docker ps -q -f "name=${CONTAINER_NAME}" | wc -l) -eq 0 ]; then
        >&2 echo "ERROR: no container '${CONTAINER_NAME}' is currently running."
        exit 1
    fi

    # Build things if they haven't been built yet
    docker exec ${CONTAINER_NAME} test -d ${CONTAINER_REPO_DIR}/${NGEN_BUILD_DIR_NAME}
    if [ $? -ne 0 ]; then
        refresh_cmake
        build_ngen
    fi

    set_dev_ptrace_for_gdbserver

    docker exec --privileged ${CONTAINER_NAME} mpirun --allow-run-as-root \
            -np ${_NUM_PROCS} \
            ${CONTAINER_REPO_DIR}/${NGEN_BUILD_DIR_NAME}/ngen \
            ${_CONTAINER_CATCHMENT_DATA} \
            "" \
            ${_CONTAINER_NEXUS_DATA} \
            "" \
            ${_CONTAINER_REALIZATION_CONFIG} \
            ${_CONTAINER_PARTITION_CONFIG} \
            --subdivided-hydrofabric
}

run_sugar_creek_2()
{
    ngen_mpi_run \
        2 \
        ${CONTAINER_HOST_MOUNT_DATA_DIR}/hydrofabric/sugar_creek/catchment_data.geojson \
        ${CONTAINER_HOST_MOUNT_DATA_DIR}/hydrofabric/sugar_creek/nexus_data.geojson \
        ${CONTAINER_HOST_MOUNT_DATA_DIR}/config/realization_config/sugar_creek_simple/simple_conf.json \
        ${CONTAINER_HOST_MOUNT_DATA_DIR}/partitions/sugar_creek/sugar_creek_partitions_2.json
}

run_sugar_creek_8()
{
    ngen_mpi_run \
        8 \
        ${CONTAINER_HOST_MOUNT_DATA_DIR}/hydrofabric/sugar_creek/catchment_data.geojson \
        ${CONTAINER_HOST_MOUNT_DATA_DIR}/hydrofabric/sugar_creek/nexus_data.geojson \
        ${CONTAINER_HOST_MOUNT_DATA_DIR}/config/realization_config/sugar_creek_simple/simple_conf.json \
        ${CONTAINER_HOST_MOUNT_DATA_DIR}/partitions/sugar_creek/sugar_creek_partitions_8.json
}

# Build new "latest" image
build_docker_image()
{
    # First, check if any running container uses the image; bail if something is running
    if [ $(docker ps -q -f ancestor=${DEFAULT_DOCKER_IMAGE} | wc -l) -gt 0 ]; then
        >&2 echo "ERROR: a container is currently running using the existing image ${DEFAULT_DOCKER_IMAGE}"
        exit 1
    # Then check if any stopped container uses the image; remove it if so
    elif [ $(docker ps -a -q -f ancestor=${DEFAULT_DOCKER_IMAGE} | wc -l) -gt 0 ]; then
        docker rm $(docker ps -a -q -f ancestor=${DEFAULT_DOCKER_IMAGE})
    fi

    if [ $(docker images -q ${DEFAULT_DOCKER_IMAGE} | wc -l) -gt 0 ]; then
        echo "Removing previous Docker image ${DEFAULT_DOCKER_IMAGE} before rebuilding"
        docker rmi $(docker images -q ${DEFAULT_DOCKER_IMAGE})
    fi

    docker build \
        --tag ${DEFAULT_DOCKER_IMAGE} \
        -f ${HOST_NGEN_DIR:?}/docker/dev_debug_ubuntu.Dockerfile \
        --build-arg "BUILD_ENV_SYNC=${SYNC_IMAGE_NGEN_REPO_BUILD_ARG:-sync} NUM_PROCS=${NUM_PROCS:-1}" \
        ${HOST_NGEN_DIR:?}

    _R=$?
    if [ ${_R} -ne 0 ]; then
        >&2 echo "Exiting after new ngen debug image ${DEFAULT_DOCKER_IMAGE} failed to build."
        exit ${_R}
    fi

    if [ -n "${DOCKER_IMAGE:-}" ]; then
        docker tag ${DEFAULT_DOCKER_IMAGE} ${DOCKER_IMAGE}
    fi
}

# Create a new debugging container from the Docker image named in the DOCKER_IMAGE variable (or default if not set).
# Make available <num_processes> ports starting at 9800 on the host, intended for use with gdbserver and attached processes.
create_debug_container()
{
    if [ $(docker ps -q -f "name=${CONTAINER_NAME}" | wc -l) -eq 1 ]; then
        >&2 echo "ERROR: container with name '${CONTAINER_NAME}' is already running"
        exit 1
    elif [ $(docker ps -q -a -f "name=${CONTAINER_NAME}" | wc -l) -eq 1 ]; then
        >&2 echo "ERROR: container with name '${CONTAINER_NAME}' is stopped but exists"
        exit 1
    fi

    local _IMAGE="${DOCKER_IMAGE:-${DEFAULT_DOCKER_IMAGE}}"

    # If the requested image doesn't exist ...
    if [ $(docker images -q ${_IMAGE} | wc -l) -eq 0 ]; then
        # ... but the default does (which implies they are different) ...
        if [ $(docker images -q ${DEFAULT_DOCKER_IMAGE} | wc -l) -gt 0 ]; then
            >&2 echo "ERROR: no image ${_IMAGE} exists for new container, but creating would require erasing default ${DEFAULT_DOCKER_IMAGE}"
            exit 1
        else
            build_docker_image
        fi
    fi

    # Dynamically build the ports arguments string
    local _PORT_COUNT=0
    local _PORT_ARG_STRING=""
    local _CURRENT_PORT=${START_PORT}
    while [ ${_PORT_COUNT} -lt ${NUM_PROCS:-8} ]; do
        _CURRENT_PORT=$((START_PORT+_PORT_COUNT))
        _PORT_ARG_STRING="${_PORT_ARG_STRING} -p ${_CURRENT_PORT}:${_CURRENT_PORT}"
        _PORT_COUNT=$((_PORT_COUNT+1))
    done

    # Dynamically build the host bind-mount volumes arguments string
    local _VOLS_ARG_STRING=""
    # Add the host ngen repo directory, unless explicitly set for no_code_sync
    if [ -z "${NO_CODE_SYNC:-}" ]; then
        #_VOLS_ARG_STRING=" -v ${HOST_NGEN_DIR}:${CONTAINER_HOST_MOUNT_REPO_DIR}"
        _VOLS_ARG_STRING=" --mount type=bind,source=${HOST_NGEN_DIR},target=${CONTAINER_HOST_MOUNT_REPO_DIR}"
    fi

    if [ -z "${HOST_DATA_DIR:-}" ] && [ -d "${HOST_DATA_DIR}" ]; then
        #_VOLS_ARG_STRING="${_VOLS_ARG_STRING} -v ${HOST_DATA_DIR}:${CONTAINER_HOST_MOUNT_DATA_DIR}"
        _VOLS_ARG_STRING="${_VOLS_ARG_STRING} --mount type=bind,source=${HOST_DATA_DIR},target=${CONTAINER_HOST_MOUNT_DATA_DIR},readonly"
    fi
    # Assume 'else' case is handled manually somehow; i.e., something else is getting data/configs in the base image

    # TODO: add support later for individually configured directories
        # TODO: Add the hydrofabric directory
        # TODO: Add the realization config directory
        # TODO: Add the partition config directory, if not in the same directory as realization config
        # TODO: Add (root) dir for BMI init configs and any transitively required files (if not realization config dir)
        # TODO: Add the forcing data files directory

    #${CONTAINER_HOST_MOUNT_DATA_DIR}/hydrofabric/sugar_creek/catchment_data.geojson \
    #        ${CONTAINER_HOST_MOUNT_DATA_DIR}/hydrofabric/sugar_creek/nexus_data.geojson \
    #        ${CONTAINER_HOST_MOUNT_DATA_DIR}/config/realization_config/sugar_creek_simple/simple_conf.json \
    #        ${CONTAINER_HOST_MOUNT_DATA_DIR}/partitions/sugar_creek/sugar_creek_partitions_2.json

    docker run --privileged ${_VOLS_ARG_STRING} ${_PORT_ARG_STRING} -t -i --name ${CONTAINER_NAME} ${_IMAGE} /bin/bash
}

start_debug_container()
{
    if [ $(docker ps -q -f "name=${CONTAINER_NAME}" | wc -l) -eq 1 ]; then
        >&2 echo "ERROR: container with name '${CONTAINER_NAME}' is already running"
        exit 1
    elif [ $(docker ps -q -a -f "name=${CONTAINER_NAME}" | wc -l) -eq 1 ]; then
        docker start -a -i ${CONTAINER_NAME}
    else
        create_debug_container
    fi
}

cmake_and_build_shared_lib_dir()
{
    if [ $(docker ps -q -f "name=${CONTAINER_NAME}" | wc -l) -eq 0 ]; then
        >&2 echo "ERROR: no container '${CONTAINER_NAME}' is currently running."
        exit 1
    fi

    docker exec ${CONTAINER_NAME} test -d ${1:?ERROR: no shared lib directory passed to init and build}
    if [ $? -ne 0 ]; then
        >&2 echo "ERROR: given shared lib ${1} is not an existing directory inside the container."
        exit 1
    fi

    docker exec ${CONTAINER_NAME} test -f ${1:?}/CMakeLists.txt
    if [ $? -ne 0 ]; then
        >&2 echo "ERROR: ${1} does not appear to be a shared lib directory."
        exit 1
    fi

    echo "Initializing and building shared library at ${1:?}"
    docker exec ${CONTAINER_NAME} cmake -B ${1:?}/cmake_build -S ${1:?}
    docker exec ${CONTAINER_NAME} cmake --build ${1:?}/cmake_build ${_TARGET_STRING:-}
    echo "Built shared library for ${1:?}"
}

init_shared_libs()
{
    if [ $(docker ps -q -f "name=${CONTAINER_NAME}" | wc -l) -eq 0 ]; then
        >&2 echo "ERROR: no container '${CONTAINER_NAME}' is currently running."
        exit 1
    fi

    # Just do the one shared lib
    if [ -n "${SHARED_LIB_DIR:-}" ]; then
        cmake_and_build_shared_lib_dir ${SHARED_LIB_DIR}
    else
        for d in $(docker exec ${CONTAINER_NAME} find extern -type d -maxdepth 1); do
            # Skip this one
            if [ "${d}" == "extern/pybind11" ]; then
                continue
            fi
            docker exec ${CONTAINER_NAME} test -f ${d}/CMakeLists.txt
            if [ $? -eq 0 ]; then
                cmake_and_build_shared_lib_dir ${d}
            fi
        done
    fi
}

usage()
{
    local _O="
${INFO:?}

Usage:
    ${NAME:?} -h|-help|--help       Display simple help message.
    ${NAME:?} -hh|--full-help       Display detailed help message.
    ${NAME:?} [options] <action>    Execute given debugging utility action.

Options:
    --container-name|-n <name>      Specify debug container name.
    --docker-image|-i <name>        Specify (additional) image name to use.
    --num-procs|-N <num>            Set number of exec processes.
    --num-compiler-jobs|-j <num>    Set number of jobs for CMake build actions.
    --cmake-target|-t <target>      Set target for CMake build actions.
    --lib-dir <dir>                 Specify single shared lib dir for building.
    --ngen-dir|-d <path>            Set host ngen dir when creating container.
    --no-code-sync                  Skip host code sync (image or container).
    --data-dir|-D <path>            Set host data dir when creating container.

Actions:
    build                           (Re)build ngen inside container.
    refresh_cmake | cmake           (Re)init CMake build dir inside container.
    new_container | new             Create and start a new debug container.
    new_image | build_docker        Build new debug container Docker image.
    setup_gdbserver|gdbserver|gdb   Attach to PIDs with gdbserver in container.
    gdb_start | gdb_go | go         Signal to container GDB connections ready.
    shared_libs                     Initialize and build shared lib(s).
    start_container | start         Start a debug container.
    sugar_creek_2 | sc2             Run ngen in container using these presets.
    sugar_creek_8 | sc8             Run ngen in container using these presets.
    sync                            Sync host code changes inside container.

Note that starting a container (new or existing) will result in the current
terminal attaching to it and provide a direct shell within the container.

See full help (-hh) for more detailed explanations.
"
    echo "${_O}" 2>&1
}

long_usage()
{
    local _O="
${INFO:?}

Usage:
    ${NAME:?} -h|-help|--help       Display simple help message.
    ${NAME:?} -hh|--full-help       Display detailed help message.
    ${NAME:?} [options] <action>    Execute given debugging utility action.

Options:
    --container-name|-n <name>
        Specify the name for the Docker container to create, start, or use.
        (Default: ${CONTAINER_NAME})

    --docker-image|-i <name>
        Specify the name to use for Docker image.  When creating a debug
        container, this controls the base image that is used.  When creating
        a new Docker image, this is used to create an additional name/tag for
        the image, though the default behavior of initially creating with the
        default tag still applies.

    --num-procs|-N <num>
        Specify the expected number of processes when performing certain
        actions, such as gdbserver setup and process attachment (typically,
        this is required for the actions that utilize it; otherwise it is
        ignored).

    --num-compiler-jobs|-j <num>
        Specify the number of compiler jobs when (re)build ngen inside the
        container. (Default: 2)
        ***********************************************************************
        * Important Note: higher values will be faster, but compiler errors   *
        * have been seen in some environments in situations when otherwise    *
        * they would have been expected to work (e.g., -j 4 in a container    *
        * with 4 CPUs available to it).  Try reducing this value if you see   *
        * unexpected compiler errors.                                         *
        ***********************************************************************

    --cmake-target|-t <target>
        Specify a particular CMake target when running **any** action that
        performs a CMake build (e.g., 'build').

    -lib-dir <dir>
        Specify a single shared lib dir when initializing and building shared
        libraries.

    --ngen-dir|-d <path>
        Specify the Nextgen repo directory on this host; this is used when
        creating a new container to configure a volume mount of the external
        host's code inside the container. (Default: current working directory)

        Note that while the value set here will affect the 'sync' action, it
        can only be set when creating a new container.

    --no-code-sync
        Skip the steps needed to sync with the host ngen repo when creating
        either a new container or new image.  For images, this adjusts steps
        during the Docker build.  For creating containers, this omits the args
        required to set up host directory volume mounts inside the container.

    --data-dir|-D <path>
        Specify the data directory on this host, which is volume mounted inside
        the created container at:

        ${CONTAINER_HOST_MOUNT_DATA_DIR}

        ***********************************************************************
        * Important Note: the directory structure of the data directory must  *
        * align with paths any used preset jobs (e.g. 'sugar_creek_2') and    *
        * the applicable contents of configuration files when such files      *
        * provide a file path.                                                *
        ***********************************************************************

Actions:
    build
        Perform a CMake (re)build of the Nextgen source inside the container.
        See above --cmake-target option for controlling the target.
        (Default: ngen)

    refresh_cmake | cmake
        Perform a re-initialization of the Nextgen CMake build directory inside
        the container.

        Note that this uses the debug container operations script, executing
        it inside the container. The script in turn uses values defined in an
        env file for various CMake flags (creating this env file using defaults
        if it does not exists).  The env file's path is inside the container's
        volume mount of the host's repo, and can thus be viewed and/or edited
        on the host before performing a refresh.

        The current path to the flags env file is: docker/debug_cmake_flags.env

    shared_libs
        Inside the debug container (re)initialize the CMake build directory and
        and (re)build one or more shared libraries.  Libraries are expected to
        be in directories under extern/ and have a CMakeLists.txt file.  All
        generated build directories are named extern/<lib_dir>/cmake_build/.

        If a particular shared library directory is supplied via the supported
        option, only that one is affected.  Otherwise, the CMake init and build
        are performed under every directory within extern/ that contains a
        CMakeLists.txt file, with the exception of extern/pybind11/ (note that
        this exception does not apply if it is specifically passed via the
        aforementioned option).

        Note also that if the option to specify a particular CMake target is
        provided, it will be respected by this action and applied to any and
        all performed builds.

    new_container | new
        Create and run a new debug container, removing any existing one.

        As with starting an existing container, this will attach to the
        container and provide an interactive bash shell.

    new_image | build_docker
        Build a new image for creating debug containers.  Before building, this
        will remove the image at the default value of ${DEFAULT_DOCKER_IMAGE},
        if there is one.  It then builds a new image and tags it there.

        If an option is given to provided the Docker image name, then an
        another tag is created using this provided name.  However, even in this
        case, the above actions involving the default image name still apply.

    gdbserver | gdb | setup_gdbserver
        Using PID files created individually by a number of driver processes
        already running inside the debug container, use gdbserver in the
        container to attach to each aforementioned process. Also removing each
        PID file once the corresponding process has been attached.

    gdb_start | gdb_go | go
        Indicate to driver processes that all remote debug connections are
        ready and that the driver should proceed with execution.

        A timed wait exists in the driver to allow for remote connections
        from the host to gdbserver in the debugging container.  This action
        signals to the processes that the wait can be ended early by removing
        a sentinel file within the container.

    start_container | start
        Start the debug container, creating it first if necessary.

        This will also attach to the container and provide an interactive bash
        shell.

    sugar_creek_2 | sc2
        Execute the driver inside the debug container using mpirun and preset
        files for Sugar Creek with 2 partitions.

    sugar_creek_8|sc8)
        Execute the driver inside the debug container using mpirun and preset
        files for Sugar Creek with 8 partitions.

    sync
        Sync the debug container's repo against the host's current repo, via
        the volume mount of the latter in the container.

        Note that this uses the debug container operations script action for
        performing a one-time sync, executing the script inside the container
        via 'docker exec'.  Not all files are synced (e.g., CMake build
        directories and git files are ignored).  See that script for how this
        is configured.
"
    echo "${_O}" 2>&1
}

# Get any options
while [ ${#} -gt 1 ]; do
    case "${1}" in
        --container-name|-n)
            CONTAINER_NAME=${2}
            shift
            ;;
        --docker-image|-i)
            DOCKER_IMAGE=${2}
            shift
            ;;
        --num-procs|-N)
            NUM_PROCS=${2}
            shift
            ;;
        --num-compiler-jobs|-j)
            NUM_COMPILER_JOBS=${2}
            shift
            ;;
        --cmake-target|-t)
            CMAKE_TARGET=${2}
            _TARGET_STRING="--target ${CMAKE_TARGET}"
            shift
            ;;
        --lib-dir)
            SHARED_LIB_DIR=${2}
            shift
            ;;
        --no-code-sync)
            NO_CODE_SYNC="true"
            SYNC_IMAGE_NGEN_REPO_BUILD_ARG="no_sync"
            ;;
        --ngen-dir|-d)
            HOST_NGEN_DIR=${2}
            shift
            ;;
        --data-dir|-D)
            HOST_DATA_DIR=${2}
            shift
            ;;
        --help|-help|-h|help)
            usage
            exit
            ;;
        --full-help|-hh)
            long_usage
            exit
            ;;
        *)
            >&2 echo "ERROR: unrecognized option '${1}'"
            exit 1
            ;;
    esac
    shift
done

if [ -z ${HOST_NGEN_DIR:-} ]; then
    # Assume current directory if there is a good data/ subdirectory
    if [ -d $(pwd)/data ]; then
        HOST_NGEN_DIR=$(pwd)
    else
        >&2 echo "ERROR: no host Nextgen directory provided, and working dir does not appear to be valid"
        exit 1
    fi
fi

if [ -z ${HOST_DATA_DIR:-} ]; then
    HOST_DATA_DIR="${HOST_NGEN_DIR}/data"
fi

# TODO: function (and opts) for fully manually supplying a mpirun execution, rather than using a preset

# The parse and execute action
case "${1}" in
    --full-help|-hh)
        long_usage
        exit
        ;;
    --help|-help|-h|help)
        usage
        exit
        ;;
    shared_libs)
        init_shared_libs
        exit $?
        ;;
    sugar_creek_2|sc2)
        run_sugar_creek_2
        exit $?
        ;;
    sugar_creek_8|sc8)
        run_sugar_creek_8
        exit $?
        ;;
    build)
        build_ngen
        exit $?
        ;;
    new_container|new)
        if [ $(docker ps -q -a -f "name=${CONTAINER_NAME}" | wc -l) -gt 0 ]; then
            echo "Removing existing '${CONTAINER_NAME}' container"
            docker rm ${CONTAINER_NAME} >/dev/null && create_debug_container
        else
            create_debug_container
        fi
        exit $?
        ;;
    new_image|build_docker)
        build_docker_image
        exit $?
        ;;
    start_container|start)
        start_debug_container
        ;;
    refresh_cmake|cmake)
        refresh_cmake
        exit $?
        ;;
    sync)
        host_code_sync
        exit $?
        ;;
    setup_gdbserver|gdbserver|gdb)
        setup_container_gdbserver
        exit $?
        ;;
    gdb_start|gdb_go|go)
        gdb_start
        exit $?
        ;;
    *)
        >&2 echo "ERROR: unsupported action '${1}'"
        exit 1
        ;;
esac