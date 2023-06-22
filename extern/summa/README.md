# SummaSundials Submodule

## About

This directory wraps the *summa* Git submodule repo, which contains a clone of the repo for the OWP SummaSundials module library that implements BMI.  From here, the shared library file for the SummaSundials module can be built for use in NGen.  This is configured with the [CMakeLists.txt](CMakeLists.txt) and other files in this outer directory.

#### Extra Outer Directory

Currently there are two directory layers beneath the top-level *extern/* directory.  This was done so that certain things used by NGen (i.e., a *CMakeLists.txt* file for building shared library files) can be placed alongside, but not within, the submodule.

## Working with the Submodule

Some simple explanations of several command actions are included below.  To better understand what these things are doing, consult the [Git Submodule documentation](https://git-scm.com/book/en/v2/Git-Tools-Submodules). 

### Getting the Latest Changes

There are two steps to getting upstream submodule changes fully 
  1. fetching and locally checking out the changes from the remote
  2. committing the new checkout revision for the submodule

To fetch and check out the latest revision (for the [currently used branch](#viewing-the-current-branch)):

    git submodule update --init -- extern/summa/summa

To commit the current submodule checkout revision to the NGen repo:

    git add extern/summa/summa
    git commit

### Viewing the Commit Hash

Git submodule configurations include the specific commit to be checked out (or an implicit default).  The current commit can be view with `git submodule status`:

    git submodule status -- extern/summa/summa/

This will show the **commit**, **submodule local path**, and the git description for the **commit**.  The specific configuration, including the configured branch, is set in the _.gitmodules_ file in the NGen project root.

### Changing the Commit Branch

The latest commit in the configured branch can be brought in as described here.  If it is ever necessary to change to a different branch, the following will do so:

    git config -f .gitmodules "submodule.extern/summa/summa.branch" <branchName>

Note that this will be done in the NGen repo configuration, so it can then be committed and push to remotes.  It is also possible to do something similar in just the local clone of a repo, by configuring `.git/config` instead of `.gitmodules`.  See the Git documentation for more on how that works if needed.

# Usage

## Building Libraries

First, cd into the outer directory containing the submodule:

    cd extern/summa

If you want to use Sundials IDA or BE Kinsol, set -DCMAKE_BUILD_TYPE=Sundials_NexGen in the build script.  Then, before summa can be built, Sundials needs to be installed. 
Download the latest release of IDA solver from SUNDIALS package in https://computing.llnl.gov/projects/sundials/sundials-software

    wget "https://github.com/LLNL/sundials/releases/download/v6.3.0/sundials-6.3.0.tar.gz"

Extract the corresponding compressed file

    tar -xzf sundials-6.3.0.tar.gz
    
Enter the buildir and run

    cd sundials/buildir
    ./build_cmake
    make
    make install
    
Note if you need to recompile after a system upgrade, delete the contents of sundials/instdir and sundials/buildir EXCEPT sundials/buildir/build_cmake before building and installing.

Before ngen library files can be built, a CMake build system must be generated.  E.g.:

    cmake -B cmake_build -S .

Note (similar to above) that when there is an existing directory, it may sometimes be necessary to clear it and regenerate, especially if any changes were made to the CMakeLists.txt file.

After there is build system directory, the shared library can be built using the `summabmi` CMake target. For example, the SummaSundials shared library file (i.e., the build config's `summabmi` target) can be built using:

    cmake --build extern/summa/cmake_build --target all

This will build a `cmake_build/libsummabmi.<version>.<ext>` file, where the version is configured within the CMake config, and the extension depends on the local machine's operating system.    

There is an example of a bash script to build the ngen libraries at ngen/extern/summa/summa/build/cmake/build_ngen.[system_type].bash. Sundials is turned off here. These need to be run in the main ngen directory.

To run a test basin, in the main ngen directory, run

    cmake_build/ngen data/catchment_data.geojson "celia" ./data/nexus_data.geojson "nex-26" ./extern/summa/summa/test_ngen/example_realization_config_w_summa_bmi.json
or 
    cmake_build/ngen data/catchment_data.geojson "cat-27" ./data/nexus_data.geojson "nex-26" ./extern/summa/summa/test_ngen/example_realization_config_w_summa_bmi__mac.json

This command can be run as ./extern/summa/summa/test_ngen/example_run_celia.[system_type].sh also.

This test is currently the Celia Laugh test, not the cat-27 ngen example catchment.
