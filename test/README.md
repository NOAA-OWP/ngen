# Testing

- [Testing Frameworks](#testing-frameworks)
- [Executing Automated Tests](#executing-automated-tests)
- [Creating New Automated Tests](#creating-new-automated-tests)

# Testing Frameworks

The project uses the **Google Test** framework for automated C++ tests.  As discussed below, it's included as a submodule, so there are a few extra steps needed to get set up locally to use it.

At present (`0.1.0`), there are no testing frameworks incorporated for automatically testing non-C++ code.  These may be added in the future.

## **Google Test** Initialization

In order to be able to run the **Google Test** tests, the submodule must be initialized, which can be done with the following command:

    git submodule update --init --recursive

### Pulling Future Updates

The submodule is checked out to a particular tagged commit of **Google Test**. Over time, as new releases for **Google Test** are available, the tagged version used by this project may change.  Clones of this repo can stay in sync with any such submodule changes by re-running the `git submodule update --init --recursive` command.

# Executing Automated Tests

There are several CMake testing executables/targets configured in the build for testing purpose.  They are discussed a bit [here](#adding-tests-to-cmake-builds). These targets each build a test executable file, which can then be executed to actually perform tests.

In particular the `test_unit`, `test_integration`, and `test_all` executables will build large collections of tests, but others applicable to the situation may also be available (see [test/CMakeLists.txt](./CMakeLists.txt)).

## Testing From the Command Line

Here is an example for (cleanly) building, then running, all unit tests.  It assumes executing the commands from the project root directory, and a build directory of `./cmake-build-dir`.

    cmake --build cmake-build-dir --target clean -- -j 4    
    cmake --build cmake-build-dir --target test_unit -- -j 4
    ./cmake-build-dir/test/test_unit
    
# Creating New Automated Tests

Automated testing design and infrastructure for this project are somewhat fluid while this project is in its early stages.  The only strict rules are (as of  `0.1.0`):

- C++ tests should use the **Google Test** framework
- C++ tests should be added to the CMake build via [/test/CMakeLists.txt](./CMakeLists.txt), as described [here](#adding-tests-to-cmake-builds)
- test code should be placed under the [/test/](../test) directory

However, there are few additional rules of thumb to keep in mind.  These are good to follow, but not set in stone yet, and (importantly) need feedback on how useful they are and if there are any situations where they are lacking.

* [Separate test types in separate files](#separate-test-types-in-separate-files)
* [Use analogous names](#use-analogous-names)
* [Use analogous paths](#use-analogous-paths)
* [Keep unit test assertions to a minimum](#keep-unit-test-assertions-to-a-minimum)

### Test Creation Rules of Thumb

#### Separate Test Types in Separate Files

Keep unit tests, integration tests, etc., in separate files dedicated to that type of test.  This is helpful for several reasons, including isolating which overall group of tests get run.

#### Use Analogous Names

To help give clarity to what relates to what, use analogous names for associated test items.  This should be the source code item's name(\*), with either a standard prefix or suffix added.

- files
    - base: source code filename
    - suffix: `_Test`/`Test` for unit tests or `_IT`/`IT` for integration tests
        - e.g., `LinearReservoir.cpp` and `LinearReservoirTest.cpp`
        - e.g., `HY_FlowPath.cpp` and `HY_FlowPath_Test.cpp`
- *test fixtures*, *test suites*, and associated classes
    - base: *upper camel case* form(\*) of source code class
    - suffix: `Test` for unit tests or `IT` for integration tests
- test names
    - base: *upper camel case* form(\*) of source code function
    - prefix: `Test`
    - suffix: optional(\*\*) alphanumeric identifier (e.g., `1`, `1a`, `1b`)

\* Underscores are not allowed in **test fixtures** by **Google Test**, thus the bases for these are belong to camel case.

\*\* The intent of an additional suffix on test name would be to differentiate a case such as two very similar tests, like two that have exactly the same logic but operate on two different example input cases.

#### Use Analogous Paths

To help with finding associated test code files, mirror the directory structure under `test/` of the path to the associated source file.  E.g., functions defined in `models/hymod/include/Hymod.h` should have unit tests in `test/models/hymod/include/HymodTest.cpp`.

#### Keep Unit Test Assertions to a Minimum

For unit tests, try to keep the number of assertions (and perhaps comparisons in general) to a minimum, to help isolate individual aspect of behavior being tested. 

## Adding Tests to CMake Builds

Tests should be added to the CMake build using [/test/CMakeLists.txt](./CMakeLists.txt).  To do this, ensure the test file is added to the appropriate calls of the `add_automated_test()` or `add_automated_test_w_mock` macro.  

In particular, the macro invocation creating either the `test_unit` or the `test_integration` executable should be updated to also have this file, depending on whether it is a unit or integration test.  Additionally, the line creating the `test_all` executable should have it added.
  
Optionally, a individual test executable can also be added for the test/test file.
