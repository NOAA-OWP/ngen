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

To execute the main automated test suite, run the `tests_run` target in **CMake**.

(TODO: more detail on this, once it actually works)

# Creating New Automated Tests

## Creating New Automated Tests

(TODO)
