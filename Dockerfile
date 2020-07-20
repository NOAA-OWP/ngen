FROM registry.access.redhat.com/ubi8/ubi as builder

RUN yum install -y git gcc-g++ gcc make cmake

RUN git clone https://github.com/NOAA-OWP/ngen.git

WORKDIR ngen

RUN git submodule update --init --recursive -- test/googletest

RUN cmake CMAKE_CXX_COMPOILER=/usr/bin/g++ -B cmake_build -S .

RUN cmake --build cmake_build --target test_unit

FROM scratch

COPY --from=builder cmake_build .

ENTRYPOINT [./cmake_build/test/test_unit]
