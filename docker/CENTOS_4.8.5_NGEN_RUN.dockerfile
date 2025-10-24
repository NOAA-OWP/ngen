FROM centos:7.8.2003 as builder

# ----------- Get packages -----------
RUN yum install -y tar git make gcc gcc-c++ glibc-devel python3 bzip2

RUN yum update -y
RUN yum clean all
# ----------- CMAKE 3.10 -----------
RUN curl -o cmake-3.10.3.tar.gz https://cmake.org/files/v3.10/cmake-3.10.3.tar.gz

RUN tar zxvf cmake-3.* && cd cmake-3.* && ./bootstrap --prefix=/usr/local && make -j$(nproc) && make install
# ----------- Get nGen -----------

RUN git clone https://github.com/NOAA-OWP/ngen.git 

WORKDIR ngen

ENV CXX=/usr/bin/g++

RUN git submodule update --init --recursive -- test/googletest

RUN curl -L -o boost_1_86_0.tar.bz2 https://sourceforge.net/projects/boost/files/boost/1.86.0/boost_1_86_0.tar.bz2/download

RUN tar -xjf boost_1_86_0.tar.bz2

ENV BOOST_ROOT="boost_1_86_0"

RUN cmake -B /ngen -S .

RUN cmake --build /ngen --target ngen

# ----------- Run nGEn -----------
CMD ./ngen data/catchment_data.geojson "" data/nexus_data.geojson "" data/example_realization_config.json
