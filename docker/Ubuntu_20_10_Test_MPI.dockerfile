FROM ubuntu:20.10 as builder

RUN apt-get update && apt-get install -y git gcc g++ make python3 wget cmake

RUN git clone https://github.com/NOAA-OWP/ngen.git

WORKDIR ngen

ENV CXX=/usr/bin/g++

RUN git submodule update --init --recursive -- test/googletest

RUN curl -L -o boost_1_79_0.tar.bz2 https://sourceforge.net/projects/boost/files/boost/1.79.0/boost_1_79_0.tar.bz2/download
RUN tar -xjf boost_1_79_0.tar.bz2
ENV BOOST_ROOT="boost_1_79_0"

RUN apt-get install -y libopenmpi-dev
RUN apt-get install -y openmpi-bin openmpi-common


RUN cmake -DCMAKE_BUILD_TYPE=Debug -B cmake-build-debug -DMPI_ACTIVE:BOOL=ON -S .
RUN cmake --build cmake-build-debug --target test_remote_nexus
#RUN mpirun --allow-run-as-root -np 2 ./test/test_remote_nexus

# We may want these things to happen in the container, rather than in the image
RUN echo "#!/bin/bash" > ./run_tests.sh \
    && echo "git pull --ff-only origin master" >> ./run_tests.sh \
    && echo "rm -rf cmake-build-debug" >> ./run_tests.sh \
    && echo "cmake -DCMAKE_BUILD_TYPE=Debug -B cmake-build-debug -DMPI_ACTIVE:BOOL=ON -S ." >> ./run_tests.sh \
    && echo "cmake --build cmake-build-debug --target test_remote_nexus" >> ./run_tests.sh \
    && echo "mpirun --allow-run-as-root -np 2 ./cmake-build-debug/test/test_remote_nexus" >> ./run_tests.sh \
    && chmod u+x ./run_tests.sh

CMD ["/bin/bash", "-l", "-c", "./run_tests.sh"]
