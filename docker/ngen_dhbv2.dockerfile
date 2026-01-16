# Build ngen with all dependencies and dhbv2 on Rocky Linux 9.1 + Python3.9.
#
# Adapted from
# - noaa-ngen/docker/CENTOS_NGEN_RUN.dockerfile
# - ciroh-ua/NGIAB-CloudInfra/docker/Dockerfile
# 
# Uses CIROH-UA T-Route fork for compatibility.
ARG NPROC=8
ARG NGEN_REPO=mhpi/ngen
ARG NGEN_BRANCH=dhbv2
ARG TROUTE_REPO=CIROH-UA/t-route
ARG TROUTE_BRANCH=ngiab


# Stage 1: Base image with dependencies
FROM rockylinux:9.1 AS base

RUN echo "max_parallel_downloads=10" >> /etc/dnf/dnf.conf
RUN dnf update -y \
    && dnf install -y epel-release \
    && dnf config-manager --set-enabled crb \
    && dnf install -y \
        vim libgfortran sqlite bzip2 expat udunits2 zlib mpich hdf5 \
        netcdf netcdf-fortran netcdf-cxx netcdf-cxx4-mpich \
    && dnf clean all

# Install astral-uv
ENV PATH="/root/.cargo/bin:${PATH}"
ENV UV_INSTALL_DIR=/root/.cargo/bin
ENV UV_COMPILE_BYTECODE=1
RUN curl -LsSf https://astral.sh/uv/install.sh | sh
RUN uv self update


# Stage 2: Build base with development tools
FROM base AS dependencies

RUN echo "max_parallel_downloads=10" >> /etc/dnf/dnf.conf
RUN dnf install -y epel-release \
    && dnf config-manager --set-enabled crb \
    && dnf install -y \
        sudo tar wget git \
        gcc gcc-c++ make cmake \
        ninja-build \
        gcc-gfortran libgfortran \
        sqlite sqlite-devel \
        python3 python3-devel python3-pip \
        expat-devel \
        flex \
        bison \
        udunits2-devel \
        zlib-devel \
        mpich-devel \
        netcdf-devel hdf5-devel netcdf-fortran-devel netcdf-cxx-devel \
        lld \
        clang \
    && dnf clean all


# Stage 3: Boost setup
FROM dependencies AS boost_setup

RUN curl -L -o boost_1_86_0.tar.bz2 https://sourceforge.net/projects/boost/files/boost/1.86.0/boost_1_86_0.tar.bz2/download \
    && tar -xjf boost_1_86_0.tar.bz2 \
    && rm boost_1_86_0.tar.bz2
ENV BOOST_ROOT=/boost_1_86_0


# Stage 4: Install T-Route dependencies
FROM boost_setup AS troute_prebuild
ARG TROUTE_REPO
ARG TROUTE_BRANCH

WORKDIR /ngen

# Troute looks for netcdf.mod in the wrong place unless we set this
ENV FC=gfortran NETCDF=/usr/lib64/gfortran/modules/
RUN ln -s /usr/bin/python3 /usr/bin/python

WORKDIR /ngen/
RUN uv venv
ENV PATH="/ngen/.venv/bin:$PATH"

# make sure clone isn't cached if repo is updated
ADD https://api.github.com/repos/${TROUTE_REPO}/git/refs/heads/${TROUTE_BRANCH} /tmp/version.json

RUN uv pip install -r https://raw.githubusercontent.com/${TROUTE_REPO}/refs/heads/${TROUTE_BRANCH}/requirements.txt


# Stage 5: Build T-Route
FROM troute_prebuild AS troute_build
ARG TROUTE_REPO
ARG TROUTE_BRANCH
ARG NPROC

WORKDIR /ngen/t-route

# Clone and init all submodules
RUN git clone --depth 1 --single-branch -b ${TROUTE_BRANCH} https://github.com/${TROUTE_REPO}.git . \
    && git submodule update --init --recursive --jobs ${NPROC} --depth 1 \
    && uv pip install build wheel

# Disable everything except the kernel builds
RUN sed -i 's/build_[a-z]*=/#&/' compiler.sh

RUN ./compiler.sh no-e

RUN uv pip install --config-setting='--build-option=--use-cython' src/troute-network/
RUN uv build --wheel --config-setting='--build-option=--use-cython' src/troute-network/
RUN uv pip install --no-build-isolation --config-setting='--build-option=--use-cython' src/troute-routing/
RUN uv build --wheel --no-build-isolation --config-setting='--build-option=--use-cython' src/troute-routing/
RUN uv build --wheel --no-build-isolation src/troute-config/
RUN uv build --wheel --no-build-isolation src/troute-nwm/


# Stage 6: Clone ngen
FROM troute_prebuild AS ngen_clone
ARG NGEN_REPO
ARG NGEN_BRANCH
ARG NPROC

WORKDIR /ngen

# Clone and init all submodules
ADD https://api.github.com/repos/${NGEN_REPO}/git/refs/heads/${NGEN_BRANCH} /tmp/version.json
RUN git clone --depth 1 --single-branch -b ${NGEN_BRANCH} https://github.com/${NGEN_REPO}.git \
    && cd ngen \
    && git submodule update --init --recursive --jobs ${NPROC} --depth 1 
    # && git submodule update --init --recursive --remote --jobs ${NPROC} --depth 1 


# Stage 7: Build ngen
FROM ngen_clone AS ngen_build
ARG NPROC

ENV VIRTUAL_ENV=/ngen/.venv
ENV PATH="/ngen/.venv/bin:$PATH"
ENV PATH=${PATH}:/usr/lib64/mpich/bin

WORKDIR /ngen/ngen

ARG COMMON_BUILD_ARGS=" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=. \
    \
    -DNGEN_IS_MAIN_PROJECT=ON \
    -DNGEN_WITH_NETCDF:BOOL=ON \
    -DNGEN_WITH_SQLITE:BOOL=ON \
    -DNGEN_WITH_UDUNITS:BOOL=ON \
    -DNGEN_WITH_BMI_FORTRAN:BOOL=ON \
    -DNGEN_WITH_BMI_C:BOOL=ON \
    -DNGEN_WITH_PYTHON:BOOL=ON \
    -DNGEN_WITH_ROUTING:BOOL=ON \
    -DNGEN_WITH_TESTS:BOOL=ON \
    -DNGEN_QUIET:BOOL=ON \
    -DUDUNITS_QUIET:BOOL=ON \
    -DNGEN_UPDATE_GIT_SUBMODULES:BOOL=OFF \
    \
    -DNGEN_WITH_EXTERN_SLOTH:BOOL=ON \
    -DNGEN_WITH_EXTERN_TOPMODEL:BOOL=ON \
    -DNGEN_WITH_EXTERN_CFE:BOOL=ON \
    -DNGEN_WITH_EXTERN_PET:BOOL=ON \
    -DNGEN_WITH_EXTERN_NOAH_OWP_MODULAR:BOOL=ON"

RUN cmake \
    -G Ninja \
    -B ngen_serial \
    -S . \
    ${COMMON_BUILD_ARGS} \
    -DNGEN_WITH_MPI:BOOL=OFF


RUN cmake \
    --build ngen_serial \
    --target all \
    -- \
    -j ${NPROC}

ARG MPI_BUILD_ARGS=" \
    -DNGEN_WITH_MPI:BOOL=ON \
    -DNetCDF_ROOT=/usr/lib64/mpich \
    -DCMAKE_PREFIX_PATH=/usr/lib64/mpich \
    -DCMAKE_LIBRARY_PATH=/usr/lib64/mpich/lib"

# Install the mpi enabled netcdf library and build ngen parallel with it
RUN dnf install -y netcdf-cxx4-mpich-devel
RUN cmake \
    -G Ninja \
    -B ngen_parallel \
    -S . ${COMMON_BUILD_ARGS} ${MPI_BUILD_ARGS} \
    -DNetCDF_CXX_INCLUDE_DIR=/usr/include/mpich-$(arch) \
    -DNetCDF_INCLUDE_DIR=/usr/include/mpich-$(arch)

RUN cmake \
    --build ngen_parallel \
    --target all \
    # -- \
    -j ${NPROC}

RUN find /ngen/ngen/extern -maxdepth 2 -mindepth 2 -type d \
    -exec uv pip install {} --python-version 3.9 \;


# Stage 8: Download dhbv2 MTS weights
FROM dependencies AS dhbv2_weights

RUN dnf install -y git gcc python3-devel gcc-c++ awscli unzip

RUN aws s3 cp s3://mhpi-spatial/mhpi-release/models/owp/dhbv_2_mts.zip /tmp/file.zip --no-sign-request && \
    unzip -o /tmp/file.zip -d /tmp && \
    mkdir -p /dhbv_2_weights && \
    cp -r /tmp/dhbv_2_mts/* /dhbv_2_weights && \
    rm -r /tmp


# Stage 9: Restructure files for final image
FROM ngen_build AS restructure_files

# Setup final directories and permissions
RUN mkdir -p /dmod/datasets /dmod/datasets/static /dmod/shared_libs /dmod/bin /dmod/utils/ \
    && shopt -s globstar \
    && cp -a ./extern/**/cmake_build/*.so* /dmod/shared_libs/. || true \
    && cp -a ./extern/noah-owp-modular/**/*.TBL /dmod/datasets/static \
    && cp -a ./ngen_parallel/ngen /dmod/bin/ngen-parallel || true \
    && cp -a ./ngen_serial/ngen /dmod/bin/ngen-serial || true \
    && cp -a ./ngen_parallel/partitionGenerator /dmod/bin/partitionGenerator || true \
    && cp -ar ./utilities/* /dmod/utils/ \
    && cd /dmod/bin \
    && (stat ngen-parallel && ln -s ngen-parallel ngen) || (stat ngen-serial \
    && ln -s ngen-serial ngen)

RUN find ./extern -name "*.so*" -exec cp -a {} /dmod/shared_libs/ \;


# Stage 10: Development image
FROM restructure_files AS dev

# Set up library path
RUN echo "/dmod/shared_libs/" >> /etc/ld.so.conf.d/ngen.conf && ldconfig -v

# Add mpirun to path
ENV PATH="$PATH:/usr/lib64/mpich/bin"

#  Set permissions
RUN chmod a+x /dmod/bin/*
RUN mv /ngen/ngen /ngen/ngen_src
WORKDIR /ngen


# Stage 11: Final image + build python env
FROM base AS final

WORKDIR /ngen

COPY --from=ngen_build /ngen/.venv /ngen/.venv
ENV VIRTUAL_ENV=/ngen/.venv
RUN uv venv $VIRTUAL_ENV
ENV PATH="/ngen/.venv/bin:/usr/lib64/mpich/bin:${PATH}"

# Load build stages
COPY --from=troute_build /ngen/t-route ./t-route
COPY --from=restructure_files /dmod ./dmod
COPY --from=restructure_files /dmod/shared_libs /ngen/shared_libs

COPY --from=ngen_build /ngen/ngen/ngen_parallel/extern ./extern
COPY --from=ngen_build /ngen/ngen/ngen_serial/test ./test

COPY --from=ngen_build /ngen/ngen/data ./data
COPY --from=ngen_build /ngen/ngen/test/data ./test/data
COPY --from=dhbv2_weights /dhbv_2_weights ./data/dhbv2_mts/models/dhbv_2_mts/

# Python env setup + install t-route wheels
RUN mv ./t-route/src/troute-*/dist/*.whl /tmp/ \
    && uv pip install --no-cache-dir /tmp/*.whl netCDF4==1.6.3 \
    && uv pip install "numpy<2.0.0" bmipy pandas scipy \
    && rm /tmp/*.whl

# Make sure libraries are indexed
RUN find /ngen/extern -name "*.so" -exec dirname {} + | sort -u > /etc/ld.so.conf.d/ngen.conf \
    && ldconfig

# Binary links and library paths
RUN chmod a+x ./dmod/bin/* \
    && ln -sf /ngen/dmod/bin/ngen /usr/local/bin/ngen \
    && ln -sf /ngen/dmod/bin/partitionGenerator /usr/local/bin/partitionGenerator

RUN echo "/usr/lib64/mpich/lib" > /etc/ld.so.conf.d/00-mpich.conf && \
    echo "/ngen/dmod/shared_libs" > /etc/ld.so.conf.d/01-ngen.conf && \
    find ./extern -name "cmake_build" -type d >> /etc/ld.so.conf.d/01-ngen.conf && \
    ldconfig

# Set absolute environment paths
ENV LD_LIBRARY_PATH="/usr/lib64/mpich/lib:/ngen/dmod/shared_libs:/usr/lib64:/usr/local/lib"
ENV PYTHONPATH="/ngen/.venv/lib/python3.9/site-packages:/ngen/extern"
ENV PATH="/ngen/dmod/bin:${PATH}"

# Permissions for tests doing temp file writes
RUN chmod -R 777 /tmp && chmod -R +x /ngen/test/
ENV CI=true

RUN echo "export PS1='\u\[\033[01;32m\]@ngiab_dev\[\033[00m\]:\[\033[01;35m\]\W\[\033[00m\]\$ '" >> ~/.bashrc

# Suppress warning from troute + dmod (?)
ENV PYTHONWARNINGS="ignore:networkx backend defined more than once:RuntimeWarning"

CMD ["./ngen", "data/catchment_data.geojson", "", "data/nexus_data.geojson", "", "data/example_realization_config.json"]
