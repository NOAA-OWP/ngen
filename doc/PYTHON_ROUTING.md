# Python Routing
- [Python Routing](#python-routing)
- [Summary](#summary)
- [Installing t-route](#installing-t-route)
- [Using t-route with ngen](#using-t-route-with-ngen)
  - [Routing Config](#routing-config)



# Summary

This describes how to use the Python-based t-route routing module with ngen. 

[t-route](https://github.com/NOAA-OWP/t-route) is the routing framework developed by NOAA-OWP.

See [Setting up t-route source](DEPENDENCIES.md#t-route) for details on aquiring the t-route submodule.
You will also need to [set up pybind11](DEPENDENCIES.md#pybind11) to use t-route.

# Installing t-route
These steps will cover installing the t-route package into a virtual environment in the ngen project.
From the project root (if this virutal environment exists, you may skip this step.)

```sh
mkdir .venv
python3 -m venv .venv
```

Activate the virtual environment and update a couple of tools.

```sh
source .venv/bin/activate
pip install -U pip setuptools cython dask
```

Install the routing driver modules.

```sh
pip install -e ngen/extern/t-route/src/ngen_routing/
pip install -e ngen/extern/t-route/src/nwm_routing/
```

Next, we need to build some python extension modules that the routing package requires.  A convience script is located in the t-route
package to help with this.

NOTE t-route extension modules rely on netcdf fortran, and thus they need to be compiled with the same fortran compiler that compiled
the netcdf library.  In the example below, `libnetcdff` was provided by the el7 package `netcdf-fortran-openmpi-static-4.2-16.el7.x86_64`
which was compiled with the openmpi fortran compiler.  So we have to set the `FC` environment variable appropriately before executing the script.  By default, gfortran is the selected fortran compiler.

```sh
pushd ngen/extern/t-route/src/python_routing_v02
F90=mpif90 ./compiler.sh
popd
```
This should compile all extension modules and `pip install -e` the various namespace package modules for the t-route framework and routing modules.

[Additional documentation for configuration and dependencies of t-route](https://github.com/NOAA-OWP/t-route#configuration-and-dependencies).  
 
# Using t-route with ngen
  * Create the build directory including the options to activate Python and Routing: 

      * Activate Python flag with `-DNGEN_ACTIVATE_PYTHON:BOOL=ON`

      * Activate Routing flag with `-DNGEN_ACTIVATE_ROUTING:BOOL=ON.`  

      * An example create build directory command with the above two options activated:

    ```sh
      cmake -DCMAKE_BUILD_TYPE=Debug -B cmake-build-debug -DNGEN_ACTIVATE_PYTHON:BOOL=ON -DNGEN_ACTIVATE_ROUTING:BOOL=ON .
    ```  
  
  * Unit tests for the Routing_Py_Adapter class can then be built and run from the main directory with the following two commands:
  
    ```sh
      cmake --build cmake-build-debug --target test_routing_pybind
      ./cmake-build-debug/test/test_routing_pybind
    ```  
  * An [example realization config](../data/example_bmi_multi_realization_config_w_routing.json) with routing inputs.

## Routing Config

t-route uses a yaml input file for configuring the routing setup.  An [example configuration](../data/ngen_routing.yaml) file is included in the example data.  In this file, the following keys should be set appropriately:
```yaml
supernetwork_parameters:
    title_string: "Ngen"
    #Below will change with new catchment route link
    geo_file_path: "<path_to_hydrofabric>/waterbody-params.json"
    #CHANGE BELOW WITH NEW NGEN HYDRO FABRIC DATA
     ngen_nexus_file: "<path_to_hydrofabric>/flowpath_edge_list.json"
#ngen output files
forcing_parameters:
    nexus_input_folder: "<path_to_ngen_output>"
    nexus_file_pattern_filter: "nex-*"
```
