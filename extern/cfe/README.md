# About

This directory holds sources and build files for a C-language implementation of the CFE (or T-shirt) model that also fulfills the BMI specification.  The BMI fulfillment is strictly valid, though only an essential subset of functions are currently fully implemented.  

The build configuration is also set up to generate a shared library artifact that can be used in the main NextGen framework.  

# Building

To generate the shared library files, build the `cfemodel` target in the generated build system.  This needs to be separate from the main NextGen build system.

#### Generating a Build System Directory

Run from the project root directory:

    cmake -B extern/cfe/cmake_cfe_lib -S extern/cfe
    
To regenerate, simply remove the `extern/cfe/cmake_cfe_lib` directory and run the command again.
 
#### Building the Shared Library

Again, from the project root directory:

    cmake --build cmake_cfe_lib --target cfemodel

This should generate the appropriate library files for your system.
