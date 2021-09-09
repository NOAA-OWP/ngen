# CFE Submodule

## About

This directory wraps the *cfe* Git submodule repo, which contains a clone of the repo for the OWP CFE module library that implements BMI.  From here, the shared library file for the CFE module can be built for use in NGen.  This is configured with the [CMakeLists.txt](CMakeLists.txt) and other files in this outer directory.

#### Extra Outer Directory

Currently there are two directory layers beneath the top-level *extern/* directory.  This was done so that certain things used by NGen (i.e., a *CMakeLists.txt* file for building shared library files) can be placed alongside, but not within, the submodule.

## Working with the Submodule

Some simple explanations of several command actions are included below.  To better understand what these things are doing, consult the [Git Submodule documentation](https://git-scm.com/book/en/v2/Git-Tools-Submodules). 

### Getting the Latest Changes

There are two steps to getting upstream submodule changes fully 
  1. fetching and locally checking out the changes from the remote
  2. committing the new checkout revision for the submodule

To fetch and check out the latest revision (for the [currently used branch](#viewing-the-current-branch)):

    git submodule update --init -- extern/cfe/cfe

To commit the current submodule checkout revision to the NGen repo:

    git add extern/cfe/cfe
    git commit

### Viewing the Commit Hash

Git submodule configurations include the specific commit to be checked out (or an implicit default).  The current commit can be view with `git submodule status`:

    git submodule status -- extern/cfe/cfe/

This will show the **commit**, **submodule local path**, and the git description for the **commit**.  The specific configuration, including the configured branch, is set in the _.gitmodules_ file in the NGen project root.

### Changing the Commit Branch

The latest commit in the configured branch can be brought in as described here.  If it is ever necessary to change to a different branch, the following will do so:

    git config -f .gitmodules "submodule.extern/cfe/cfe.branch" <branchName>

Note that this will be done in the NGen repo configuration, so it can then be committed and push to remotes.  It is also possible to do something similar in just the local clone of a repo, by configuring `.git/config` instead of `.gitmodules`.  See the Git documentation for more on how that works if needed.

# Usage

## Building Libraries

First, cd into the outer directory containing the submodule:

    cd extern/cfe

Before library files can be built, a CMake build system must be generated.  E.g.:

    cmake -B cmake_build -S .

Note that when there is an existing directory, it may sometimes be necessary to clear it and regenerate, especially if any changes were made to the [CMakeLists.txt](CMakeLists.txt) file.

After there is build system directory, the shared library can be built using the `cfebmi` CMake target. For example, the CFE shared library file (i.e., the build config's `cfebmi` target) can be built using:

    cmake --build cmake_build --target cfebmi -- -j 2

This will build a `cmake_build/libcfebmi.<version>.<ext>` file, where the version is configured within the CMake config, and the extension depends on the local machine's operating system.    
