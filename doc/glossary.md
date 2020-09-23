# Definitions

Basic Model Interface (BMI) - a library specification created by the Community Surface Dynamics Modeling System (CSDMS) to facilitate the conversion of a model or dataset into a reusable, plug-and-play component.

Catchment - Generally, a physiographic unit where hydrologic processes take place. This class denotes a physiographic unit, which is defined by a hydrologically determined outlet to which all waters flow. While a catchment exists, it may or may not be clearly identified for repeated study. Specifically for this framework, a catchment represents an arbitrary spatial area and is an abstraction used to encapsulate a model.

Cohesion - a measure of how well components of a software module fit together. Cohesion promotes readability, reliability, reusability, and robustness.

Complex Realization - a single catchment representation of a network of higher detail catchement realizations and their relationships. This allows the modeled area to be represented at multiple levels of detail and supports dynamic high resolution nesting.

Encapsulation - bundling of hydrologic data and methods into a model for a given catchment realization.

Formulation - the actualization of a catchment realization into a computable object with set parameters.

Framework - an abstraction in which software providing generic functionality can be selectively changed by additional user-written code, thus
providing application-specific software.

Functional Abstraction - details of the algorithms to accomplish the function are not visible to the consumer of the function.

Geomorphologic Instantaneous Unit Hydrograph (GIUH) - a modeling method for simulating the runoff for ungauged basins.

HY_Features - The OGC Surface Hydrology Features (HY_Features) standard defines a common conceptual information model for identification of specific hydrologic features independent of their geometric representation and scale.

Nexus - Generally, any location of hydrologic significance and is usually represented as a geometric point. Specifically for this framework, it is a flux transfer object where one catchment interacts with one or more other catchments. 

Realization - Generally, a hydrologic feature type that reflects a distinct hydrology-specific perspective of the catchment or hydro nexus feature types. Shares identity and catchment-nexus relationships with the catchment or nexus it realizes but has hydrologically determined topological properties that express unique ways of perceiving catchments and hydrologic nexuses. Distinct from representation in that it is a refinement of the holistic catchment, allowing for multiple geometric representations of each hydrologic realization. Specifically for this framework, it is an encapsulation of a specific model for a given catchment area.

Software Development Kit (SDK) - a collection of software development tools in one package including documentation, examples, tools and scripts, development environments, static code analysis, and automated testing.

- 

# References

1. https://docs.opengeospatial.org/is/14-111r6/14-111r6.html
2. http://infolab.stanford.edu/~burback/watersluice/node147.html
3. https://www.mdpi.com/2073-4441/11/4/772
