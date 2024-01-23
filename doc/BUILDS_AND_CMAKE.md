# Project Builds

The project uses the [CMake](https://gitlab.kitware.com/cmake/cmake) tool to manage the building of project artifacts.  A complete understanding of CMake is not necessary, but some level of familiarity is, at least to perform certain contribution tasks. 

This document briefly discusses a few of the essential aspects as they relate to the project.

## The TL;DR

- The project needs a (or multiple) CMake-base build system (or buildsystem) to compile code, build artifacts, and perform other project meta-tasks frequently referred to as "builds."
- The build system has to be manually [generated](#generating-a-build-system); it does not exist by default after cloning.
- The build system for the project should be *out-of-source*, meaning in a separate build directory.
    - The build directory should not be committed to Git.
    - `/cmake-build-*/` is currently configured in [.gitignore](../.gitignore) as a suggested pattern for such directories.
- Generating the build system can be done either from [inside](#generating-from-build-directory) the (empty) build directory or [outside](#generating-from-project-root) of it, but the required commands are different.
- Executing builds can also be done either from [inside](#building-from-build-directory) the build directory or [outside](#building-from-source-directory) of it, but again the required commands are different.
- Ensure [dependencies](#dependencies) are available and that the build system is updated or regenerated appropriately when dependencies' statuses change
- The project [build structure](#build-design) is designed using collections of [nested libraries](#library-based-structure), configured at the directory level by multiple `CMakeLists.txt` files

# Generating a Build System

Details on CMake-based build systems can be found [here](https://cmake.org/cmake/help/latest/manual/cmake-buildsystem.7.html), but basically a build system is a bunch of organized _targets_ for building useful artifacts.

## Out-of-Source Building

Before generating, it is important to understanding that build systems come in two basic varieties:

- *in-source* : CMake generated files and compiled artifacts just get created in the same directory as the source
- *out-of-source* : CMake generated files and compiled artifacts get created in a separate build directory

Since the project doesn't need artifacts or other derivative files cluttering up the source tree (or the git repo), *out-of-source* builds should be used.  This impacts the way a build system should be [generated](#generation-commands).

## Ignoring the Build Directory

As suggested, the repo should not include anything from the build system.  The [.gitignore](../.gitignore) file has already been configured with an entry `/cmake-build-*/`, so a directory name following this pattern would be a good choice when generating a local build system.   Regardless, it is highly recommended that build directories be ignored, either based on the existing config or future changes, to avoid accidentally adding build system files or artifacts to the repo.

## Generation Commands

Even for *out-of-source* builds, there are (at least) two ways to generate the build system:

- from within the build system directory (or build directory), by specifying the source directory
- from outside the build directory (typically the source directory), by specifying the build and source directories

### Generating from Build Directory

Starting from the project root, here is an example of creating a CMake build system in an arbitrary directory (here, `cmake-build-dir/`).

    mkdir cmake-build-dir
    cd cmake-build-dir
    cmake ..

### Generating from Project Root

Alternatively, it is also possible to generate a build directory in one command without changing directories.  Again, from the project root:

    cmake -B cmake-build-dir -S .
    
This will create the necessary build directory as well if it did not already exist.
 
Note both the `-B` flag before the build directory and the explicit `-S` flag before the source directory (here `./`).
    
### Optional: Specifying Project Build Config Type

For development builds, it can also be useful to add the `-DCMAKE_BUILD_TYPE` flag to the CMake generation command to specify a particular configuration for that build directory.  E.g., depending which generation method above was used,

    cd cmake-build-dir
    cmake -DCMAKE_BUILD_TYPE=Debug ..
    
or 

    cmake -DCMAKE_BUILD_TYPE=Debug -B cmake-build-debug -S . 

As illustrated, it is a good practice to use a build directory name that reflects the configuration type.  

See the CMake docs and full help for more on this.

## Regenerating

The simplest way to freshly regenerate a build system is by creating a new one, which usually implies deleting the old one.  A previous build directory can simply be deleted and recreated, or have its contents fully deleted, and then the previous generation commands can be re-run to create a new build system there.

# Building Targets

CMake *targets* get defined in the various `CMakeLists.txt` files in the repo.  The primary target to build a runnable program (as of `0.1.0`) is the `ngen` target.  There are also several testing targets defined for generating test executables, as [detailed here](../test/README.md#test-targets-and-executables).

## Building From Build Directory

E.g., 

    cd cmake-build-debug
    make test_unit -j 4

## Building From Source Directory

To build without leaving the source directory, use CMake to build the target and specifying the build directory using the `--build` flag.  Build tool (i.e., `make`) options can also be passed after a `--` at the end of the CMake command.  E.g.

    cmake --build cmake-build-debug --target test_unit -- -j 4
    
# Dependencies

The project has several dependencies, as discussed in more detail in the [Dependencies](DEPENDENCIES.md) doc.  Typically, dependency problems are obvious, but there are a few subtle things related to builds that are worth mentioning here.

## Build System Regeneration

In some cases - in particular **Google Test** - the build system will need to be [generated](#generating-a-build-system) or [regenerated](#regenerating) after the dependency is available.  If it is not, several build targets may either fail or simply not be available.

## Boost ENV Variable

The Boost libraries must be available for the project to compile.  The details are discussed more in the [Dependencies](DEPENDENCIES.md) doc, but as a helpful hint, the **BOOST_ROOT** environmental variable can be set to the path of the applicable [Boost root directory](https://www.boost.org/doc/libs/1_79_0/more/getting_started/unix-variants.html#the-boost-distribution).  The project's [CMakeLists.txt](../CMakeLists.txt) is written to check for this env variable and use it to set the Boost include directory.

Note that if the variable is not set, it may still be possible for CMake to find Boost, although a *status* message will be printed by CMake indicating **BOOST_ROOT** was not set.

# Build Design

## Library-Based Structure

The general design of the CMake configuration is to have each of the various subdirectories containing source files have its own `CMakeLists.txt` file, creating a library for the sources of that directory.  This creates a nested structure of libraries for source files, essentially mirroring the directory structure.

To incorporate these libraries into the main project, the top-level source directories are added within [CMakeLists.txt](../CMakeLists.txt) in the project root.  These get added first with `add_subdirectory`, and then the appropriate libraries being linked to targets as needed with `target_link_libraries`.  The sub-directory `CMakeLists.txt` files follow a similar setup to nest their child directories and libraries.

## Handling Changes

In general, existing `CMakeLists.txt` do not necessarily have to be modified to handle code changes, include the addition of a new source file.  A custom CMake function within [cmake/dynamic_sourced_library.cmake](../cmake/dynamic_sourced_library.cmake) allows for the libraries to dynamically determine the source files to use, in particular using any `*.cpp` files in the library's directory.

However, this assumes includes and dependencies for the library aren't affected.  If the nature of a code change results in a change to the library's dependency or required include directories, that will have to be reflected in the `CMakeLists.txt`.

### Adding New Subdirectories

To add a new directory and library, follow a similar convention as an existing directory.  Ensure the necessary include directories are set for the library, and that any libraries it depends on are available first by setting them as dependencies.

Also, keep in mind adding a new directory and library will require an adjustment to the `CMakeLists.txt` file handling the parent directory/library, within which the addition is nested. 