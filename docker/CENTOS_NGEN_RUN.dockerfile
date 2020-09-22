FROM centos:8 as builder

RUN yum update -y
RUN yum install -y tar git gcc-c++ gcc make cmake python3 bzip2

RUN git clone https://github.com/NOAA-OWP/ngen.git 

WORKDIR ngen

ENV CXX=/usr/bin/g++

RUN git submodule update --init --recursive -- test/googletest

RUN curl -L -O https://dl.bintray.com/boostorg/release/1.72.0/source/boost_1_72_0.tar.bz2

RUN tar -xjf boost_1_72_0.tar.bz2

ENV BOOST_ROOT="boost_1_72_0"

WORKDIR /ngen

RUN cmake -B /ngen -S .

RUN cmake --build /ngen --target ngen

WORKDIR /ngen/

CMD ./ngen data/catchment_data.geojson "" data/nexus_data.geojson "" data/refactored_example_realization_config.json
