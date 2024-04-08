# Testing

- [Testing Frameworks](#testing-frameworks)
- [Executing Automated Tests](#executing-automated-tests)
- [Creating New Automated Tests](#creating-new-automated-tests)

# Testing Frameworks

- [Google Test](#google-test)

Note: At present (`0.1.0`), there are no testing frameworks incorporated for automatically testing non-C++ code.  These may be added in the future.

## **Google Test** 

The project uses the **Google Test** framework for automated C++ tests.  Its source code and documentation can be found [here](https://github.com/google/googletest).  It is included as a [Git Submodule](https://git-scm.com/book/en/v2/Git-Tools-Submodules), so the submodule must be initialized locally, a step not automatically performed by default when cloning the repo.  

Initializing can be done by running the following command from the project root:

    git submodule update --init --recursive -- test/googletest
    
Additionally, this command can be re-run to sync the local submodule working tree with the state expected upstream.  I.e., if this project gets updated to utilize a new version of **Google Test**, the command above can be run again locally to make sure the local copy also gets moved to that version.

See the `git help submodule` command for more information.

### (Re)generating CMake Project Buildsystem

The CMake buildsystem will need to either be generated or regenerated after the submodule is initialized.  

Regenerating just requires an initial step of removing the/an existing CMake build directory (frequently something like `cmake-build-debug/` in the project root). 
 
If/once the desired build directory does not exist, use an appropriate CMake command to generate the buildsystem (see [wiki/Quickstart](https://github.com/NOAA-OWP/ngen/wiki/NGen-Tutorial)) for more detailed description.  E.g.:

    cmake -DCMAKE_BUILD_TYPE=Debug -DNGEN_WITH_TESTS:BOOL=ON -B cmake-build-debug -S .

# Executing Automated Tests

- [C++ Tests](#c-tests)

## C++ Tests

The basic process for executing a group of C++ tests is:

- execute an appropriate CMake test target 
    - this generates an executable tests file
- execute the generated tests file

### Test Targets and Executables

There are several CMake testing executables/targets configured in [/test/CMakeLists.txt](./CMakeLists.txt), discussed in the section on [adding tests to CMake builds](#adding-tests-to-cmake-builds).  The primary ones, corresponding to important collections of tests, are:

* `test_unit` for all unit tests
* `test_integration` for all integration tests
* `test_all` for all unit and integration tests

_Note_: make sure the CMake build system is [(re)generated](#regenerating-cmake-project-buildsystem) before trying to build the targets below.

When built with CMake, these and other similar targets each produce a target-specific test executable file.  E.g.:

    cmake --build cmake-build-dir --target test_unit -- -j 4
    
This produces an executable test file `cmake-build-dir/test/test_unit`, which can then be used to run the configured collection of tests:

    ./cmake-build-dir/test/test_unit
    
It is also possible to add the `--gtest_filter=` flag followed by a colon-delimited series of tests' *full names*.  **Google Test** defines the *full name* of a test as:

    test_suite_name.test_name
    
For example:

    ./cmake-build-dir/test/test_unit --gtest_filter=HymodKernelTest.TestCalcET0:HymodKernelTest.TestRun0
            
# Creating New Automated Tests

Automated testing design and infrastructure for this project are somewhat fluid while this project is in its early stages.  The only strict rules are (as of  `0.1.0`):

- C++ tests should use the **Google Test** framework
- C++ tests should be added to the CMake build via [/test/CMakeLists.txt](./CMakeLists.txt), as described [here](#adding-tests-to-cmake-builds)
- test code should be placed under the [/test/](../test) directory

However, there are a few [rules of thumb](#test-creation-rules-of-thumb) discussed below to consider as the process matures in its early stages.

## Adding Tests to CMake Builds

Tests should be added to the CMake build using [/test/CMakeLists.txt](./CMakeLists.txt).  Within that file, there are several test targets that are already defined, using either the `add_automated_test()` or `add_automated_test_w_mock()` macro.  In particular:

* `test_unit` for all unit tests
* `test_integration` for all integration tests
* `test_all` for all unit and integration tests

These and other similar targets are configured (using either the `add_automated_test()` or `add_automated_test_w_mock()` macro) with the applicable `*.cpp` files that have the code for tests that should be included in the particular target.  

Existing target configurations must be updated with the test file for any newly added tests, assuming the file is not already configured.  

## Test Creation Rules of Thumb

There are few additional rules of thumb to keep in mind when creating new tests.  These are not set in stone yet; rather, they seem like good practices, but feedback is still needed on how useful they are and if there are any situations where they are lacking.

* [Test work before push or PR](#test-work-before-push-or-pr)
* [Separate test types in separate files](#separate-test-types-in-separate-files)
* [Use analogous names](#use-analogous-names)
* [Use analogous paths](#use-analogous-paths)
* [Keep unit test assertions to a minimum](#keep-unit-test-assertions-to-a-minimum)

#### Test Work Before Push or PR

Make sure existing tests related to a change are still applicable and still pass before distributing the change (i.e., pushing, making a pull request, etc.).  Add new tests as needed for changes not covered by existing tests.

If new tests are needed, but creating them is not immediately feasible, create one or more tickets/issues for adding such tests in the future. 

#### Separate Test Types in Separate Files

Keep unit tests, integration tests, etc., in separate files dedicated to that type of test.  This is helpful for several reasons, including isolating which overall group of tests get run.

#### Use Analogous Names

To help give clarity to what relates to what, use analogous names for associated test items.  This should be the source code item's name(\*), with either a standard prefix or suffix added.

- files
    - base: source code filename
    - suffix: `_Test`/`Test` for unit tests or `_IT`/`IT` for integration tests
        - e.g., `HY_FlowPath.cpp` and `HY_FlowPath_Test.cpp`
- *test fixtures*, *test suites*, and associated classes
    - base: *upper camel case* form(\*) of source code class
    - suffix: `Test` for unit tests or `IT` for integration tests
- test names
    - base: *upper camel case* form(\*) of source code function
    - prefix: `Test`
    - suffix: optional(\*\*) alphanumeric identifier (e.g., `1`, `1a`, `1b`)

\* Underscores are not allowed in identifiers that become part of a test's **full name** by **Google Test**, which will be based on the **test fixture** name (when one is used) and the test's name; thus, for fixture classes and test functions, the applicable bases should be converted.

\*\* The intent of an additional suffix on test name would be to differentiate a case such as two very similar tests, like two that have exactly the same logic but operate on two different example input cases.

#### Use Analogous Paths

To help with finding associated test code files, mirror the directory structure under `test/` of the path to the associated source file.  E.g., functions defined in `models/hymod/include/Hymod.h` should have unit tests in `test/models/hymod/include/HymodTest.cpp`.

#### Keep Unit Test Assertions to a Minimum

For unit tests, try to keep the number of assertions (and perhaps comparisons in general) to a minimum, to help isolate individual aspect of behavior being tested. 
