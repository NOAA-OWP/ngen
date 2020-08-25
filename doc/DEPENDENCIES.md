# Project Dependencies

## Summary

| Dependency | Management | Version | Notes |
| ------------- |:------------- | :-----:| :-------: |
| [Google Test](#google-test) | submodule  | `release-1.10.0` | |
| [C/C++ Compiler](#c-and-c-compiler) | external | see below |  |
| [CMake](#cmake) | external | \> `3.10` | |
| [Boost (Headers Only)](#boost-headers-only) | external | `1.72.0` | headers only library |

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

Currently, a version of CMake >= `3.10.0` is required.

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