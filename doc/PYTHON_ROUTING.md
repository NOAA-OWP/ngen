# Python Routing

## Summary

[t-route](https://github.com/NOAA-OWP/t-route) is the routing framework developed by NOAA-OWP.

See [Setting up t-route source](DEPENDENCIES.md#t-route) for details on aquiring the t-route submodule.
You will also need to [set up pybind11](DEPENDENCIES.md#pybind11) to use t-route.

## Setup Virtual Environment

Since t-route is set of Python modules, it will need to be installed in the Python environment ngen will be running with. Below are recommended steps to accomplish this: 

1. From the ngen project root (if this virtual environment exists, you may skip this step.)

```sh
python3 -m venv venv
```

2. Activate the virtual environment and install/update a few prerequisites:

```sh
source venv/bin/activate
pip install -U pip deprecated pyarrow geopandas tables
```

## Install t-route

[Compile and install t-route](https://github.com/NOAA-OWP/t-route#usage-and-testing) following the instructions from the t-route repository.
Ensure that you install the t-route modules in the virtual environment from step 2 of the Setup Virtual Environment section.
The t-route source can be downloaded and placed anywhere, it is only important that the Python modules are installed in the right virtual environment.

## Installation Caveats

### Compilers and Libraries

The t-route compiler script, `compiler.sh`, compiles and links `C` and `Fortran` code that will run _within_ an `ngen` process.
Ensure that compiler paths and flags, include paths, library paths, and other build time environment variables match the settings used to compile `ngen` to avoid conflicting dependencies and undefined behavior.

Note, the t-route extension modules rely on netcdf fortran, and thus they need to be compiled with the same fortran compiler that compiled
the netcdf library.  For example, if `libnetcdff` was provided by the RHEL7 package `netcdf-fortran-openmpi-static-4.2-16.el7.x86_64`
which was compiled with the openmpi fortran compiler, you will have to set the `FC` environment variable appropriately before executing the t-route `compiler.sh` script, like so:

```sh
FC=mpif90 ./compiler.sh
```

### Default installation is in development mode (impacts macOS)

The `compiler.sh` script will install the Python modules with `-e`. On macOS, you may need to re-install the modules in t-route's `src` directory directly after running `compiler.sh`.

### Some tips on installation if you run into issues
  * [On install mpi4py](https://github.com/Unidata/netcdf4-python/issues/1296)

  * [On pip version](https://github.com/NOAA-OWP/t-route/issues/621)

  * [On NetCDF version](https://github.com/NOAA-OWP/t-route/issues/705)

[Additional documentation for configuration and dependencies of t-route](https://github.com/NOAA-OWP/t-route#configuration-and-dependencies).
 
## Using t-route with ngen
  * Create the build directory including the options to activate Python and Routing: 

      * Activate Python flag with `-DNGEN_WITH_PYTHON:BOOL=ON`

      * Activate Routing flag with `-DNGEN_WITH_ROUTING:BOOL=ON.`  

      * An example create build directory command with the above two options activated:

    ```sh
      cmake -B cmake_build -DNGEN_WITH_PYTHON:BOOL=ON -DNGEN_WITH_ROUTING:BOOL=ON -DNGEN_WITH_TESTS:BOOL=ON .
    ```  
  
  * Unit tests for the Routing_Py_Adapter class can then be built and run from the main directory with the following two commands:
  
    ```sh
      cmake --build cmake_build --target test_routing_pybind
      ./cmake-build-debug/test/test_routing_pybind
    ```
  * An [example realization config](../data/gauge_01073000/example_bmi_multi_realization_config_w_routing.json) with routing inputs.

### Realization Config

To enable routing in a simulation realization config file, a `routing` block should appear with a path to the t-route configuration file at the same level as the `time` configuration, like so:

```JSON
...
"time": {
    "start_time": "2015-12-01 0:00:00",
    "end_time": "2015-12-30 23:00:00",
    "output_interval": 3600
},
"routing": {
    "t_route_config_file_with_path": "./data/gauge_01073000/routing_config.yaml"
}
...
```

### Routing Config

t-route uses a yaml input file for configuring the routing setup, see the [t-route repo documentation](https://github.com/NOAA-OWP/t-route#configuration) for more information.  An [example configuration](../data/gauge_01073000/routing_config.yaml) file is included in the example data.

### Configuration considerations for t-route with ngen

Output from ngen is currently created on an hourly basis and in files per nexus, which is different from t-route's native processing expectations. To account for this, currently t-route preprocesses the ngen nexus output CSV files before running. To ensure this happens correctly, these settings *must* be correct in the configuration YAML:

```YAML
    # These examples assume a 720h (30 day) simulation:
    forcing_parameters:
        # t-route's internal timestep in seconds
        dt                          : 300
        # ngen's timestep divided by t-route's (e.g. 3600/300)
        qts_subdivisions            : 12 
        # total simulation t-route timesteps (e.g. 12 per hour, 288 per day)
        nts                         : 8640
        # number of external (ngen) timesteps
        max_loop_size               : 720
        # The location to find the nex-* CSV files
        qlat_input_folder           : ./ 
        nexus_input_folder          : ./
        # The glob pattern to match nexus output files - MUST NOT CHANGE!
        qlat_file_pattern_filter    : "nex-*"
        nexus_file_pattern_filter   : "nex-*"
        # A directory where the temporary *.parquet files will be stored
        binary_nexus_file_folder    : /tmp
```
IMPORTANT: See the #known-issues below!

## Running t-route separately with ngen output

In some cases it may be useful to run the routing step separately. To do so, after installing t-route in your environment as described above, execute it directly this way:

```sh
python -m nwm_routing -V4 -f /path/to/routing_config.yaml
```


This is particularly useful if a long simulation completes in ngen but fails in t-route. Running routing this way will also often give more detailed error messages, if you are experiencing problems during the routing phase.

## Known issues

### Cleanup of `*.parquet` files required

Running t-route with ngen `nex-*.csv` input will generate hourly files with names matching `*.parquet` in the directory specified by `binary_nexus_file_folder` but it *does not remove them after the simulation compeltes*, and the presence of these files will prevent t-route from running. To run a simulation a second time, you will need to manually remove the created `*.parquet` files.

### Bug in multiprocessing on macOS

It is not currently possible to use multiprocessing in t-route on macOS as part of an ngen simulation directly. To use routing on macOS, either:

1. Run t-route in ngen with the `routing` block in the realization config and ensure that the t-route configuration specifies `cpu_pool: 1` to disable multiprocessing,

OR

2. Run t-route separately, after the ngen simulation as described above.

At present, running within ngen with `cpu_pool` > 1 will result in spawning many additional ngen processes, consuming lots of resources and likely corrupting your output! See #505 .
