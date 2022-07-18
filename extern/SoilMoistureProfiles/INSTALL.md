# SMP Submodule Library Build Instructions for use in the Next Generation Water Resources Modeling Framework

**These build instructions are specific to running SoilMoistureProfiles in the [Next Generation Water Resources Modeling Framework](https://github.com/NOAA-OWP/ngen)**

First, cd into the outer directory containing the submodule:

    cd extern/SoilMoistureProfiles/SoilMoistureProfiles

Before library files can be built, a CMake build system must be generated.  E.g.:

    cmake -B cmake_build -S . -DNGEN=1

Note that when there is an existing directory, it may sometimes be necessary to clear it and regenerate, especially if any changes were made to the [CMakeLists.txt](CMakeLists.txt) file.

After there is build system directory, the shared library can be built using the `smpbmi` CMake target. For example, the SMP shared library file (i.e., the build config's `smpbmi` target) can be built using:

    cmake --build cmake_build --target smpbmi -- -j 2

This will build a `cmake_build/libsmpbmi.<version>.<ext>` file, where the version is configured within the CMake config, and the extension depends on the local machine's operating system.    
