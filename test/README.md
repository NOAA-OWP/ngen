# Testing

## Testing Framework Initialization

The project uses the **Google Test** framework for creating automated tests, included as a *Git Submodule*. The submodule must be initialized before tests will function.

The following command will initialize the submodule:

    git submodule update --init --recursive

### Pulling Future Updates

The submodule is checked out to a particular tagged commit of **Google Test**. Over time, as new releases for **Google Test** are available, the tagged version used by this project may change.  Clones of this repo can stay in sync with any such submodule changes by re-running the `git submodule update --init --recursive` command.

## Executing Automated Tests

To execute the main automated test suite, run the `tests_run` target in **CMake**.

(TODO: more detail on this, once it actually works)

## Creating New Automated Tests

(TODO)
