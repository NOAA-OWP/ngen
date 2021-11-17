# Project Dependencies

## Summary

| Dependency | Management | Version | Notes |
| ------------- |:------------- | :-----:| :-------: |
| [Google Test](#google-test) | submodule  | `release-1.10.0` | |
| [C/C++ Compiler](#c-and-c-compiler) | external | see below |  |
| [CMake](#cmake) | external | \>= `3.12` | |
| [Boost (Headers Only)](#boost-headers-only) | external | `1.72.0` | headers only library |
| [MPI](https://www.mpi-forum.org) | external | No current implementation or version requirements | Required for [multi-process distributed execution](DISTRIBUTED_PROCESSING.md) |
| [Python 3 Libraries](#python-3-libraries) | external | \> `3.6.8` | Can be [excluded](#overriding-python-dependency). Requires ``numpy`` package |
| [pybind11](#pybind11) | submodule | `v2.6.0` | Can be [excluded](#overriding-pybind11-dependency). |
| [dmod.subsetservice](#the-dmodsubsetservice-package) | external | `>= 0.3.0` | Only required to perform integrated [hydrofabric file subdividing](DISTRIBUTED_PROCESSING.md#subdivided-hydrofabric) for distributed processing . |
| [t-route](#t-route) | submodule | see below | Module required to enable channel-routing.  Requires pybind11 to enable |

# Details

## Google Test

[Google Test](https://github.com/google/googletest) is an external framework used for writing automated tests, as described in the [testing README](../test/README.md).

### Setup 

The dependency is handled as a Git Submodule, located at `test/googletest/`.  To initialize the submodule after cloning the repo:

    git submodule update --init --recursive -- test/googletest
    
Note that initializing the submodule will require the CMake build system be [regenerated](BUILDS_AND_CMAKE.md#build-system-regeneration) if it had already been generated, in order for test targets to work.

### Version Requirements

The version used is automatically handled by submodule config.  This can be synced by re-running the initialization command above.

As of project version `0.1.0`, this was the **Google Test** tag `release-1.10.0`.

## C and C++ Compiler

At present, [GCC]((https://gcc.gnu.org/)) and [Clang](https://clang.llvm.org/) are officially supported, though this is expected to change/expand in the near future.

### Setup

This dependency itself is handled externally.  To used within the project, CMake just needs to know the path to each compiler.  CMake is often able to handle this automatically when [generating the build system](BUILDS_AND_CMAKE.md#generating-a-build-system).

If CMake is unable to find a compiler automatically, the CMake `CMAKE_C_COMPILER` and/or `CMAKE_CXX_COMPILER` variables will need to be explicitly set.  This can be done from the command line when running CMake, or by setting the standard `CC` and `CXX` environment variables.

### Version Requirements

Project C++ code needs to be compliant with the C++ 14 standard.  Supported compilers need to be of a recent enough version to be compatible.  

Additionally, C++ compilers needs to be compatible (ideally officially *tested* as such) with other project C++ dependencies.

#### GCC

Based on [this page](https://gcc.gnu.org/projects/cxx-status.html#cxx14), the C++ 14 support requirement probably equates to a version of GCC \>= version `5.0.0`.

Note also that the [Boost.Geometry](https://www.boost.org/doc/libs/1_72_0/libs/geometry/doc/html/geometry/compilation.html) documentation for `1.72.0` lists GCC `5.0.0` as the latest *tested* compatible GCC version.

#### Clang

The Clang versioning scheme is a little convoluted.  Using the official scheme, Clang 3.4 and later should support all C++ 14 features.

However, Apple likes to apply their own versioning to Clang and LLVM.  The `Apple LLVM version 10.0.1 (clang-1001.0.46.4)` version released with MacOS 10.14.x should do fine.  Recent, earlier version likely will as well, but YMMV.

## CMake

The [CMake](https://gitlab.kitware.com/cmake/cmake) utility facilitates defining and running project builds.  More details on CMake and builds have been documented [here](BUILDS_AND_CMAKE.md).

### Setup

This dependency itself is handled externally.  No special steps are required to use CMake within the project.

However, a [CMake build system](BUILDS_AND_CMAKE.md#generating-a-build-system) must be generated for the project before CMake builds can be run and executable artifacts can be created.

### Version Requirements

Currently, a version of CMake >= `3.12.0` is required.

## Boost (Headers Only)

Boost libraries are used by this project.  In particular, [Boost.Geometry](https://www.boost.org/doc/libs/1_72_0/libs/geometry/doc/html/geometry/compilation.html) is used, but others are also.  

Currently, only headers-only Boost libraries are utilized.  As such, they are not exhaustively listed here since getting one essentially gets them all.

### Setup

Since only headers-only libraries are needed, the Boost headers simply need to be available on the local machine at a location the build process can find.

There are a variety of different ways to get the Boost headers locally.  Various OS may have packages specifically to install them, though one should take note of whether such packages provide a version of Boost that meets this project's requirements.  

Alternatively, the Boost distribution itself can be manually downloaded and unpacked, as described for both [Unix-variants](https://www.boost.org/doc/libs/1_72_0/more/getting_started/unix-variants.html) and [Windows](https://www.boost.org/doc/libs/1_72_0/more/getting_started/windows.html) on the Boost website.

#### Setting **BOOST_ROOT**

If necessary, the project's CMake config is able to use the value of the **BOOST_ROOT** environment variable to help it find Boost, assuming the variable is set.  It is not always required, as CMake may be able to find Boost without this, depending on how Boost was "installed."
  
However, it will often be necessary to set **BOOST_ROOT** if Boost was manually set up by downloading the distribution.

The variable should be set to the value of the **boost root directory**, which is something like `<some_path>/boost_1_72_0`.

### Version Requirements

At present, a version >= `1.72.0` is required.

## Python 3 Libraries

In order to interact with external models written in Python, the Python libraries must be made available.  

Further, the Python environment must have `numpy` installed.  Several Python BMI function calls utilize `numpy` arrays, making `numpy` a necessity for using external Python models implementing BMI

#### Overriding Python Dependency

It is possible to run the build, build artifacts, etc. in a way that excludes Python-related functionality, thereby relieving this as a dependency.  This may be useful in runtime environments that will not interact with a Python external model, and/or installing Python is prohibitively difficult.

To do this, set either the `NGEN_ACTIVATE_PYTHON` environment variable to `false` or include the `-DNGEN_ACTIVATE_PYTHON:BOOL=false` option when running the `cmake` build on the command line to generate the build system.

### Setup

The first step is to make sure Python is installed (with step 1a being installing NumPy.  CMake will then use its [FindPython](https://cmake.org/cmake/help/v3.19/module/FindPython.html#module:FindPython) functionality to obtain the Python libraries. On some systems, CMake may be able to find everything automatically, though that will not always be the case.

If CMake cannot find the needed Python artifacts on its own, the following user environmental variables can be set in the user's shell to control where CMake looks:

* `PYTHON_INCLUDE_DIR`
    * This should be the directory containing the Python header files in the local installation
* `PYTHON_LIBRARIES`
    * This should be set to the directory containing the appropriate local lib file(s) (e.g., `libpython*.dylib` on a Mac)

#### NumPy

Additionally, the NumPy package must be installed and its headers available, in particular to work with Python BMI functions (with the general details of installing Python packages left to the user).  The simplest method is installing in the root/global environment, though this is not always an option.

One supported option is to create a `virtualenv` environment at `.venv` in the project root, then install `numpy` within it.   Such `.venv` directory is ignored by Git and searched by CMake for the NumPy headers.

The variable `Python_NumPy_INCLUDE_DIR` (either as a CMake variable or a user environment variable) can also be used to set the NumPy include directory path searched by CMake.  However, this will be ignored if there is a `.venv` directory found, as described above.
    
### Version Requirements

A version of Python 3 >= `3.6.8` is required.  There are no specific version requirements for `numpy` currently. 

## pybind11

The `pybind11` dependency is a header-only library that allows for exposing Python types, etc. within C++ code.  It is necessary to work with externally maintained, modular Python models.

#### Overriding pybind11 Dependency

This dependency can be overridden and disabled if the Python dependency is configured as such, as described [here](#overriding-python-dependency).

### Setup

The dependency is handled as a Git Submodule, located at `extern/pybind11`.   To initialize the submodule after cloning the repo:

    git submodule update --init extern/pybind11
    
Git _should_ take care of checking out the commit for the required version automatically (assuming latest upstream changes have been fetched), so it should be possible to also use the command above to sync future updates to the required version.
 
However, to verify the checked out version, examine the output of:

    git submodule status
    
If the above `update` command does not check out the expected version, this can be done manually.  Below is an example of how to do this for the version tagged `v2.6.0`:

    cd extern/pybind11
    git checkout v2.6.0
    
### Version Requirements

The version used is automatically handled by submodule config.  This can be synced by re-running the initialization command above.

As of project version `0.1.0`, the required version is tag `v2.6.0`.

## The `dmod.subsetservice` Package

This is an optional package available via the [OWP DMOD tool](https://github.com/NOAA-OWP/DMOD).  It only is required to enable the driver to generate partition-specific subdivided hydrofabric files from the complete hydrofabric file, as is conditionally required for [distributed processing](DISTRIBUTED_PROCESSING.md).  Further, its use is only attempted when such files are needed for distributed processing _and_ are not already created.

As such, this dependency is ignored unless both Python support and MPI support are enabled and built into the _ngen_ driver.

### Setup

This must be installed in the active Python environment at the time the `ngen` executable is called.  This is usually best done via a virtual environment, especially if one is already set up as described for the [Numpy](#numpy) dependency.  However, all that matters is that the Python environment available to the ngen driver has this package installed.

If installing to a virtual env as suggested above, the installation steps are similar to what is described in the [DMOD project document for the subsetting CLI tool]( https://github.com/NOAA-OWP/DMOD/tree/master/doc/SUBSETTING_CLI_TOOL.md).  The difference is just to create and/or source a virtual environment locate here first, and then change to the DMOD project directory.  All the example commands after the step of sourcing the virtual environment can then be followed as described on that page.

### Version Requirements

Version `0.3.0` or greater must be installed.

## t-route

### Setup

The dependency is handled as a Git Submodule, located at `extern/t-route`.   To initialize the submodule after cloning the repo:
```sh
git submodule update --init extern/t-route
```
Git _should_ take care of checking out the commit for the required version automatically (assuming latest upstream changes have been fetched), so it should be possible to also use the command above to sync future updates to the required version.
 
Once the submodule is fetched, the routing module must installed in a suitable environment.

One supported option is to create a `virtualenv` environment at `.venv` in the project root and activate this environment for any simulations using routing.  

See the [installing t-route section here](PYTHON_ROUTING.md#installing-t-route) for more details.

### Enabling Routing in Simulations

To do this, include the `-DNGEN_ACTIVATE_ROUTING:BOOL=true` option when running the `cmake` build on the command line to generate the build system.  An appropriate `routing_config.yaml` must be passed to the NGen realization config.  More info can be found in the [python routing documentation](PYTHON_ROUTING.md#routing-config)

Before executing any simulation, be sure to activate the virtual environment.
```sh
source .venv/bin/activate
```
