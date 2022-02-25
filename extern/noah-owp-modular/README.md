# Noah-OWP-Modular Submodule

## About

This directory wraps the *noah-owp-modular* Git submodule repo, which contains a modularized land surface model implementing BMI. From here, a shared library file for the *noah-owp-modular* module can be built for use in NGen.  This is configured with the [CMakeLists.txt](CMakeLists.txt) and other files in this outer directory.

#### Extra Outer Directory

Currently there are two directory layers beneath the top-level *extern/* directory.  This was done so that certain things used by NGen (i.e., a *CMakeLists.txt* file for building shared library files) can be placed alongside, but not within, the submodule.

## Working with the Submodule

Some simple explanations of several command actions are included below.  To better understand what these things are doing, consult the [Git Submodule documentation](https://git-scm.com/book/en/v2/Git-Tools-Submodules). 

### Getting the Latest Changes

There are two steps to getting upstream submodule changes fully 
  1. fetching and locally checking out the changes from the remote
  2. committing the new checkout revision for the submodule

To fetch and check out the latest revision (for the [currently used branch](#viewing-the-current-branch)):

    git submodule update --init --remote -- extern/noah-owp-modular/noah-owp-modular

To commit the current submodule checkout revision to the NGen repo:

    git add extern/noah-owp-modular/noah-owp-modular
    git commit

### Viewing the Commit Hash

Git submodule configurations include the specific commit to be checked out (or an implicit default).  The current commit can be view with `git submodule status`:

    git submodule status -- extern/noah-owp-modular/noah-owp-modular/

This will show the **commit**, **submodule local path**, and the git description for the **commit**.  The specific configuration, including the configured branch, is set in the _.gitmodules_ file in the NGen project root.

### Changing the Commit Branch

The latest commit in the configured branch can be brought in as described here.  If it is ever necessary to change to a different branch, the following will do so:

    git config -f .gitmodules "submodule.extern/noah-owp-modular/noah-owp-modular.branch" <branchName>

Note that this will be done in the NGen repo configuration, so it can then be committed and push to remotes.  It is also possible to do something similar in just the local clone of a repo, by configuring `.git/config` instead of `.gitmodules`.  See the Git documentation for more on how that works if needed.

# Usage

## Building Libraries

First, cd into the outer directory containing the submodule:

    cd extern/noah-owp-modular

Before library files can be built, a CMake build system must be generated.  E.g.:

    cmake -B cmake_build -S .

Note that when there is an existing directory, it may sometimes be necessary to clear it and regenerate, especially if any changes were made to the [CMakeLists.txt](CMakeLists.txt) file.

> 🚧 Warning
> 
> This module (optionally) depends on the netCDF libraries for C and Fortran.  By default, netcdf output from the library is disabled by using the `NGEN_OUTPUT_ACTIVE` compiler directive, and this warning is not applicable.
>
> If, for whatever reason, you remove the `NGEN_OUTPUT_ACTIVE` compiler directive from the CMakeLists.txt file, you will need to uncomment the NetCDF dependency finding and linking sections of the CMakeLists.txt.  When you do this, you may need to pass information to cmake to inform it about libraries or include files in a non-standard location. If the build system is not be able to find them it may be necessary to provide one or more of the following variables to `cmake`:
> 
> * `netCDF_C_LIB` The path to the C shared object library file, `libnetcdf.so`
> * `netCDF_FORTRAN_LIB` The path to the Fortran shared object library file, `libnetcdff.so`
> * `netCDF_INCLUDE_DIR` netCDF Include Directory.
> * `netCDF_MOD_PATH` Directory containing the `netcdf.mod` Fortran module
>
> These can be specified with the `-D` flag like so:
> ```
> cmake -DnetCDF_MOD_PATH=/usr/include/openmpi-x86_64/ -B cmake_build -S .
> ```

After there is a configured build system directory, the shared libraries can be built. For example, the Noah-OWP-Modular bmi shared library file (i.e., the build config's `surfacebmi` target) can be built using:

    cmake --build cmake_build --target surfacebmi -- -j 2

This will build a `cmake_build/libsurfacebmi.<version>.<ext>` file, where the version is configured within the CMake config, and the extension depends on the local machine's operating system.    
