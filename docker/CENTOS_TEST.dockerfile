FROM centos:8 as builder

RUN yum update -y
RUN yum install -y tar git gcc-c++ gcc make cmake python3 bzip2

RUN git clone https://github.com/NOAA-OWP/ngen.git

WORKDIR ngen

ENV CXX=/usr/bin/g++

RUN git submodule update --init --recursive -- test/googletest

RUN curl -L -o boost_1_86_0.tar.bz2 https://sourceforge.net/projects/boost/files/boost/1.79.0/boost_1_86_0.tar.bz2/download

RUN tar -xjf boost_1_86_0.tar.bz2

ENV BOOST_ROOT="boost_1_86_0"

WORKDIR /ngen

RUN cmake -B /ngen -S .

RUN cmake --build /ngen --target test_unit

WORKDIR /ngen

CMD ["/bin/bash", "-l", "-c", "./test_unit"]
