# syntax=docker/dockerfile:1.4

############################################################################
# Change/Verify these values when adopting this Dockerfile into another org:
#   GH_ORG, GHCR_ORG, IMAGE_NAMESPACE,
#   EWTS_ORG, EWTS_REF
############################################################################

# Ownership / branding overrides
ARG GH_ORG=NGWPC
ARG GHCR_ORG=ngwpc
ARG IMAGE_NAMESPACE=ngwpc

# External repository source and branch overrides
ARG EWTS_ORG=${GH_ORG}
ARG EWTS_REF=development
ARG USE_EWTS=ON

############################################################################
# Image selection
############################################################################

# Use the ngen-bmi-forcing image as the base. This already includes the
# inherited Bookworm-based compiled dependency stack plus the ngen-bmi-forcing
# Python package.
#
# Default build:
#   docker build -t ngen .
#
# Build from a different published forcing image:
#   docker build \
#     --build-arg FORCING_IMAGE=ghcr.io/ngwpc/ngen-bmi-forcing:development \
#     -t ngen .
#
# Build from a locally built forcing image:
#   docker build \
#     --build-arg FORCING_IMAGE=ngen-bmi-forcing \
#     -t ngen .
ARG FORCING_IMAGE=ghcr.io/${GHCR_ORG}/ngen-bmi-forcing:latest

FROM ${FORCING_IMAGE} AS base

# Re-expose args after FROM for the remaining build stage.
ARG GH_ORG
ARG GHCR_ORG
ARG IMAGE_NAMESPACE
ARG EWTS_ORG
ARG EWTS_REF
ARG USE_EWTS=ON

# OCI Metadata Arguments
ARG FORCING_IMAGE
ARG FORCING_IMAGE_NAME="${FORCING_IMAGE}"
ARG FORCING_IMAGE_DIGEST="unknown"
ARG FORCING_IMAGE_REVISION="unknown"
ARG IMAGE_SOURCE="unknown"
ARG IMAGE_VENDOR="unknown"
ARG IMAGE_VERSION="unknown"
ARG IMAGE_REVISION="unknown"
ARG EWTS_REVISION="unknown"

# Image Labels: OCI-spec annotations followed by custom source-repo metadata.
LABEL org.opencontainers.image.base.name="${FORCING_IMAGE_NAME}" \
      org.opencontainers.image.base.digest="${FORCING_IMAGE_DIGEST}" \
      org.opencontainers.image.source="${IMAGE_SOURCE}" \
      org.opencontainers.image.vendor="${IMAGE_VENDOR}" \
      org.opencontainers.image.version="${IMAGE_VERSION}" \
      org.opencontainers.image.revision="${IMAGE_REVISION}" \
      org.opencontainers.image.title="Next Generation Water Modeling Engine and Framework Prototype" \
      org.opencontainers.image.description="Docker image for the NGEN application" \
      io.${IMAGE_NAMESPACE}.image.base.revision="${FORCING_IMAGE_REVISION}" \
      io.${IMAGE_NAMESPACE}.ewts.org="${EWTS_ORG}" \
      io.${IMAGE_NAMESPACE}.ewts.ref="${EWTS_REF}" \
      io.${IMAGE_NAMESPACE}.ewts.revision="${EWTS_REVISION}"

############################################################################
# Runtime/build environment inherited from ngen-bmi-forcing-bookworm
############################################################################

# Reuse the shared venv created by ngen-dependencies-bookworm and populated by
# ngen-bmi-forcing-bookworm. Do not recreate it here.
ENV VIRTUAL_ENV="/ngen-app/ngen-python"
ENV PATH="${VIRTUAL_ENV}/bin:${PATH}"

# ngen-specific runtime/build dependencies not already provided by the dependency image.
RUN --mount=type=cache,target=/var/cache/apt,id=apt-cache-bookworm,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,id=apt-lib-bookworm,sharing=locked \
    set -eux && \
    apt-get update && \
    apt-get install -y --no-install-recommends \
        findutils && \
    rm -rf /var/lib/apt/lists/*

# Use ccache so repeated Docker builds can reuse previously
# compiled C/C++/Fortran objects even when a Docker layer 
# containing source files is invalidated.
# ccache computes a hash based on things like:
#   source file contents
#   compiler flags
#   compiler version
#   included headers

SHELL ["/bin/bash", "-c"]

# Fix OpenMPI support within container.
ENV PSM3_HAL=loopback \
    PSM3_DEVICES=self

WORKDIR /ngen-app/

##############################
# Stage: EWTS Build – Error, Warning and Trapping System
##############################
# EWTS is built in its own stage so that:
#   - It is cached independently from ngen source changes (COPY . /ngen-app/ngen/
#     happens later in the submodules stage).
#   - Iterative ngen/submodule development doesn't re-trigger the EWTS clone+build.
#   - EWTS_REF can be pinned without affecting other stages' caches.
#
# When USE_EWTS is enabled, EWTS provides a unified logging framework used by
# ngen and the C, C++, Fortran, and Python modules that have been wired to it.
# When USE_EWTS is disabled, this stage removes EWTS artifacts and the later
# CMake builds receive -DUSE_EWTS=OFF.
#
# Libraries are created for C, C++ and Fortran submodules
# (cfe, evapotranspiration, LASAM, noah-owp-modular, snow17, sac-sma,
# SoilFreezeThaw, SoilMoistureProfiles, topmodel, ueb-bmi) and a Python package is
# used by Python submodules (lstm, topoflow-glacier and t-route).
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
#        EWTS Python wheel       – python -m pip installed package  (lstm, topoflow-glacier, t-route)
#
# Build args – override at build time to select a remote branch:
#   docker build --build-arg EWTS_REF=development ...
##############################
FROM base AS ewts-build

SHELL ["/bin/bash", "-c"]

# USE_EWTS defaults to ON globally, but this value is also passed into CMake as
# a cached option. If a prior Docker build configured a module with USE_EWTS=OFF,
# a reused CMake build directory may retain that OFF value. Keep the Docker ARG
# default explicit in this stage and normalize/default it before passing it to
# CMake so an unset or empty value never becomes -DUSE_EWTS=.
ARG USE_EWTS=ON
ARG GH_ORG
ARG EWTS_ORG
ARG EWTS_REF
ARG EWTS_CACHE_BUST=0

RUN set -eux && \
    USE_EWTS="${USE_EWTS:-ON}" && \
    USE_EWTS_NORMALIZED="$(echo "${USE_EWTS}" | tr '[:lower:]' '[:upper:]')" && \
    if [[ "${USE_EWTS_NORMALIZED}" =~ ^(ON|YES|TRUE|1)$ ]]; then \
        echo "Checking EWTS branch/tag: ${EWTS_REF}"; \
        if ! git ls-remote --exit-code --heads \
            "https://github.com/${EWTS_ORG}/nwm-ewts.git" \
            "${EWTS_REF}" >/dev/null 2>&1; then \
            echo "ERROR: EWTS branch '${EWTS_REF}' not found in ${EWTS_ORG}/nwm-ewts. Make sure it has been pushed." >&2; \
            exit 1; \
        fi; \
    fi

# Install path for the built EWTS libraries, headers, cmake config, and
# Fortran .mod files. /opt/ewts follows the FHS convention for add-on
# packages and avoids /tmp which can be cleaned unexpectedly.
ENV EWTS_PREFIX=/opt/ewts

# EWTS_PY_ROOT records the EWTS Python source location used by the EWTS-enabled
# t-route build path. It is informational at runtime; code should only use it
# when USE_EWTS is enabled.
ENV EWTS_PY_ROOT=/tmp/nwm-ewts/runtime/python/ewts

# Clone the requested nwm-ewts branch, build and install EWTS, capture git
# metadata for provenance, and then remove the build artifacts and Git metadata.
#
# The shallow clone is expected to succeed because the validation above confirms
# that EWTS_REF identifies an existing remote branch. The fallback clone and
# checkout are retained to provide a clearer failure path if the shallow clone
# cannot be completed.
#
# Ensures an unset or empty USE_EWTS defaults to ON.
#
# USE_EWTS accepted ON values:
#   ON, YES, TRUE, 1
#
# Everything else is treated as OFF.
#
RUN --mount=type=cache,target=/root/.cache/ccache,id=ccache-bookworm \
    --mount=type=cache,target=/root/.cache/pip,id=pip-cache-bookworm \
    echo "USE_EWTS=${USE_EWTS}; EWTS cache bust: ${EWTS_CACHE_BUST}" && \
    set -eux && \
    USE_EWTS="${USE_EWTS:-ON}" && \
    USE_EWTS_NORMALIZED="$(echo "${USE_EWTS}" | tr '[:lower:]' '[:upper:]')" && \
    if [[ "${USE_EWTS_NORMALIZED}" =~ ^(ON|YES|TRUE|1)$ ]]; then \
        echo "Building and installing EWTS"; \
        rm -rf /tmp/nwm-ewts && \
        git clone --depth 1 -b "${EWTS_REF}" \
            "https://github.com/${EWTS_ORG}/nwm-ewts.git" /tmp/nwm-ewts \
        || (git clone "https://github.com/${EWTS_ORG}/nwm-ewts.git" /tmp/nwm-ewts && \
            cd /tmp/nwm-ewts && git checkout "${EWTS_REF}"); \
        rm -rf /tmp/nwm-ewts/cmake_build && \
        cd /tmp/nwm-ewts; \
        export PATH="${VIRTUAL_ENV}/bin:${PATH}"; \
        export PYTHONNOUSERSITE=1; \
        export PYTHONPATH=; \
        python -c "import sys, setuptools, wheel, build, packaging; print(sys.executable); print(setuptools.__version__)"; \
        cmake -S . -B cmake_build \
            -DCMAKE_BUILD_TYPE=Release \
            -DEWTS_WITH_NGEN=ON \
            -DEWTS_BUILD_SHARED=ON \
            -DPython_EXECUTABLE="${VIRTUAL_ENV}/bin/python" \
            -DPython3_EXECUTABLE="${VIRTUAL_ENV}/bin/python" \
            -DCMAKE_C_COMPILER_LAUNCHER=ccache \
            -DCMAKE_CXX_COMPILER_LAUNCHER=ccache; \
        cmake --build cmake_build -j "$(nproc)"; \
        cmake --install cmake_build --prefix "${EWTS_PREFIX}"; \
        jq -n \
            --arg commit_hash "$(git rev-parse HEAD)" \
            --arg branch "$(git branch -r --contains HEAD 2>/dev/null | grep -v '\->' | sed 's|origin/||' | head -n1 | xargs || echo "${EWTS_REF}")" \
            --arg tags "$(git tag --points-at HEAD 2>/dev/null | tr '\n' ' ')" \
            --arg author "$(git log -1 --pretty=format:'%an')" \
            --arg commit_date "$(date -u -d @$(git log -1 --pretty=format:'%ct') +'%Y-%m-%d %H:%M:%S UTC')" \
            --arg message "$(git log -1 --pretty=format:'%s' | tr '\n' ';')" \
            --arg build_date "$(date -u +'%Y-%m-%d %H:%M:%S UTC')" \
            '{"nwm-ewts": {commit_hash: $commit_hash, branch: $branch, tags: $tags, author: $author, commit_date: $commit_date, message: $message, build_date: $build_date}}' \
            > /ngen-app/nwm-ewts_git_info.json; \
        python -m pip install "${EWTS_PREFIX}"/python/dist/ewts-*.whl; \
        cd /; \
        rm -rf /tmp/nwm-ewts/cmake_build /tmp/nwm-ewts/.git; \
    else \
        echo "EWTS disabled; removing any EWTS artifacts"; \
        python -m pip uninstall -y ewts || true; \
        rm -rf "${EWTS_PREFIX}" /tmp/nwm-ewts /ngen-app/nwm-ewts_git_info.json; \
    fi

# When USE_EWTS=ON, make EWTS shared libraries (.so) discoverable at runtime.
# Harmless when USE_EWTS=OFF because these directories will not exist.
# We include both lib/ and lib64/ because cmake may install to either depending
# on the platform/distro convention.
ENV LD_LIBRARY_PATH="${EWTS_PREFIX}/lib:${EWTS_PREFIX}/lib64:${LD_LIBRARY_PATH}"

##############################
# Stage: Submodules Build
##############################
# Inherits from ewts-build. When USE_EWTS is enabled, /opt/ewts and the
# EWTS Python package are present; when disabled, EWTS artifacts are removed.
#
# Cache strategy:
#   1. Copy dependency manifests first so pip dependency installs are cached.
#   2. Copy only the standalone extern/submodule trees and build/install them.
#   3. Copy the full ngen source only after the submodule-heavy work is done.
#
# Result:
#   - Pure ngen src/include/cmake changes should not invalidate the standalone
#     submodule build layers.
#   - Submodule source changes still invalidate the corresponding submodule
#     COPY/RUN layers, as desired.
#   - The final full COPY keeps .git/.gitmodules/provenance behavior intact.
##############################
FROM ewts-build AS submodules

SHELL ["/bin/bash", "-c"]

# USE_EWTS defaults to ON globally, but this value is also passed into CMake as
# a cached option. If a prior Docker build configured a module with USE_EWTS=OFF,
# a reused CMake build directory may retain that OFF value. Keep the Docker ARG
# default explicit in this stage and normalize/default it before passing it to
# CMake so an unset or empty value never becomes -DUSE_EWTS=.
ARG USE_EWTS=ON
ARG GH_ORG
ARG EWTS_REF

WORKDIR /ngen-app/

# Copy only the requirements files first for dependency installation caching.
# Changes to ngen source files will not invalidate this dependency install layer unless
# one of these requirements files changes.
COPY extern/test_bmi_py/requirements.txt /tmp/test_bmi_py_requirements.txt
COPY extern/t-route/requirements.txt /tmp/t-route_requirements.txt

# Install Python dependencies and remove the temporary requirements files.
RUN --mount=type=cache,target=/root/.cache/pip,id=pip-cache-bookworm \
    set -eux && \
        python -m pip install -r /tmp/test_bmi_py_requirements.txt && \
        python -m pip install -r /tmp/t-route_requirements.txt && \
        rm /tmp/test_bmi_py_requirements.txt /tmp/t-route_requirements.txt

# Copy only the extern/submodule trees that are built independently below.
# Keep these COPY lines separate enough that a change in one submodule does not
# invalidate every unrelated submodule build layer.
WORKDIR /ngen-app/ngen/

COPY extern/bmi-cxx extern/bmi-cxx
COPY extern/LASAM extern/LASAM
COPY extern/iso_c_fortran_bmi extern/iso_c_fortran_bmi
COPY extern/snow17 extern/snow17
COPY extern/sac-sma extern/sac-sma
COPY extern/SoilMoistureProfiles extern/SoilMoistureProfiles
COPY extern/SoilFreezeThaw extern/SoilFreezeThaw
COPY extern/ueb-bmi extern/ueb-bmi
COPY extern/lstm extern/lstm

# Place t-route last because it is currently the submodule that changes most
# often. Keeping it later in the COPY/build sequence avoids invalidating the
# more stable submodule layers above. If t-route stabilizes in the future,
# consider moving it earlier so changes to other submodules do not trigger a
# relatively expensive t-route rebuild.
COPY extern/t-route extern/t-route

# ──────────────────────────────────────────────────────────────────────────────
# Build each submodule in a separate layer.
#
# When USE_EWTS is enabled, CMAKE_PREFIX_PATH allows submodules that use EWTS
# to locate the ewtsConfig.cmake package under /opt/ewts.
#
# When USE_EWTS is disabled, submodules should skip EWTS linkage based on
# -DUSE_EWTS=${USE_EWTS}.
# ──────────────────────────────────────────────────────────────────────────────

ARG LASAM_CACHE_BUST=0
RUN --mount=type=cache,target=/root/.cache/ccache,id=ccache-bookworm \
    set -eux && \
    USE_EWTS="${USE_EWTS:-ON}" && \
    echo "LASAM cache bust: ${LASAM_CACHE_BUST}" && \
    echo "LASAM USE_EWTS=${USE_EWTS}" && \
    USE_EWTS_NORMALIZED="$(echo "${USE_EWTS}" | tr '[:lower:]' '[:upper:]')" && \
    rm -rf extern/LASAM/cmake_build && \
    cmake -B extern/LASAM/cmake_build -S extern/LASAM/ \
      -DCMAKE_PREFIX_PATH=${EWTS_PREFIX} \
      -DUSE_EWTS="${USE_EWTS_NORMALIZED}" \
      -DNGEN=ON \
      -DCMAKE_C_COMPILER_LAUNCHER=ccache \
      -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
      -DBOOST_ROOT=/opt/boost && \
    cmake --build extern/LASAM/cmake_build/ && \
    find /ngen-app/ngen/extern/LASAM -name '*.o' -exec rm -f {} +

ARG SNOW17_CACHE_BUST=0
RUN --mount=type=cache,target=/root/.cache/ccache,id=ccache-bookworm \
    set -eux && \
    USE_EWTS="${USE_EWTS:-ON}" && \
    echo "SNOW17 cache bust: ${SNOW17_CACHE_BUST}" && \
    echo "SNOW17 USE_EWTS=${USE_EWTS}" && \
    USE_EWTS_NORMALIZED="$(echo "${USE_EWTS}" | tr '[:lower:]' '[:upper:]')" && \
    rm -rf extern/snow17/cmake_build && \
    cmake -B extern/snow17/cmake_build -S extern/snow17/ \
      -DCMAKE_PREFIX_PATH=${EWTS_PREFIX} \
      -DUSE_EWTS="${USE_EWTS_NORMALIZED}" \
      -DCMAKE_C_COMPILER_LAUNCHER=ccache \
      -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
      -DBOOST_ROOT=/opt/boost && \
    cmake --build extern/snow17/cmake_build/ && \
    find /ngen-app/ngen/extern/snow17 -name '*.o' -exec rm -f {} +

ARG SACSMA_CACHE_BUST=0
RUN --mount=type=cache,target=/root/.cache/ccache,id=ccache-bookworm \
    set -eux && \
    USE_EWTS="${USE_EWTS:-ON}" && \
    echo "SACSMA cache bust: ${SACSMA_CACHE_BUST}" && \
    echo "SACSMA USE_EWTS=${USE_EWTS}" && \
    USE_EWTS_NORMALIZED="$(echo "${USE_EWTS}" | tr '[:lower:]' '[:upper:]')" && \
    rm -rf extern/sac-sma/cmake_build && \
    cmake -B extern/sac-sma/cmake_build -S extern/sac-sma/ \
      -DCMAKE_PREFIX_PATH=${EWTS_PREFIX} \
      -DUSE_EWTS="${USE_EWTS_NORMALIZED}" \
      -DCMAKE_C_COMPILER_LAUNCHER=ccache \
      -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
      -DBOOST_ROOT=/opt/boost && \
    cmake --build extern/sac-sma/cmake_build/ && \
    find /ngen-app/ngen/extern/sac-sma -name '*.o' -exec rm -f {} +

ARG SMP_CACHE_BUST=0
RUN --mount=type=cache,target=/root/.cache/ccache,id=ccache-bookworm \
    set -eux && \
    USE_EWTS="${USE_EWTS:-ON}" && \
    echo "SMP cache bust: ${SMP_CACHE_BUST}" && \
    echo "SMP USE_EWTS=${USE_EWTS}" && \
    USE_EWTS_NORMALIZED="$(echo "${USE_EWTS}" | tr '[:lower:]' '[:upper:]')" && \
    rm -rf extern/SoilMoistureProfiles/cmake_build && \
    cmake -B extern/SoilMoistureProfiles/cmake_build -S extern/SoilMoistureProfiles/SoilMoistureProfiles/ \
      -DCMAKE_PREFIX_PATH=${EWTS_PREFIX} \
      -DUSE_EWTS="${USE_EWTS_NORMALIZED}" \
      -DNGEN=ON \
      -DCMAKE_C_COMPILER_LAUNCHER=ccache \
      -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
      -DBOOST_ROOT=/opt/boost && \
    cmake --build extern/SoilMoistureProfiles/cmake_build/ && \
    find /ngen-app/ngen/extern/SoilMoistureProfiles -name '*.o' -exec rm -f {} +

ARG SFT_CACHE_BUST=0
RUN --mount=type=cache,target=/root/.cache/ccache,id=ccache-bookworm \
    set -eux && \
    USE_EWTS="${USE_EWTS:-ON}" && \
    echo "SFT cache bust: ${SFT_CACHE_BUST}" && \
    echo "SFT USE_EWTS=${USE_EWTS}" && \
    USE_EWTS_NORMALIZED="$(echo "${USE_EWTS}" | tr '[:lower:]' '[:upper:]')" && \
    rm -rf extern/SoilFreezeThaw/cmake_build && \
    cmake -B extern/SoilFreezeThaw/cmake_build -S extern/SoilFreezeThaw/SoilFreezeThaw/ \
      -DCMAKE_PREFIX_PATH=${EWTS_PREFIX} \
      -DUSE_EWTS="${USE_EWTS_NORMALIZED}" \
      -DNGEN=ON \
      -DCMAKE_C_COMPILER_LAUNCHER=ccache \
      -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
      -DBOOST_ROOT=/opt/boost && \
    cmake --build extern/SoilFreezeThaw/cmake_build/ && \
    find /ngen-app/ngen/extern/SoilFreezeThaw -name '*.o' -exec rm -f {} +

ARG UEB_CACHE_BUST=0
RUN --mount=type=cache,target=/root/.cache/ccache,id=ccache-bookworm \
    set -eux && \
    USE_EWTS="${USE_EWTS:-ON}" && \
    echo "UEB cache bust: ${UEB_CACHE_BUST}" && \
    echo "UEB USE_EWTS=${USE_EWTS}" && \
    USE_EWTS_NORMALIZED="$(echo "${USE_EWTS}" | tr '[:lower:]' '[:upper:]')" && \
    rm -rf extern/ueb-bmi/cmake_build && \
    cmake -B extern/ueb-bmi/cmake_build -S extern/ueb-bmi/ \
      -DUEB_SUPPRESS_OUTPUTS=ON \
      -DCMAKE_PREFIX_PATH="${EWTS_PREFIX}" \
      -DUSE_EWTS="${USE_EWTS_NORMALIZED}" \
      -DBMICXX_INCLUDE_DIRS=/ngen-app/ngen/extern/bmi-cxx/ \
      -DCMAKE_C_COMPILER_LAUNCHER=ccache \
      -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
      -DBOOST_ROOT=/opt/boost && \
    cmake --build extern/ueb-bmi/cmake_build/ && \
    find /ngen-app/ngen/extern/ueb-bmi/ -name '*.o' -exec rm -f {} +

RUN --mount=type=cache,target=/root/.cache/pip,id=pip-cache-bookworm \
    set -eux; \
    cd extern/lstm; \
    python -m pip install .


# Build/install the t-route submodule.
# t-route's compiler scripts run make/pip commands inside the source tree, so a
# separate t-route cache mount (e.g. /root/.cache/t-route) does not help unless
# the scripts explicitly write build artifacts there. pip/uv caches are the
# useful reusable caches here.
RUN --mount=type=cache,target=/root/.cache/pip,id=pip-cache-bookworm \
    --mount=type=cache,target=/root/.cache/uv,id=uv-cache-bookworm \
    set -eux && \
    export CC="gcc" && \
    export CXX="g++" && \
    export F90="gfortran" && \
    export FC="gfortran" && \
    USE_EWTS="${USE_EWTS:-ON}" && \
    echo "T-Route USE_EWTS=${USE_EWTS}" && \
    USE_EWTS_NORMALIZED="$(echo "${USE_EWTS}" | tr '[:lower:]' '[:upper:]')" && \
    cd extern/t-route && \
    export LDFLAGS='-Wl,-L/usr/local/lib64/,-L/usr/local/lib/,-rpath,/usr/local/lib64/,-rpath,/usr/local/lib/' && \
    if [[ "${USE_EWTS_NORMALIZED}" =~ ^(ON|YES|TRUE|1)$ ]]; then \
        echo "Running compiler_bmi.sh (EWTS enabled)"; \
        ./compiler_bmi.sh no-e --gh-org "${GH_ORG}" --ewts-ref "${EWTS_REF}" ; \
    else \
        echo "Running compiler.sh (EWTS disabled)"; \
        ./compiler.sh no-e; \
    fi && \
    rm -rf /ngen-app/ngen/extern/t-route/test/LowerColorado_TX_v4 && \
    find /ngen-app/ngen/extern/t-route -name "*.o" -exec rm -f {} + && \
    find /ngen-app/ngen/extern/t-route -name "*.a" -exec rm -f {} +

# Copy the full ngen application source only after the standalone submodules
# above have been built. This keeps ordinary ngen code changes from invalidating
# the expensive submodule build layers while preserving .git/.gitmodules for the
# provenance extraction step below.
COPY . /ngen-app/ngen/

# topoflow-glacier uses setuptools-scm/VCS versioning and requires Git
# metadata to be available when `python -m pip install .` runs. Therefore this must
# be built after the full repository COPY step.
RUN --mount=type=cache,target=/root/.cache/pip,id=pip-cache-bookworm \
    set -eux; \
    cd extern/topoflow-glacier; \
    python -m pip install .

# Configure ngen using ccache for compiler caching.
# -DCMAKE_PREFIX_PATH=${EWTS_PREFIX} tells cmake where
# to find the ewtsConfig.cmake package file so that ngen's
# CMakeLists.txt line find_package(ewts CONFIG REQUIRED) succeeds.
# ngen links against ewts::ewts_ngen_bridge (the C++/MPI bridge).
RUN --mount=type=cache,target=/root/.cache/ccache,id=ccache-bookworm \
    set -eux && \
    USE_EWTS="${USE_EWTS:-ON}" && \
    echo "ngen USE_EWTS=${USE_EWTS}" && \
    USE_EWTS_NORMALIZED="$(echo "${USE_EWTS}" | tr '[:lower:]' '[:upper:]')" && \
    export FFLAGS="-fPIC" && \
    export FCFLAGS="-fPIC" && \
    export CMAKE_Fortran_FLAGS="-fPIC" && \
    rm -rf cmake_build && \
    cmake -B cmake_build -S . \
        -DCMAKE_PREFIX_PATH=${EWTS_PREFIX} \
        -DPython_EXECUTABLE="${VIRTUAL_ENV}/bin/python" \
        -DPython3_EXECUTABLE="${VIRTUAL_ENV}/bin/python" \
        -DUSE_EWTS="${USE_EWTS_NORMALIZED}" \
        -DNGEN_WITH_MPI=ON \
        -DNGEN_WITH_NETCDF=ON \
        -DNGEN_WITH_SQLITE=ON \
        -DNGEN_WITH_UDUNITS=ON \
        -DNGEN_WITH_BMI_FORTRAN=ON \
        -DNGEN_WITH_BMI_C=ON \
        -DNGEN_WITH_PYTHON=ON \
        -DNGEN_WITH_TESTS=OFF \
        -DNGEN_WITH_ROUTING=ON \
        -DNGEN_QUIET=ON \
        -DNGEN_UPDATE_GIT_SUBMODULES=OFF \
        -DCMAKE_C_COMPILER_LAUNCHER=ccache \
        -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
        -DBOOST_ROOT=/opt/boost && \
    cmake --build cmake_build --target all && \
    rm -rf /ngen-app/ngen/cmake_build/test/CMakeFiles && \
    rm -rf /ngen-app/ngen/cmake_build/src/core/CMakeFiles && \
    find /ngen-app/ngen/cmake_build -name "*.a" -exec rm -f {} + && \
    find /ngen-app/ngen/cmake_build -name "*.o" -exec rm -f {} +

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
    # Merge the main repository JSON and all submodule provenance JSON files into
    # a single metadata file. When EWTS is enabled, also include the EWTS provenance
    # JSON created during the optional EWTS build step above so
    #   cat /ngen-app/ngen_git_info.json
    # shows the exact EWTS version/build information included in the container.
    if [ -f /ngen-app/nwm-ewts_git_info.json ]; then \
        jq -s 'add' $GIT_INFO_PATH /ngen-app/submodules-json/*.json /ngen-app/nwm-ewts_git_info.json > /ngen-app/merged_git_info.json; \
    else \
        jq -s 'add' $GIT_INFO_PATH /ngen-app/submodules-json/*.json > /ngen-app/merged_git_info.json; \
    fi && \
    mv /ngen-app/merged_git_info.json $GIT_INFO_PATH && \
    rm -rf /ngen-app/submodules-json /ngen-app/nwm-ewts_git_info.json

# Make LSTM source-tree modules importable at runtime.
# Do not add venv site-packages; the active venv already handles installed packages.
ENV PYTHONPATH="/ngen-app/ngen/extern/lstm:/ngen-app/ngen/extern/lstm/lstm"

# Make Topoflow-Glacier source-tree modules importable at runtime.
ENV PYTHONPATH="${PYTHONPATH}:/ngen-app/ngen/extern/topoflow-glacier:/ngen-app/ngen/extern/topoflow-glacier/src/topoflow-glacier"

WORKDIR /

ENTRYPOINT [ "/ngen-app/bin/run-ngen.sh" ]
CMD [ "--info" ]
