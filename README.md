# Next Gen Water Modeling Framework Prototype

**Description**:  
As we attempt to apply hydrological modeling at different scales, the traditional organizational structure and algorithms of model software begin to interfere with the ability of the model to represent complex and heterogeneous processes at appropriate scales.  While it is possible to do so, the code becomes highly specialized, and reasoning about the model and its states becomes more difficult.  Model implementations are often the result of taking for granted the availability of a particular form of data **and** solution -- attempting to map the solution to that data. This framework takes a data centric approach, organizing the data first and mapping appropriate solutions to the existing data.

This framework includes an encapsulation strategy which focuses on the hydrologic data first, and then builds a functional abstraction of hydrologic behavior.  This abstraction is naturally recursive, and unlocks a higher level of modeling and reasoning using computational modeling for hydrology.  This is done by organizing model components along well-defined flow boundaries, and then implementing strict API’s to define the movement of water amongst these components.  This organization also allows control and orchestration of first-class model components to leverage more sophisticated programming techniques and data structures.


  - **Technology stack**: Core Framework using C++ (minimum standard c++14) to provide polymorphic interfaces with reasonable systems integration.
  - **Status**:  Version 0.1.0 in initial development including interfaces, logical data model, and framework structure.  See  [CHANGELOG](CHANGELOG.md) for revision details.

## Structural Diagrams

![Catchments](https://raw.githubusercontent.com/NOAA-OWP/ngen/master/doc/images/hy_features_catchment_diagram.png)

**Catchments**: Catchments represent arbitrary spatial areas. They are the abstraction used to encapsulate a model. The three marked catchments could use three different models, 3 copies of the same model, or some combination of the previous options 

![Realizations](https://raw.githubusercontent.com/NOAA-OWP/ngen/master/doc/images/realization_relations.png)

**Realizations**: Different kinds of catchment realizations can be used to encapsulate different types of models. These models will have different types of relations with neighbors. When a relation exists between two adjacent catchments synchronization is necessary.

![Complex Realizations](https://raw.githubusercontent.com/NOAA-OWP/ngen/master/doc/images/complex_realizations.png)

**Complex Realizations**: An important type of catchment realization is the complex catchment realization. This allows a single catchment to be represented by a network of higher detail catchment realizations and their relationships. This allows the modeled area to be represented at multiple levels of detail and supports dynamic high resolution nesting.

## Dependencies
[gcc](https://gcc.gnu.org/)

[CMake](https://gitlab.kitware.com/cmake/cmake)

[Googletest](https://github.com/google/googletest) only for testing.

[Boost.Geometry](https://www.boost.org/doc/libs/1_72_0/libs/geometry/doc/html/geometry/compilation.html)

If specific versions of other software are required, or known not to work, call that out.

## Installation

Detailed instructions on how to install, configure, and get the project running.
This should be frequently tested to ensure reliability. Alternatively, link to
a separate [INSTALL](INSTALL.md) document.

## Configuration

If the software is configurable, describe it in detail, either here or in other documentation to which you link.

## Usage

Show users how to use the software.
Be specific.
Use appropriate formatting when showing code snippets.

## How to test the software

The project uses the **Google Test** framework for creating automated tests for C++ code.

To execute the full collection of automated C++ tests, run the `test_all` target in **CMake**, then execute the generated executable (substituting `cmake-build-dir` with appropriate directory name as needed):  

    cmake --build cmake-build-dir --target test_all -- -j 4
    ./cmake-build-dir/test/test_all

Replace `test_all` with `test_unit` or `test_integration` to run only those tests.

See the [Testing ReadMe](test/README.md) file for a more thorough overview of testing infrastructure.

## Known issues

Document any known significant shortcomings with the software.

## Getting help

Instruct users how to get help with this software; this might include links to an issue tracker, wiki, mailing list, etc.

**Example**

If you have questions, concerns, bug reports, etc, please file an issue in this repository's Issue Tracker.

## Getting involved

This section should detail why people should get involved and describe key areas you are
currently focusing on; e.g., trying to get feedback on features, fixing certain bugs, building
important pieces, etc.

General instructions on _how_ to contribute should be stated with a link to [CONTRIBUTING](CONTRIBUTING.md).


----

## Open source licensing info
1. [TERMS](TERMS.md)
2. [LICENSE](LICENSE)


----

## Credits and references

[Reference Document]https://github.com/NOAA-OWP/ngen/blob/ZacharyWills-review-1/doc/references.md
