ARG ROCKYLINUX_TAG=8
FROM rockylinux:${ROCKYLINUX_TAG}

RUN dnf update -y \
    && dnf install -y dnf-plugins-core epel-release \
    && dnf repolist \
    && dnf install -y --allowerasing tar git gcc-toolset-12 make cmake udunits2-devel coreutils \
    && dnf clean all

# Rocky 8's system compiler is GCC 8, whose libstdc++ lacks complete C++17 <filesystem>
# support (notably std::hash<std::filesystem::path>). Build with the gcc-toolset-12 SCL
# toolchain instead, and point the runtime loader at its libstdc++.
ENV PATH="/opt/rh/gcc-toolset-12/root/usr/bin:${PATH}" \
    LD_LIBRARY_PATH="/opt/rh/gcc-toolset-12/root/usr/lib64:/opt/rh/gcc-toolset-12/root/usr/lib" \
    CC="/opt/rh/gcc-toolset-12/root/usr/bin/gcc" \
    CXX="/opt/rh/gcc-toolset-12/root/usr/bin/g++"

ARG BOOST_VERSION="1.86.0"
RUN export BOOST_ARCHIVE="boost_$(echo ${BOOST_VERSION} | tr '\.' '_').tar.gz" \
    && export BOOST_URL="https://sourceforge.net/projects/boost/files/boost/${BOOST_VERSION}/${BOOST_ARCHIVE}/download" \
    && cd / \
    && curl -L -o "${BOOST_ARCHIVE}" "${BOOST_URL}" \
    && tar -xzf "${BOOST_ARCHIVE}" \
    && rm ${BOOST_ARCHIVE}

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
