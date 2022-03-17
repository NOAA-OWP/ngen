FROM gitpod/workspace-full

ENV LIB=/home/linuxbrew/.linuxbrew/lib
ENV INCLUDE=/home/linuxbrew/.linuxbrew/include
ENV LD_LIBRARY_PATH=/home/linuxbrew/.linuxbrew/lib
ENV CC=/home/linuxbrew/.linuxbrew/bin/gcc-11
ENV CXX=/home/linuxbrew/.linuxbrew/bin/g++-11

RUN brew install boost udunits numpy netcdf
