# Python Routing

* [Summary](#summary)

## Summary

This describes how to use the Python-based t-route routing module with ngen. 

[t-route](https://github.com/NOAA-OWP/t-route) is the routing framework developed by NOAA-OWP.

The basic outline of steps needed to run Python Routing is:
  * [Set up pybind11](DEPENDENCIES.md#pybind11)

  * The t-route module is handled as a Git Submodule, located at extern/t-route. To initialize the submodule:
  `git submodule update --init extern/t-route`

  * Navigate to extern/t-route and [follow these steps for configuration and dependencies of t-route](https://github.com/NOAA-OWP/t-route#configuration-and-dependencies).  
 
  * Create the build directory including the options to activate Python and Routing: 

      * Activate Python flag with `-DNGEN_ACTIVATE_PYTHON:BOOL=ON`

      * Activate Routing flag with `-DNGEN_ACTIVATE_ROUTING:BOOL=ON.`  

      * An example create build directory command with the above two options activated:

      ```
      cmake -DCMAKE_BUILD_TYPE=Debug -B cmake-build-debug -DNGEN_ACTIVATE_PYTHON:BOOL=ON -DNGEN_ACTIVATE_ROUTING:BOOL=ON .
    ```  
  
  * Unit tests for the Routing_Py_Adapter class can then be built and run from the main directory with the following two commands:
  
    ```
    cmake --build cmake-build-debug --target test_routing_pybind
    
    ./cmake-build-debug/test/test_routing_pybind
    ```
  
An [example realization](example_realization_config_w_routing.json) with routing inputs.
