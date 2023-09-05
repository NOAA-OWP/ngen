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

```sh
yum install tar git gcc-c++ gcc make cmake python38 python38-devel python38-numpy bzip2 udunits2-devel texinfo
```

**Clone the NGen repository:**

```sh
git clone https://github.com/NOAA-OWP/ngen
cd ngen
```

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

**Configure NGen using cmake:**

In addition to normal CMake options, the following `ngen` configuration options are available:

Option                | Description
--------------------- | -----------
NETCDF_ACTIVE         | Include NetCDF support
NGEN_WITH_SQLITE3     | Include SQLite3 support (GeoPackage support)
UDUNITS_ACTIVE        | Include UDUNITS support
MPI_ACTIVE            | Include MPI (Parallel Execuation) support
BMI_FORTRAN_ACTIVE    | Include Fortran BMI support
BMI_C_LIB_ACTIVE      | Include C BMI support
NGEN_ACTIVATE_PYTHON  | Include Python support
NGEN_ACTIVATE_ROUTING | Include `t-route` integration

> These can be included by adding `-D{Option}=[ON/OFF]` to the following CMake command.

> **Note**: if `-DNGEN_ACTIVATE_PYTHON=ON`, and you are using an *activated* virtual environment, then CMake will bind its Python configuration to your virtual environment. This means, when you run the `ngen` executable, your virtual environment must be activated.

The following CMake command will configure the build:

```shell
cmake -DCMAKE_CXX_COMPILER=/usr/bin/g++ \
      -DBOOST_ROOT=boost_1_72_0 \
      -B /build \
      -S .
```

**Build NGen using cmake:**

```shell
cmake --build build --target ngen
```

If you want to build with multiple jobs (in parallel), then you can run the following instead:

```shell
# replace 2 with number of jobs
cmake --build build --target ngen -- -j 2
```

**Running the Model:**

```shell
./build/ngen data/catchment_data.geojson all data/nexus_data.geojson all data/example_realization_config.json
```

---

**Further information on each step:**

Make sure all necessary [dependencies](doc/DEPENDENCIES.md) are installed, and then [build the main ngen target with CMake](doc/BUILDS_AND_CMAKE.md).  Then run the executable, as described [here for basic use](README.md#usage) or [here for distributed execution](doc/DISTRIBUTED_PROCESSING.md#examples).
