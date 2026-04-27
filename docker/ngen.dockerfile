ARG ROCKYLINUX_TAG=8
FROM rockylinux:${ROCKYLINUX_TAG}

RUN dnf update -y \
    && dnf install -y dnf-plugins-core epel-release \
    && dnf repolist \
    && dnf install -y --allowerasing tar git gcc-c++ gcc make cmake udunits2-devel coreutils \
    && dnf clean all

ARG BOOST_VERSION="1.86.0"
# Boost is mostly used header-only in this project, but the BMI
# serialization protocol library links against `libboost_serialization`
# (the first compiled-Boost dependency). Building it here against the
# extracted source tree leaves the libraries under
# `<BOOST_ROOT>/stage/lib/` where CMake's FindBoost picks them up
# automatically — no CMake-side change is required when BOOST_ROOT is
# already pointed at this directory below.
RUN export BOOST_ARCHIVE="boost_$(echo ${BOOST_VERSION} | tr '\.' '_').tar.gz" \
    && export BOOST_DIR="boost_$(echo ${BOOST_VERSION} | tr '\.' '_')" \
    && export BOOST_URL="https://sourceforge.net/projects/boost/files/boost/${BOOST_VERSION}/${BOOST_ARCHIVE}/download" \
    && cd / \
    && curl -L -o "${BOOST_ARCHIVE}" "${BOOST_URL}" \
    && tar -xzf "${BOOST_ARCHIVE}" \
    && rm ${BOOST_ARCHIVE} \
    && cd "${BOOST_DIR}" \
    && ./bootstrap.sh --with-libraries=serialization \
    && ./b2 --with-serialization

COPY . /ngen
WORKDIR /ngen
RUN git submodule update --init --recursive -- test/googletest
RUN git submodule update --init --recursive -- extern/pybind11

RUN cmake -S . \
          -B /ngen_build \
          -DBOOST_ROOT="/boost_$(echo ${BOOST_VERSION} | tr '\.' '_')" \
          -DBoost_NO_BOOST_CMAKE:BOOL=TRUE \
          -DNGEN_WITH_MPI:BOOL=OFF \
          -DNGEN_WITH_NETCDF:BOOL=OFF \
          -DNGEN_WITH_SQLITE:BOOL=OFF \
          -DNGEN_WITH_UDUNITS:BOOL=ON \
          -DNGEN_WITH_BMI_FORTRAN:BOOL=OFF \
          -DNGEN_WITH_BMI_C:BOOL=OFF \
          -DNGEN_WITH_PYTHON:BOOL=OFF \
          -DNGEN_WITH_TESTS:BOOL=ON \
          -DNGEN_QUIET:BOOL=ON \
          -DNGEN_WITH_EXTERN_SLOTH:BOOL=ON

RUN cmake --build /ngen_build \
          --target testbmicppmodel ngen \
          -- \
          -j $(nproc)

WORKDIR /ngen_build
CMD ["./ngen"]
