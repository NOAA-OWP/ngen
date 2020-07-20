FROM registry.access.redhat.com/ubi8/ubi as builder

RUN yum install -y tar git gcc-c++ gcc make cmake python3 bzip2

RUN git clone https://github.com/NOAA-OWP/ngen.git

WORKDIR ngen

ENV CXX=/usr/bin/gcc

RUN git submodule update --init --recursive -- test/googletest

RUn curl -L -O https://dl.bintray.com/boostorg/release/1.72.0/source/boost_1_72_0.tar.bz2

RUN tar -xjf boost_1_72_0.tar.bz2

ENV BOOST_ROOT="boost_1_72_0"

RUN mkdir cmake_build

WORKDIR /ngen

RUN cmake -B /ngen -S .

RUN cmake --build /ngen --target test_unit

FROM scratch

COPY --from=builder cmake_build .

ENTRYPOINT [./cmake_build/test/test_unit]
