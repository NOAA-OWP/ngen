# Python Routing

* [Summary](#summary)

## Summary

This describes how to use the Python-based t-route routing module with ngen. 

The basic outline of steps needed to run the Remote Nexus Class with MPI is:
  * [Set up pybind11] (https://github.com/robertbartel/ngen/blob/python/main/doc/DEPENDENCIES.md#pybind11)
    UPDATE ABOVE LINK WHEN MERGED INTO MASTER
 
  * Create the build directory including the options to activate Python and Routing: 
  
      `cmake -DCMAKE_BUILD_TYPE=Debug -B cmake-build-debug -DNGEN_ACTIVATE_PYTHON:BOOL=ON -DNGEN_ACTIVATE_ROUTING:BOOL=ON -DBMI_C_LIB_ACTIVE:BOOL=ON -S .`  
  
  * Unit tests for the Routing_Py_Adapter class can then be built and run from the main directory with the following two commands:
  
      `cmake --build cmake-build-debug --target test_routing_pybind`  <br />
      
      `./cmake-build-debug/test/test_routing_pybind`
  
An [example realization](https://github.com/jdmattern-noaa/ngen/blob/routing-py-adapter/data/example_realization_config_w_routing.json) with routing inputs.
    UPDATE ABOVE LINK WHEN MERGED INTO MASTER
