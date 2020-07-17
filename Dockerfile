FROM registry.access.redhat.com/ubi7/ubi as builder

RUN yum install -y git gcc

RUN git submodule update --init --recursive -- test/googletest
RUN git clone https://github.com/NOAA-OWP/ngen.git
#Currently know this wont work. Need to get cmake working in UBI
RUN cmake --build cmake_build --target test_unit


ENTRYPOINT [./cmake_build/test/test_unit]
