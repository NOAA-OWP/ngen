# Installation instructions

At present, there is no supported standard installation process.

---

## Building and running with docker:

The quickest way to build and run NGEN is to follow what we do for our [model run testing](./docker/CENTOS_NGEN_RUN.dockerfile):

First we make a new directory:
`mkdir ngen && cd ngen`

Then clone the github repository:
`git clone https://github.com/NOAA-OWP/ngen.git`

Then we can build with docker:
`docker build . --file ./docker/CENTOS_NGEN_RUN.dockerfile --tag localbuild/ngen:latest`

## Building manually:

**Install the Linux required packages (package names shown for Centos 8.4.2105):**

`
yum install tar git gcc-c++ gcc make cmake python38 python38-devel python38-numpy bzip2 udunits2-devel texinfo
`

**Make a directory to contain all the NGEN items:**

`
mkdir ngen && cd ngen
`


**Download the Boost Libraries:**

```shell
curl -L -O https://boostorg.jfrog.io/artifactory/main/release/1.72.0/source/boost_1_72_0.tar.bz2 \
    && tar -xjf boost_1_72_0.tar.bz2 \
    && rm boost_1_72_0.tar.bz2
```

**Set the ENV for Boost and C compiler:**

```shell
set BOOST_ROOT="/boost_1_72_0"
set CXX=/usr/bin/g++
```

**Get the git submodules:**

```shell
git submodule update --init --recursive -- test/googletest 
git submodule update --init --recursive -- extern/pybind11
```

**Build NGEN using cmake:**

```shell
cmake -B /ngen -S . &&
cmake --build . --target ngen
```

**Running the Model:**

```shell
./ngen data/catchment_data.geojson "" data/nexus_data.geojson "" data/example_realization_config.json
```

---

**Further information on each step:**

Make sure all necessary [dependencies](doc/DEPENDENCIES.md) are installed, and then [build the main ngen target with CMake](doc/BUILDS_AND_CMAKE.md).  Then run the executable, as described [here for basic use](README.md#usage) or [here for distributed execution](doc/DISTRIBUTED_PROCESSING.md#examples).
