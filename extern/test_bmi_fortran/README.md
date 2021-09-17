# Test BMI Model Fotran Implementation

* [About](#about)
* [Building](#building)
    * [Generating a Build System Directory](#generating-a-build-system-directory)
    * [Building the Shared Library](#building-the-shared-library)
* [Implementation Details](#implementation-details)

# About

This is a implementation of a Fortran-based model that fulfills the Fortran language BMI interface and can be built into a shared library.  It is intended to serve as a control for testing purposes, freeing the framework from dependency on any real-world model in order to test BMI related functionality.

# Building

To generate the shared library files, build the `testbmifotranmodel` target in the generated build system.  This needs to be separate from the main NextGen build system.

#### Generating a Build System Directory

Run from the project root directory:

    cmake -B extern/test_bmi_fortran/cmake_build -S extern/test_bmi_fortran

To regenerate, simply remove the `extern/test_bmi_fortran/cmake_build` directory and run the command again.

#### Building the Shared Library

Again, from the project root directory:

    cmake --build extern/test_bmi_fortran/cmake_build --target testbmifortranmodel

This should generate the appropriate library files for your system.

# Implementation Details

As a reminder, the BMI spec for Fortran (2003) is implemented as a Fortran abstract type with deferred procedures.  The extended type has a `model` member for holding a variable of the data structure used for the model (here, a `test_bmi_model` type, declared in [src/test_model.f90](src/test_model.f90)), and a series of deferred procedures pointing to the functions necessary to fulfill BMI.  

In order to interoperate with a C/C++ frammework, the [ISO C fortran BMI](../iso_c_fortran_bmi/README.md) is required.  In constrast to the C BMI libary, where a (non BMI) `register_bmi()` function is required to hook a BMI struct's function pointers to the model's BMI implementation of those functions, using the ISO C fortran BMI requires the implementing model library to provide a `register_bmi()` which provides an opaque (`void *`) handle to the internal Fortran model stucture.  In this case, the register function can be found in [src/bmi_test_bmi_fortran.f90](src/bmi_test_bmi_fortran.f90).

For this test model, the implemented functions in general follow the BMI documented spec, so in most cases that [documentation](https://bmi.readthedocs.io/en/latest/) is sufficient for understanding this model's operation.  As any items worth special note are determined, they will be listed here.
