# syntax=docker/dockerfile:1.4

##############################
# Stage: Base – Common Setup
##############################
ARG ORG=ngwpc
ARG NGEN_FORCING_IMAGE_TAG=latest
ARG NGEN_FORCING_IMAGE=ghcr.io/${ORG}/ngen-bmi-forcing:${NGEN_FORCING_IMAGE_TAG}

FROM ${NGEN_FORCING_IMAGE} AS base

# Uncomment when building locally
# FROM ngen-bmi-forcing AS base

# OCI Metadata Arguments
ARG NGEN_FORCING_IMAGE
ARG BASE_IMAGE_DIGEST="unknown"
ARG BASE_IMAGE_REVISION="unknown"
ARG IMAGE_SOURCE="unknown"
ARG IMAGE_VENDOR="unknown"
ARG IMAGE_VERSION="unknown"
ARG IMAGE_REVISION="unknown"

# OCI Standard Labels
LABEL org.opencontainers.image.base.name="${NGEN_FORCING_IMAGE}" \
    org.opencontainers.image.base.digest="${BASE_IMAGE_DIGEST}" \
    io.ngwpc.image.base.revision="${BASE_IMAGE_REVISION}" \
    org.opencontainers.image.source="${IMAGE_SOURCE}" \
    org.opencontainers.image.vendor="${IMAGE_VENDOR}" \
    org.opencontainers.image.version="${IMAGE_VERSION}" \
    org.opencontainers.image.revision="${IMAGE_REVISION}" \
    org.opencontainers.image.title="Next Generation Water Modeling Engine and Framework Prototype" \
    org.opencontainers.image.description="Docker image for the NGEN application"

# cannot remove LANG even though https://bugs.python.org/issue19846 is fixed
# last attempted removal of LANG broke many users:
# https://github.com/docker-library/python/pull/570
ENV LANG="C.UTF-8" \
    \
    PYTHON_VERSION="3.11.13" \
    SZIP_VERSION="2.1.1" \
    HDF5_VERSION="1.10.11" \
    NETCDF_C_VERSION="4.7.4" \
    NETCDF_FORTRAN_VERSION="4.5.4" \
    BOOST_VERSION="1.86.0"

# runtime dependencies
RUN set -eux && \
    dnf install -y epel-release && \
    dnf config-manager --set-enabled powertools && \
    dnf install -y \
        cmake \
        curl curl-devel \
        file \
        findutils \
        git \
## FIXME: replace GNU compilers with Intel compiler ##
        gcc-toolset-10 \
        gcc-toolset-10-libasan-devel \
        libasan6 \
        libffi libffi-devel \
        m4 \
        udunits2 udunits2-devel \
        bzip2 bzip2-devel \
        zlib zlib-devel \
        xz-devel \
## FIXME: replace openmpi with Intel MPI libraries ##
        openmpi openmpi-devel \
        openssl openssl-devel \
        rsync \
        sqlite sqlite-devel \
        tk tk-devel \
        uuid uuid-devel \
        which \
        xz \
        jq \
    && dnf clean all && rm -rf /var/cache/dnf

## FIXME: replace GNU compilers with Intel compiler ##
SHELL [ "/usr/bin/scl", "enable", "gcc-toolset-10" ]
## FIXME: replace openmpi with Intel MPI libraries ##
ENV PATH="${PATH}:/usr/lib64/openmpi/bin/"

# Fix OpenMPI support within container
ENV PSM3_HAL=loopback \
    PSM3_DEVICES=self

# Build Python from source and install it
RUN --mount=type=cache,target=/root/.cache/build-python,id=build-python \
	set -eux && \
    curl --location --output python.tar.xz "https://www.python.org/ftp/python/${PYTHON_VERSION%%[a-z]*}/Python-$PYTHON_VERSION.tar.xz" && \
	mkdir --parents /usr/src/python && \
    tar --extract --directory /usr/src/python --strip-components=1 --file python.tar.xz && \
    rm python.tar.xz && \
    cd /usr/src/python && \
	./configure \
		--enable-loadable-sqlite-extensions \
		--enable-optimizations \
		--enable-option-checking=fatal \
		--enable-shared \
		--with-lto \
		--with-system-expat \
		--without-ensurepip && \
	nproc="$(nproc)" && \
	make -j "$nproc" "PROFILE_TASK=${PROFILE_TASK:-}" && \
# https://github.com/docker-library/python/issues/784
# prevent accidental usage of a system installed libpython of the same version
    rm python && \
	make -j "$nproc" \
		"LDFLAGS=${LDFLAGS:--Wl},-rpath='\$\$ORIGIN/../lib'" \
		"PROFILE_TASK=${PROFILE_TASK:-}" \
        python && \
    make install && \
# enable GDB to load debugging data: https://github.com/docker-library/python/pull/701
    bin="$(readlink -ve /usr/local/bin/python3)" && \
    dir="$(dirname "$bin")" && \
    mkdir --parents "/usr/share/gdb/auto-load/$dir" && \
    cp -vL Tools/gdb/libpython.py "/usr/share/gdb/auto-load/$bin-gdb.py" && \
    cd / && \
    rm -rf /usr/src/python && \
    find /usr/local -depth \
        \( \
            \( -type d -a \( -name test -o -name tests -o -name idle_test \) \) \
            -o \( -type f -a \( -name '*.pyc' -o -name '*.pyo' -o -name 'libpython*.a' \) \) \
        \) -exec rm -rf '{}' + && \
    ldconfig && \
    python3 --version

# make some useful symlinks that are expected to exist ("/usr/local/bin/python" and friends)
RUN set -eux && \
	for src in idle3 pydoc3 python3 python3-config; do \
		dst="$(echo "$src" | tr -d 3)"; \
		[ -s "/usr/local/bin/$src" ]; \
		[ ! -e "/usr/local/bin/$dst" ]; \
		ln -svT "$src" "/usr/local/bin/$dst"; \
	done

# Build additional libraries (SZIP, HDF5, netcdf-c, netcdf-fortran, Boost)
RUN --mount=type=cache,target=/root/.cache/cmake,id=cmake-szip \
    set -eux && \
	curl --location --output szip.tar.gz https://docs.hdfgroup.org/archive/support/ftp/lib-external/szip/${SZIP_VERSION%%[a-z]*}/src/szip-${SZIP_VERSION}.tar.gz && \
	mkdir --parents /usr/src/szip && \
	tar --extract --directory /usr/src/szip --strip-components=1 --file szip.tar.gz && \
	cd /usr/src/szip && \
	./configure --prefix=/usr/local/ && \
	nproc="$(nproc)" && \
	make -j "$nproc" && \
	make install && \
    strip --strip-debug /usr/local/lib/libsz*.so.* || true && \
    rm --recursive --force /usr/src/szip szip.tar.gz

RUN --mount=type=cache,target=/root/.cache/cmake,id=cmake-hdf5 \
    set -eux && \
	curl --location --output hdf5.tar.gz https://github.com/HDFGroup/hdf5/archive/refs/tags/hdf5-${HDF5_VERSION//./_}.tar.gz && \
	mkdir --parents /usr/src/hdf5 && \
	tar --extract --directory /usr/src/hdf5 --strip-components=1 --file hdf5.tar.gz && \
	cd /usr/src/hdf5 && \
	./configure --prefix=/usr/local/ --with-szlib=/usr/local/ && \
	make check -j "$(nproc)" && \
    make install && \
    strip --strip-debug /usr/local/lib/libhdf5*.so.* || true && \
    rm -f /usr/local/lib/libhdf5*.a && \
    rm --recursive --force /usr/src/hdf5 hdf5.tar.gz

RUN --mount=type=cache,target=/root/.cache/cmake,id=cmake-netcdf-c \
    set -eux && \
    curl --location --output netcdf-c.tar.gz https://github.com/Unidata/netcdf-c/archive/refs/tags/v${NETCDF_C_VERSION%%[a-z]*}.tar.gz && \
    mkdir --parents /usr/src/netcdf-c && \
    tar --extract --directory /usr/src/netcdf-c --strip-components=1 --file netcdf-c.tar.gz && \
    cd /usr/src/netcdf-c && \
    cmake -B cmake_build -S . -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DENABLE_TESTS=OFF && \
    cmake --build cmake_build --parallel "$(nproc)" && \
    cmake --build cmake_build --target install && \
    #strip --strip-debug /usr/local/lib64/libnetcdf*.so.* || true && \
    rm --recursive --force /usr/src/netcdf-c netcdf-c.tar.gz

RUN --mount=type=cache,target=/root/.cache/cmake,id=cmake-netcdf-fortran \
    set -eux && \
    curl --location --output netcdf-fortran.tar.gz https://github.com/Unidata/netcdf-fortran/archive/refs/tags/v${NETCDF_FORTRAN_VERSION%%[a-z]*}.tar.gz && \
    mkdir --parents /usr/src/netcdf-fortran && \
    tar --extract --directory /usr/src/netcdf-fortran --strip-components=1 --file netcdf-fortran.tar.gz && \
    cd /usr/src/netcdf-fortran && \
    cmake -B cmake_build -S . -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DENABLE_TESTS=OFF \
       -DCMAKE_Fortran_COMPILER=/opt/rh/gcc-toolset-10/root/usr/bin/gfortran && \
    cmake --build cmake_build --parallel "$(nproc)" && \
    cmake --build cmake_build --target install && \
    #strip --strip-debug /usr/local/lib/libnetcdff*.so.* || true && \
    rm --recursive --force /usr/src/netcdf-fortran netcdf-fortran.tar.gz

RUN --mount=type=cache,target=/root/.cache/cmake,id=cmake-boost \
    set -eux && \
    curl --location --output boost.tar.gz https://archives.boost.io/release/${BOOST_VERSION%%[a-z]*}/source/boost_${BOOST_VERSION//./_}.tar.gz && \
    mkdir --parents /opt/boost && \
    tar --extract --directory /opt/boost --strip-components=1 --file boost.tar.gz && \
    rm boost.tar.gz \
    # build boost libraries
    && cd /opt/boost && ./bootstrap.sh && ./b2 && ./b2 install --prefix=/usr/local && \
    strip --strip-debug /usr/local/lib/libboost_*.so.* || true && \
    rm -f /usr/local/lib/libboost_*.a

ENV VIRTUAL_ENV="/ngen-app/ngen-python"

# Create virtual environment for the application and upgrade pip within it
RUN python3.11 -m venv --system-site-packages ${VIRTUAL_ENV}

ENV PATH="${VIRTUAL_ENV}/bin:${PATH}"

# Consolidated LD_LIBRARY_PATH for MPI
ENV LD_LIBRARY_PATH="/usr/lib64/openmpi/lib:/usr/local/lib:/usr/local/lib64:${LD_LIBRARY_PATH}"

RUN --mount=type=cache,target=/root/.cache/pip,id=pip-cache \
    set -eux && \
    pip3 install --upgrade pip setuptools wheel && \
    pip3 install 'numpy==1.26.4' && \
    pip3 install --no-binary=mpi4py 'mpi4py'&& \
    pip3 install 'netcdf4<=1.6.3' && \
    pip3 install 'bmipy' && \
    pip3 install 'pandas' && \
    pip3 install 'pyyml' && \
    pip3 install torch torchvision --index-url https://download.pytorch.org/whl/cpu && \
    pip install /ngen-app/ngen-forcing

WORKDIR /ngen-app/

##############################
# Stage: EWTS Build – Error, Warning and Trapping System
##############################
# EWTS is built in its own stage so that:
#   - It is cached independently from ngen source changes (COPY . /ngen-app/ngen/
#     happens later in the submodules stage).
#   - Iterative ngen/submodule development doesn't re-trigger the EWTS clone+build.
#   - EWTS_ORG / EWTS_REF can be pinned without affecting other stages' caches.
#
# EWTS provides a unified logging framework used by ngen core and ALL C, C++, Fortran,
# and Python submodules. Libraries are created for C, C++ and Fortran submodules
# (cfe, evapotranspiration, LASAM, noah-owp-modular, snow17, sac-sma,
# SoilFreezeThaw, SoilMoistureProfiles, topmodel, ueb-bmi) and a Python package is
# used by Python sumbodules (lstm, topoflow-glacier and t-route).
#
# How the plumbing works:
#   1. We build EWTS here and install it to /opt/ewts.
#   2. Every cmake call in the submodules stage passes
#      -DCMAKE_PREFIX_PATH=/opt/ewts so that
#      find_package(ewts CONFIG REQUIRED) in each submodule's CMakeLists.txt
#      can locate the ewtsConfig.cmake package file.
#   3. The following gives each submodule access to the EWTS targets:
#        ewts::ewts_c            – C runtime             (cfe, evapotranspiration, topmodel)
#        ewts::ewts_cpp          – C++ runtime logger    (used by LASAM, SoilFreezeThaw, SoilMoistureProfiles)
#        ewts::ewts_fortran      – Fortran runtime       (noah-owp-modular sac-sma,, snow17)
#        ewts::ewts_ngen_bridge  – ngen↔EWTS bridge lib  (linked by ngen itself)
#        EWTS Python wheel       – pip intalled package  (lstm, topoflow-glacier, t-route)
#
# Build args – override at build time to pin a branch, tag, or full commit SHA:
#   docker build --build-arg EWTS_REF=v1.2.3 ...
#   docker build --build-arg EWTS_REF=abc123def456 ...
##############################
FROM base AS ewts-build

SHELL [ "/usr/bin/scl", "enable", "gcc-toolset-10" ]

ARG EWTS_ORG=NGWPC
ARG EWTS_REF=development
ARG EWTS_CACHE_BUST=0

# Install path for the built EWTS libraries, headers, cmake config, and
# Fortran .mod files.  /opt/ewts follows the FHS convention for add-on
# packages (same pattern as /opt/boost already in this image) and avoids
# /tmp which can be cleaned unexpectedly.
ENV EWTS_PREFIX=/opt/ewts

# Point the development fallback to the cloned source tree so that
# compiler.sh can pip-install EWTS from source if the wheel is missing.
ENV EWTS_PY_ROOT=/tmp/nwm-ewts/runtime/python/ewts

# Clone nwm-ewts, build, install, capture git metadata for provenance,
# then remove the source tree.
# Try shallow clone by branch/tag name first; fall back to full clone + checkout
# for bare commit SHAs (which git clone -b doesn't support).
RUN --mount=type=cache,target=/root/.cache/cmake,id=cmake-ewts \
    echo "EWTS cache bust: ${EWTS_CACHE_BUST}" && \
    set -eux && \
    (git clone --depth 1 -b "${EWTS_REF}" \
        "https://github.com/${EWTS_ORG}/nwm-ewts.git" /tmp/nwm-ewts \
     || (git clone "https://github.com/${EWTS_ORG}/nwm-ewts.git" /tmp/nwm-ewts && \
         cd /tmp/nwm-ewts && git checkout "${EWTS_REF}")) && \
    cd /tmp/nwm-ewts && \
    # ── Build EWTS ──
    # This produces: C, C++, Fortran shared libs + ngen bridge + Python wheel.
    # -DEWTS_WITH_NGEN=ON  enables the ngen bridge (ewts_ngen_bridge.so) which
    #   provides the C shim ewts_ngen_log() that ngen's core calls into.
    # -DEWTS_BUILD_SHARED=ON  builds .so's so submodule DSOs can link at runtime.
    cmake -S . -B cmake_build \
      -DCMAKE_BUILD_TYPE=Release \
      -DEWTS_WITH_NGEN=ON \
      -DEWTS_BUILD_SHARED=ON && \
    cmake --build cmake_build -j "$(nproc)" && \
    cmake --install cmake_build --prefix ${EWTS_PREFIX} && \
    # ── Capture EWTS git provenance ──
    # Saved as a JSON sidecar so the git-info merge step at the bottom of this
    # Dockerfile can include EWTS metadata alongside ngen + submodules.
    jq -n \
      --arg commit_hash "$(git rev-parse HEAD)" \
      --arg branch "$(git branch -r --contains HEAD 2>/dev/null | grep -v '\->' | sed 's|origin/||' | head -n1 | xargs || echo "${EWTS_REF}")" \
      --arg tags "$(git tag --points-at HEAD 2>/dev/null | tr '\n' ' ')" \
      --arg author "$(git log -1 --pretty=format:'%an')" \
      --arg commit_date "$(date -u -d @$(git log -1 --pretty=format:'%ct') +'%Y-%m-%d %H:%M:%S UTC')" \
      --arg message "$(git log -1 --pretty=format:'%s' | tr '\n' ';')" \
      --arg build_date "$(date -u +'%Y-%m-%d %H:%M:%S UTC')" \
      '{"nwm-ewts": {commit_hash: $commit_hash, branch: $branch, tags: $tags, author: $author, commit_date: $commit_date, message: $message, build_date: $build_date}}' \
      > /ngen-app/nwm-ewts_git_info.json && \
    # ── Cleanup source (keep Python source as fallback for compiler.sh) ──
    cd / && \
    rm -rf /tmp/nwm-ewts/cmake_build /tmp/nwm-ewts/.git

# Install the EWTS Python wheel into the venv.
# This is what makes "import ewts" work for Python-based submodules.
# For example, lstm's bmi_lstm.py does:  import ewts; LOG = ewts.get_logger(ewts.LSTM_ID)
RUN --mount=type=cache,target=/root/.cache/pip,id=pip-cache \
    set -eux && \
    pip install ${EWTS_PREFIX}/python/dist/ewts-*.whl

# Make EWTS shared libraries (.so) discoverable at runtime.
# Without this, ngen and every submodule DSO would fail with:
#   "error while loading shared libraries: libewts_ngen_bridge.so: cannot open"
# We include both lib/ and lib64/ because cmake may install to either depending
# on the platform/distro convention.
ENV LD_LIBRARY_PATH="${EWTS_PREFIX}/lib:${EWTS_PREFIX}/lib64:${LD_LIBRARY_PATH}"

##############################
# Stage: Submodules Build
##############################
# Inherits from ewts-build so /opt/ewts is already present.
# The ngen source COPY happens here — changing ngen code only invalidates
# this stage and below, not the EWTS build above.
##############################
FROM ewts-build AS submodules

SHELL [ "/usr/bin/scl", "enable", "gcc-toolset-10" ]

WORKDIR /ngen-app/

# Copy the ngen application source.
# This is placed here (not in base) so that
# ngen code changes only invalidate the submodules stage, leaving the base and
# ewts-build stages cached.
COPY . /ngen-app/ngen/

WORKDIR /ngen-app/ngen/

# Copy only the requirements files first for dependency installation caching
COPY extern/test_bmi_py/requirements.txt /tmp/test_bmi_py_requirements.txt
COPY extern/t-route/requirements.txt /tmp/t-route_requirements.txt

# Install Python dependencies and remove the temporary requirements files
RUN --mount=type=cache,target=/root/.cache/pip,id=pip-cache \
    set -eux && \
        pip3 install -r /tmp/test_bmi_py_requirements.txt && \
        pip3 install -r /tmp/t-route_requirements.txt && \
        rm /tmp/test_bmi_py_requirements.txt /tmp/t-route_requirements.txt

ENV LD_LIBRARY_PATH=/usr/local/lib64:$LD_LIBRARY_PATH

# Use cache for building the t-route submodule
RUN --mount=type=cache,target=/root/.cache/t-route,id=t-route-build \
    set -eux && \
    cd extern/t-route && \
    echo "Running compiler.sh" && \
    LDFLAGS='-Wl,-L/usr/local/lib64/,-L/usr/local/lib/,-rpath,/usr/local/lib64/,-rpath,/usr/local/lib/' && \
    ./compiler.sh no-e && \
    rm -rf /ngen-app/ngen/extern/t-route/test/LowerColorado_TX_v4 && \
    find /ngen-app/ngen/extern/t-route -name "*.o" -exec rm -f {} + && \
    find /ngen-app/ngen/extern/t-route -name "*.a" -exec rm -f {} +

# Configure the build with cache for CMake
# -DCMAKE_PREFIX_PATH=${EWTS_PREFIX} tells cmake where
# to find the ewtsConfig.cmake package file so that ngen's
# CMakeLists.txt line find_package(ewts CONFIG REQUIRED) succeeds.
# ngen links against ewts::ewts_ngen_bridge (the C++/MPI bridge).
RUN --mount=type=cache,target=/root/.cache/cmake,id=cmake-ngen \
    set -eux && \
        export FFLAGS="-fPIC" && \
        export FCFLAGS="-fPIC" && \
        export CMAKE_Fortran_FLAGS="-fPIC" && \
        cmake -B cmake_build -S . \
          -DCMAKE_PREFIX_PATH=${EWTS_PREFIX} \
          -DNGEN_WITH_MPI=ON \
          -DNGEN_WITH_NETCDF=ON \
          -DNGEN_WITH_SQLITE=ON \
          -DNGEN_WITH_UDUNITS=ON \
          -DNGEN_WITH_BMI_FORTRAN=ON \
          -DNGEN_WITH_BMI_C=ON \
          -DNGEN_WITH_PYTHON=ON \
          -DNGEN_WITH_TESTS=ON \
          -DNGEN_WITH_ROUTING=ON \
          -DNGEN_QUIET=ON \
          -DNGEN_UPDATE_GIT_SUBMODULES=OFF \
          -DBOOST_ROOT=/opt/boost && \
    cmake --build cmake_build --target all && \
    rm -rf /ngen-app/ngen/cmake_build/test/CMakeFiles && \
    rm -rf /ngen-app/ngen/cmake_build/src/core/CMakeFiles && \
    find /ngen-app/ngen/cmake_build -name "*.a" -exec rm -f {} + && \
    find /ngen-app/ngen/cmake_build -name "*.o" -exec rm -f {} +

# ──────────────────────────────────────────────────────────────────────────────
# Build each submodule in a separate layer, using cache for CMake as well
#
# IMPORTANT: Every submodule's CMakeLists.txt now contains:
#   find_package(ewts CONFIG REQUIRED)
# so we MUST pass -DCMAKE_PREFIX_PATH=${EWTS_PREFIX} to each cmake call.
# Without it, cmake cannot locate ewtsConfig.cmake and the build fails with:
#   "Could not find a package configuration file provided by "ewts"..."
#
# What each submodule links:
#   LASAM              → ewts::ewts_cpp  + ewts::ewts_ngen_bridge
#   snow17             → ewts::ewts_fortran + ewts::ewts_ngen_bridge
#   sac-sma            → ewts::ewts_fortran + ewts::ewts_ngen_bridge
#   SoilMoistureProfiles → (check its CMakeLists.txt for specifics)
#   SoilFreezeThaw     → (check its CMakeLists.txt for specifics)
#   cfe                → (check its CMakeLists.txt for specifics)
#   ueb-bmi            → (check its CMakeLists.txt for specifics)
# ──────────────────────────────────────────────────────────────────────────────

RUN --mount=type=cache,target=/root/.cache/cmake,id=cmake-lasam \
    set -eux && \
    cmake -B extern/LASAM/cmake_build -S extern/LASAM/ \
      -DCMAKE_PREFIX_PATH=${EWTS_PREFIX} -DNGEN=ON -DBOOST_ROOT=/opt/boost && \
    cmake --build extern/LASAM/cmake_build/ && \
    find /ngen-app/ngen/extern/LASAM -name '*.o' -exec rm -f {} +

RUN --mount=type=cache,target=/root/.cache/cmake,id=cmake-snow17 \
    set -eux && \
    cmake -B extern/snow17/cmake_build -S extern/snow17/ \
      -DCMAKE_PREFIX_PATH=${EWTS_PREFIX} -DBOOST_ROOT=/opt/boost && \
    cmake --build extern/snow17/cmake_build/ && \
    find /ngen-app/ngen/extern/snow17 -name '*.o' -exec rm -f {} +

RUN --mount=type=cache,target=/root/.cache/cmake,id=cmake-sac-sma \
    set -eux && \
    cmake -B extern/sac-sma/cmake_build -S extern/sac-sma/ \
      -DCMAKE_PREFIX_PATH=${EWTS_PREFIX} -DBOOST_ROOT=/opt/boost && \
    cmake --build extern/sac-sma/cmake_build/ && \
    find /ngen-app/ngen/extern/sac-sma -name '*.o' -exec rm -f {} +

RUN --mount=type=cache,target=/root/.cache/cmake,id=cmake-soilmoistureprofiles \
    set -eux && \
    cmake -B extern/SoilMoistureProfiles/cmake_build -S extern/SoilMoistureProfiles/SoilMoistureProfiles/ \
      -DCMAKE_PREFIX_PATH=${EWTS_PREFIX} -DNGEN=ON -DBOOST_ROOT=/opt/boost && \
    cmake --build extern/SoilMoistureProfiles/cmake_build/ && \
    find /ngen-app/ngen/extern/SoilMoistureProfiles -name '*.o' -exec rm -f {} +

RUN --mount=type=cache,target=/root/.cache/cmake,id=cmake-soilfreezethaw \
    set -eux && \
    cmake -B extern/SoilFreezeThaw/cmake_build -S extern/SoilFreezeThaw/SoilFreezeThaw/ \
      -DCMAKE_PREFIX_PATH=${EWTS_PREFIX} -DNGEN=ON -DBOOST_ROOT=/opt/boost && \
    cmake --build extern/SoilFreezeThaw/cmake_build/ && \
    find /ngen-app/ngen/extern/SoilFreezeThaw -name '*.o' -exec rm -f {} +

RUN --mount=type=cache,target=/root/.cache/cmake,id=cmake-ueb-bmi \
    set -eux && \
    cmake -B extern/ueb-bmi/cmake_build -S extern/ueb-bmi/ \
      -DUEB_SUPPRESS_OUTPUTS=ON -DCMAKE_PREFIX_PATH=${EWTS_PREFIX} \
      -DBMICXX_INCLUDE_DIRS=/ngen-app/ngen/extern/bmi-cxx/ -DBOOST_ROOT=/opt/boost && \
    cmake --build extern/ueb-bmi/cmake_build/ && \
    find /ngen-app/ngen/extern/ueb-bmi/ -name '*.o' -exec rm -f {} +

RUN --mount=type=cache,target=/root/.cache/pip,id=pip-cache \
    set -eux; \
    cd extern/lstm; \
    pip install .

RUN --mount=type=cache,target=/root/.cache/pip,id=pip-cache \
    set -eux; \
    cd extern/topoflow-glacier; \
    pip install .

RUN --mount=type=cache,target=/root/.cache/pip,id=pip-cache \
    set -eux; \
    cd extern/topoflow-glacier; \
    pip install .

RUN set -eux && \
    mkdir --parents /ngencerf/data/ngen-run-logs/ && \
    mkdir --parents /ngen-app/bin/ && \
    mv run-ngen.sh /ngen-app/bin/ && \
    chmod +x /ngen-app/bin/run-ngen.sh && \
    find /ngen-app/ngen -name "*.so" -exec chmod 755 {} \;

WORKDIR /ngen-app/ngen

# --- Git Info Extraction with Submodules ---
# Extract Git information for the main repository
ARG CI_COMMIT_REF_NAME

RUN set -eux && \
    # Get the remote URL from Git configuration
    repo_url=$(git config --get remote.origin.url); \
    # Remove trailing slash if present
    repo_url=${repo_url%/}; \
    # Extract the repo name (everything after the last slash) and remove any trailing .git
    key=${repo_url##*/}; \
    key=${key%.git}; \
    # Construct the file path using the derived key
    export GIT_INFO_PATH="/ngen-app/${key}_git_info.json"; \
    # Determine branch name: use CI_COMMIT_REF_NAME if set; otherwise, use git's current branch
    branch=$( [ -n "${CI_COMMIT_REF_NAME:-}" ] && echo "${CI_COMMIT_REF_NAME}" || git rev-parse --abbrev-ref HEAD ); \
    jq -n \
      --arg commit_hash "$(git rev-parse HEAD)" \
      --arg branch "$branch" \
      --arg tags "$(git tag --points-at HEAD | tr '\n' ' ')" \
      --arg author "$(git log -1 --pretty=format:'%an')" \
      --arg commit_date "$(date -u -d @$(git log -1 --pretty=format:'%ct') +'%Y-%m-%d %H:%M:%S UTC')" \
      --arg message "$(git log -1 --pretty=format:'%s' | tr '\n' ';')" \
      --arg build_date "$(date -u +'%Y-%m-%d %H:%M:%S UTC')" \
      "{\"$key\": {commit_hash: \$commit_hash, branch: \$branch, tags: \$tags, author: \$author, commit_date: \$commit_date, message: \$message, build_date: \$build_date}}" \
      > $GIT_INFO_PATH; \
    \
    # Create directory for submodule JSON files
    mkdir -p /ngen-app/submodules-json; \
    \
    # Process each submodule listed in .gitmodules (skipping unwanted ones)
    for sub in $(git config --file .gitmodules --get-regexp path | awk '{print $2}'); do \
      cd "$sub"; \
      # Derive submodule key from its remote URL
      subrepo_url=$(git config --get remote.origin.url); \
      # Remove trailing slash if present
      subrepo_url=${subrepo_url%/}; \
      sub_key=${subrepo_url##*/}; \
      sub_key=${sub_key%.git}; \
      \
      # Skip unwanted submodules based on the derived key
      if [[ "$sub_key" == "googletest" || "$sub_key" == "pybind11" || "$sub_key" == "netcdf-cxx4" ]]; then \
        cd - > /dev/null; \
        continue; \
      fi; \
      \
      # Try to find a preferred branch (development, release-candidate, main, or master) that contains the current commit
      sub_branch=$( \
        git branch -r --contains HEAD | grep -v '\->' | \
        grep -E 'origin/(development|release-candidate|main|master)' | \
        sed 's|origin/||' | head -n1 | xargs \
      ); \
      \
      # If none of the preferred branches contain the commit, fall back to any branch that does
      if [ -z "$sub_branch" ]; then \
        sub_branch=$(git branch -r --contains HEAD | grep -v '\->' | sed 's|origin/||' | head -n1 | xargs); \
      fi; \
      \
      # If no branches at all contain the commit, use "HEAD" as a fallback
      sub_branch=${sub_branch:-HEAD}; \
      \
      info=$(jq -n \
        --arg commit_hash "$(git rev-parse HEAD)" \
        --arg branch "$sub_branch" \
        --arg tags "$(git tag --points-at HEAD | tr '\n' ' ')" \
        --arg author "$(git log -1 --pretty=format:%an)" \
        --arg commit_date "$(date -u -d @$(git log -1 --pretty=format:%ct) +'%Y-%m-%d %H:%M:%S UTC')" \
        --arg message "$(git log -1 --pretty=format:%s | tr '\n' ' ')" \
        --arg build_date "$(date -u +'%Y-%m-%d %H:%M:%S UTC')" \
        '{"'"$sub_key"'": {commit_hash: $commit_hash, branch: $branch, tags: $tags, author: $author, commit_date: $commit_date, message: $message, build_date: $build_date}}' ); \
      cd - > /dev/null; \
      echo "$info" > /ngen-app/submodules-json/git_info_"$sub_key".json; \
    done; \
    \
    # Merge the main repository JSON + submodule JSONs + EWTS provenance into one file.
    # The EWTS json was created during the EWTS build step above; including it
    # here means `cat /ngen-app/ngen_git_info.json` shows EWTS version info too.
    jq -s 'add' $GIT_INFO_PATH /ngen-app/submodules-json/*.json /ngen-app/nwm-ewts_git_info.json > /ngen-app/merged_git_info.json && \
    mv /ngen-app/merged_git_info.json $GIT_INFO_PATH && \
    rm -rf /ngen-app/submodules-json /ngen-app/nwm-ewts_git_info.json

# Extend PYTHONPATH for LSTM models (preserve venv path from ngen-bmi-forcing)
ENV PYTHONPATH="${PYTHONPATH}:/ngen-app/ngen/extern/lstm:/ngen-app/ngen/extern/lstm/lstm"

# Extend PYTHONPATH for Topoflow-Glacier
ENV PYTHONPATH="${PYTHONPATH}:/ngen-app/ngen/extern/topoflow-glacier:/ngen-app/ngen/extern/topoflow-glacier/src/topoflow-glacier"

WORKDIR /
SHELL ["/bin/bash", "-c"]

ENTRYPOINT [ "/ngen-app/bin/run-ngen.sh" ]
CMD [ "--info" ]
