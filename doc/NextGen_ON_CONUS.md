# NextGen on CONUS

This documentation provides instructions on all neccessary steps and components to run NextGen jobs at CONUS scale. Considering the computations large scale, we focus only on running parallel jobs using MPI.

* [Summary](#summary)
* [Download the Codes](#doenload-the-codes)
* [Setting Up the Environment](#setting-up-the-environment)
* [Build the Executable](#build-the-executable)
* [Generate Partition For Parallel Computation](#generate-partition-for-parallel-computation)
* [CONUS Hydrofabric](#CONUS-hydrofabric)
* [Prepare the Input Data](#prepare-the-input-data)
* [Build the Realization Configurations](#build-the-realization-configurations)
* [Run Computations with submodules](#run-computations-with-submodules)
* [Run Computation with Topmodel](#run-computation-with-topmodel)
* [Run Computation with Routing](#run-computation-with-routing)

# Summary

This is a tutorial-like documentation. We provide suficient details in the hope that by following this document step by step, you can run NextGen computations from simple to sophisticated realization that models simple test examples to realistic cases. Throughout this document, we assume a Linux operating system environment.

# Download the Codes

To download the `ngen` source code, run the following commands:

`git clone https://github.com/NOAA-OWP/ngen.git`
`cd ngen`

Then we need all the submodule codes. So run the command below:

`git submodule update --init --recursive`

# Setting up the Environment

For setting up the build and computation environment, we refer the users to our documentation chapter [DEPENDENCIES.md](DEPENDENCIES.md) for details. Basically, you will need to have access to C/C++ compiler, MPI, Boost, NetCDF, Cmake, SQLite3. Some of them may already be on your system. Otherwise, you have to install your own version. There are also some required software packages that come with `ngen` as submodules, such as `Udunits libraries`, `pybind11`, and `iso_c_fortran_bmi`. 

You are most likely need to use Python. For that we recommend setting up a virtual environment. For details, see [PYTHON_ROUTING.md](PYTHON_ROUTING.md). After setting up the Python virtual environment and activating it, you may need install additional python modules depending what `ngen submodules` you want to run.

# Build the Executable

After setting up the environment variables, we need first build the necessay dynamically linked librares. Although `ngen` has capability for automated building of submodule libraries, we build them explicitly so that users have a better understanding. For simplicity, we display the content a script which we name it `build_libs`.

```
cmake -B extern/sloth/cmake_build -S extern/sloth && \
make -C extern/sloth/cmake_build && \
cmake -B extern/cfe/cmake_build -S extern/cfe/cfe/ -DNGEN=ON && \
make -C extern/cfe/cmake_build && \
cmake -B extern/topmodel/cmake_build -S extern/topmodel && \
make -C extern/topmodel/cmake_build && \
cmake -B extern/iso_c_fortran_bmi/cmake_build -S extern/iso_c_fortran_bmi && \
make -C extern/iso_c_fortran_bmi/cmake_build && \
cmake -B extern/noah-owp-modular/cmake_build -S extern/noah-owp-modular -DNGEN_IS_MAIN_PROJECT=ON && \
make -C extern/noah-owp-modular/cmake_build && \
cmake -B extern/evapotranspiration/evapotranspiration/cmake_build -S extern/evapotranspiration/evapotranspiration && \
make -C extern/evapotranspiration/evapotranspiration/cmake_build && \
cmake -B extern/sloth/cmake_build -S extern/sloth && \
make -C extern/sloth/cmake_build && \
cmake -B extern/SoilFreezeThaw/SoilFreezeThaw/cmake_build -S extern/SoilFreezeThaw/SoilFreezeThaw -DNGEN=ON && \
cmake --build extern/SoilFreezeThaw/SoilFreezeThaw/cmake_build --target sftbmi -- -j 2 && \
cmake -B extern/SoilMoistureProfiles/SoilMoistureProfiles/cmake_build -S extern/SoilMoistureProfiles/SoilMoistureProfiles -DNGEN=ON && \
cmake --build extern/SoilMoistureProfiles/SoilMoistureProfiles/cmake_build --target smpbmi -- -j 2 &&
```

Copy the content into the file named `build_libs` and run the command:

```
source build_libs
```

This will build all libraries we need to run `ngen` at the time of this writing.

Then, with the Python virtual environment activated, we can build the MPI executable using the following script:

```
cmake -S . -B cmake_build_mpi -DCMAKE_C_COMPILER=/local/lib/bin/mpicc -DCMAKE_CXX_COMPILER=/local/lib/bin/mpicxx \
-DBOOST_ROOT=/home/shengting.cui/usr/boost_1_79_0/ \
    -DNetCDF_ROOT=<path-to-NetCDF-ROOT-dir> \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DNGEN_IS_MAIN_PROJECT=ON \
    -DNGEN_WITH_MPI:BOOL=ON                      \
    -DNGEN_WITH_NETCDF:BOOL=ON                   \
    -DNGEN_WITH_SQLITE:BOOL=ON                   \
    -DNGEN_WITH_UDUNITS:BOOL=ON                  \
    -DNGEN_WITH_BMI_FORTRAN:BOOL=ON              \
    -DNGEN_WITH_BMI_C:BOOL=ON                    \
    -DNGEN_WITH_PYTHON:BOOL=ON                   \
    -DNGEN_WITH_ROUTING:BOOL=OFF                 \
    -DNGEN_WITH_TESTS:BOOL=ON                    \
    -DNGEN_QUIET:BOOL=ON                         \
    -DNGEN_WITH_EXTERN_SLOTH:BOOL=ON             \
    -DNGEN_WITH_EXTERN_TOPMODEL:BOOL=OFF         \
    -DNGEN_WITH_EXTERN_CFE:BOOL=OFF              \
    -DNGEN_WITH_EXTERN_PET:BOOL=OFF              \
    -DNGEN_WITH_EXTERN_NOAH_OWP_MODULAR:BOOL=ON
cmake --build cmake_build_mpi --target all -j 8
```

For the meaning of each option in the script, see `ngen/wiki` [build](https://github.com/NOAA-OWP/ngen/wiki/Building) page.

Suppose the above script is named `build_mpi`, execute the following command to build:

`source build_mpi`

This will build an executable in the `cmake_build_mpi` directory named `ngen` and another named `partitionGenerator` as well as all the unit tests in the `cmake_build_mpi/test`.

# Generate Partition For Parallel Computation

For parallel computation using MPI on hydrofabric, a [partition generate tool](Distributed_Processing.md) is used to partition the hydrofabric features ids into a number of partitions equal to the number of MPI processing CPU cores. To generate the partition file, run the following command:

```
./cmake-build_mpi/partitionGenerator ./hydrofabric/conus.gpkg ./hydrofabric/conus.gpkg ./partition_config_32.json 32 '' ''
```

In the command above, `conus.gpkg` is the NextGen hydrofabric version 2.01 for CONUS, `partition_config_32.json` is the partition file that contains all features ids and their interconnected network information. The number `32` is intended number of processing cores for running parallel build `ngen` using MPI. The last two empty strings, as indicated by `''`, indicate there is no subsetting, i.e., we intend to run the whole CONUS hydrofabric.

# CONUS Hydrofabric

The CONUS hydrofabric is downloaded from [here](https://www.lynker-spatial.com/#v20.1/). The file name under the list is `conus.gpkg`. It is cautioned that since the data there are evolving and newer version may be available in the future. When using a newer version, be mindful that the corresponding initial configuration file generation and validation for all submodules at CONUS scale are necessary, which may be a non-trivial process due to the shear size of the spatial scale.

As the file is fairly large, it is worth some consideration to store it in a proper place, then simply build a symbolic link in the `ngen` home directory, thus named `./hydrofabric/conus.gpkg`. Note the easiest way to create the symbolic link is to `makedir hydrofabric` and then create the full path.

# Prepare the Input Data

Input data include the forcing data and initial parameter data for various submodules. These depend on what best suit the user need. For our case, as of this documentation, beside forcing data, which can be accessed at `./forcing/NextGen_forcing_2016010100.nc` using the symbolic link scheme, we also generated initial input data for various submodules `noah-owp-modular`, `PET`, `CFE`, `SoilMoistureProfiles (SMP)`, `SoilFreezeThaw (SFT)`. The first three are located in `./conus_config/`, the SMP initial configus are located in `./conus_smp_configs/` and the SFT initial configs are located in `./conus_sft_configs/`.

For code used to generate the initial config files for the various modules, the interested users are directed to this [web location](https://github.com/NOAA-OWP/ngen-cal/tree/master/python/ngen_config_gen). 

The users are warned that since the simulated region is large, some of the initial config parameters values for some catchments may be unsuitable and cause the `ngen` execution to stop due to errors. Usually, in such cases, either `ngen` or the submodule itself may provide some hint as to the catchment ids or the location of the code that caused the error. Users may follow these hints to figure out as to which initial input parameter or parameters are initialized with inappropriate values. In the case of SFT, an initial value of `smcmax=1.0` would be too large. In the case of SMP, an initial value of `b=0.01` would be too small, for example.

# Build the Realization Configurations

The realization configuration file in `Json` format contains high level information to run a `ngen` simulation, such as interconnected submodules, paths to forcing file, shared libraries, initialization parameters, duration of simulation, I/O variables, etc. We have built the realization configurations for several commonly used submodules which are located in `data/baseline/`. These are built by adding one submodule at a time, test run for 10 days simulation time. The successive submodules used are:
```
sloth (conus_bmi_multi_realization_config_w_sloth.json)
sloth+noah-owp-modular (conus_bmi_multi_realization_config_w_sloth_noah.json)
sloth+noah-owp-modular+pet (conus_bmi_multi_realization_config_w_sloth_noah_pet.json)
sloth+noah-owp-modular+pet+cfe (conus_bmi_multi_realization_config_w_sloth_noah_pet_cfe.json)
sloth+noah-owp-modular+pet+smp (conus_bmi_multi_realization_config_w_sloth_noah_pet_smp.json)
sloth+noah-owp-modular+pet+smp+sft (conus_bmi_multi_realization_config_w_sloth_noah_pet_smp_sft.json)
sloth+noah-owp-modular+pet+smp+sft+cfe (conus_bmi_multi_realization_config_w_sloth_noah_pet_smp_sft_cfe.json)
```

# Run Computations with Submodules

With all preparation steps completed, we are now ready to run computations. We use MPI as our parallel processing application with 32 cores as an example. Users are free to choose whatever number cores they want, just make sure you will need to have the appropriate corresponding partition json file for the number of cores used. The command line for running a MPI job is as sollows:

For a simple example run and quick turn around, you can run:

```
run -n 32 ./cmake_build_mpi/ngen ./hydrofabric/conus.gpkg '' ./hydrofabric/conus.gpkg '' data/baseline/conus_bmi_multi_realization_config_w_sloth.json conus_partition_32.json
```

For a more substantial example simulation, you can run:

```
run -n 32 ./cmake_build_mpi/ngen ./hydrofabric/conus.gpkg '' ./hydrofabric/conus.gpkg '' data/baseline/conus_bmi_multi_realization_config_w_sloth_noah.json conus_partition_32.json
```

For an example taken into account more realistic contributions, you can try:
```
run -n 32 ./cmake_build_mpi/ngen ./hydrofabric/conus.gpkg '' ./hydrofabric/conus.gpkg '' data/baseline/conus_bmi_multi_realization_config_w_sloth_noah_pet_smp_sft_cfe.json conus_partition_32.json
```

where `ngen` is the executable we build in the [Building the Executable](#Building the Executable) section. All other terms have been discussed above in details. With the current existing realization config files, the above jobs run 10 days simulation time on CONUS scale.

Be aware that the above commands will generate over a million output files associated with catchment and nexus ids so if you issue a `ls` command in `ngen` directory, it will be significantly slower than usual to list all the file names. The exact time will depend on the computer you are working on.

# Run Computation with Topmodel

To be added

# Run Computation with Routing

To be added
