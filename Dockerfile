FROM registry.access.redhat.com/ubi8/ubi as builder

RUN yum install -y git gcc cmake

RUN git submodule update --init --recursive -- test/googletest
RUN git clone https://github.com/NOAA-OWP/ngen.git

RUN cmake --build cmake_build --target test_unit


ENTRYPOINT [./cmake_build/test/test_unit]
