# "sync" will copy in the current build context and sync it, while "no_sync" will not
ARG CORE_BASE_IMAGE=ubuntu:20.10

################################################################################################################
FROM ${CORE_BASE_IMAGE} as get_boost

RUN apt-get update && apt-get install -y wget bzip2 \
    && wget https://boostorg.jfrog.io/artifactory/main/release/1.72.0/source/boost_1_72_0.tar.bz2 \
    && mkdir /boost \
    && mv boost_1_72_0.tar.bz2 /boost/. \
    && cd /boost \
    && tar -xjf boost_1_72_0.tar.bz2 && rm boost_1_72_0.tar.bz2

################################################################################################################
FROM ${CORE_BASE_IMAGE} as clone_repo
ARG NGEN_URL="https://github.com/NOAA-OWP/ngen.git"
ARG NGEN_BRANCH="master"
ARG TOPMODEL_URL="https://github.com/NOAA-OWP/topmodel.git"

RUN apt-get update \
    && apt-get install -y git \
    && rm -rf /var/lib/apt/lists/ \
    && git clone --single-branch --branch $NGEN_BRANCH $NGEN_URL /ngen \
    && cd /ngen \
    && if [ "$(git config --file=.gitmodules -l | grep topmodel | grep url | awk -F= '{print $2}')" != "${TOPMODEL_URL}" ]; then \
            cat .gitmodules | sed "s|\(\s*url =\) [^\s]*NOAA-OWP/topmodel.git|\1 ${TOPMODEL_URL}|" > temp && mv temp .gitmodules; \
       fi \
    && git submodule update --init --recursive

################################################################################################################
FROM ${CORE_BASE_IMAGE} as foundation

RUN apt-get update \
    && apt-get install -y cmake gcc gfortran g++ make python3 python3-dev openmpi-bin libopenmpi-dev gdbserver rsync \
            screen python3-numpy python3-numpy-dbg libnetcdf-mpi-dev libnetcdf-mpi-15 libnetcdf-dev \
            libhdf5-openmpi-fortran-102 libnetcdff-dev libhdf5-openmpi-cpp-103-1 libhdf5-openmpi-dev libopenmpi-dev \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/

################################################################################################################
FROM foundation as sync_prep
ENV UTIL_SCRIPT="./utilities/scripts/debug_container_ops.sh"
ARG BUILD_ENV_SYNC=sync
COPY . /host_ngen
COPY --from=clone_repo /ngen /ngen
RUN cd /ngen \
    && if [ "$BUILD_ENV_SYNC" != "sync" ]; then \
            mkdir -p utilities/scripts; \
            cp /host_ngen/${UTIL_SCRIPT} ${UTIL_SCRIPT}; \
        else \
            /host_ngen/${UTIL_SCRIPT} one_time_sync; \
        fi

################################################################################################################
FROM foundation
ENV UTIL_SCRIPT="./utilities/scripts/debug_container_ops.sh"
ENV BOOST_ROOT /boost/boost_1_72_0
COPY --from=get_boost /boost/boost_1_72_0 /boost/boost_1_72_0
COPY --from=sync_prep /ngen /ngen

WORKDIR /ngen

ARG NUM_PROCS=1

# Use util script for CMake build init to have single places where defaults are setup for CMake flags
# Prep and build the shared libs first, then do a build of the main ngen target
#RUN cd /ngen && ${UTIL_SCRIPT} shared_libs && ${UTIL_SCRIPT} image-cmake && cmake --build /ngen/cmake_build --target ngen -- -j $NUM_PROCS

# Actually, skip main build for now; leave this to inside container to not clog up image build
RUN cd /ngen && ${UTIL_SCRIPT} shared_libs
