# Test BMI Model C Implementation

* [About](#about)
* [Building](#building)
    * [Generating a Build System Directory](#generating-a-build-system-directory)
    * [Building the Shared Library](#building-the-shared-library)
* [Implementation Details](#implementation-details)

# About

This is a implementation of a C-based model that fulfills the C language BMI interface and can be built into a shared library.  It is intended to serve as a control for testing purposes, freeing the framework from dependency on any real-world model in order to test BMI related functionality.

# Building

To generate the shared library files, build the `testbmicmodel` target in the generated build system.  This needs to be separate from the main NextGen build system.

#### Generating a Build System Directory

Run from the project root directory:

    cmake -B extern/test_bmi_c/cmake_build -S extern/test_bmi_c

To regenerate, simply remove the `extern/test_bmi_c/cmake_build` directory and run the command again.

#### Building the Shared Library

Again, from the project root directory:

    cmake --build extern/test_bmi_c/cmake_build --target testbmicmodel

This should generate the appropriate library files for your system.

# Implementation Details

As a reminder, the BMI spec for C is implemented as a C struct declaration.  The struct has a `void*` member for holding a pointer to some data structure for the model (here, a `test_bmi_c_model` struct, declared in [include/test_bmi_c.h](include/test_bmi_c.h)), and a series of function pointers to the functions necessary to fulfill BMI.  A separate function in the implementing model library - here `register_bmi()` in [src/bmi_test_bmi_c.c](src/test_bmi_c.c) - must then set those function pointers to backing function definitions.

For this test model, the implemented functions in general follow the BMI documented spec, so in most cases that [documentation](https://bmi.readthedocs.io/en/latest/) is sufficient for understanding this model's operation.  As any items worth special note are determined, they will be listed here.
