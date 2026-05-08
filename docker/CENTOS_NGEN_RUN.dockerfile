FROM rockylinux:8 as builder

RUN yum update -y \
    && yum install -y dnf-plugins-core \
#    && yum -y config-manager --set-enabled powertools \
    && yum -y install epel-release \
    && yum repolist \
    && yum install -y tar git gcc-c++ gcc make cmake python38 python38-devel python38-numpy bzip2 udunits2-devel \
    && dnf clean all \
  	&& rm -rf /var/cache/yum

RUN curl -L -o boost_1_86_0.tar.bz2 https://sourceforge.net/projects/boost/files/boost/1.86.0/boost_1_86_0.tar.bz2/download \
    && tar -xjf boost_1_86_0.tar.bz2 \
    && rm boost_1_86_0.tar.bz2

ENV BOOST_ROOT="/boost_1_86_0"

ENV CXX=/usr/bin/g++

# Note that this may need to be temporarily used during development instead of the COPY below, to prevent any issues
# cause by a local checkout being different than a fresh checkout (as is used in the Github Actions execution)
#RUN git clone https://github.com/NOAA-OWP/ngen.git
COPY . /ngen/

WORKDIR /ngen

RUN git submodule update --init --recursive -- test/googletest

RUN git submodule update --init --recursive -- extern/pybind11

WORKDIR /ngen

RUN cmake -DNGEN_WITH_NETCDF:BOOL=OFF \
          -DNGEN_WITH_SQLITE:BOOL=OFF \
          -B /ngen -S .

RUN cmake --build . --target ngen

WORKDIR /ngen/

CMD ./ngen data/catchment_data.geojson "" data/nexus_data.geojson "" data/example_realization_config.json
