# Definitions

Basic Model Interface (BMI) - A library specification created by the Community Surface Dynamics Modeling System (CSDMS) to facilitate the conversion of a model or dataset into a reusable, plug-and-play component. [See BMI](https://bmi.readthedocs.io/en/latest/)

Catchment - Generally, a physiographic unit where hydrologic processes take place. This class denotes a physiographic unit, which is defined by a hydrologically determined outlet to which all waters flow. While a catchment exists, it may or may not be clearly identified for repeated study. Specifically for this framework, a catchment represents an arbitrary spatial area and is an abstraction used to encapsulate a model. Every catchment has a catchment area realization. Every catchment of degree 2 will have a flowpath realization. [See HY\_Features](https://docs.opengeospatial.org/is/14-111r6/14-111r6.html#_catchment)

Catchment Area Realization - A catchment implementation as a local water budget model which takes inputs from the atmosphere and non-surficial hydro geologic systems and contributes outputs to an outlet nexus.

Catchment Aggregate - A set of non-overlapping dendritic and interior catchments arranged in an encompassing catchment. [See HY\_Features](https://docs.opengeospatial.org/is/14-111r6/14-111r6.html#_catchment_aggregate)

Catchment Network - Edge-node topology pattern of a set of catchments connected by their hydro nexuses. [see HY\_Features](https://docs.opengeospatial.org/is/14-111r6/14-111r6.html#_catchment_network_topology)

Cohesion - A measure of how well components of a software module fit together. Cohesion promotes readability, reliability, reusability, and robustness.

Complex Realization - A single catchment representation of a network of higher detail catchement realizations and their relationships. This allows the modeled area to be represented at multiple levels of detail and supports dynamic high resolution nesting.

Encapsulation - A fundamental concept of object-oriented programming that is the practice of bundling data and methods that operate on that data into one unit. This entails information hiding, which is hiding the details of the internal implementation of this unit that are not relevant to those who wish to use it. Various levels of access can be assigned to data held by an object that determine whether or not any external parts of a program can access or modify the data with an object's defined methods. Specifically for this framework, encapsulation is used for bundling of hydrologic data and methods into a model for a given catchment realization among other areas in the code. 

Formulation - The actualization of a catchment realization into a computable object with set parameters.

Flowpath Realization - One-dimensional (linear) feature that is a hydrology-specific realization of the holistic catchment. Relates waterbodies to a catchment at hydrologic locations. [see HY\_Features](https://docs.opengeospatial.org/is/14-111r6/14-111r6.html#_flowpath_also_flow_path) 

Framework - An abstraction in which software providing generic functionality can be selectively changed by additional user-written code, thus
providing application-specific software.

Functional Abstraction - Details of the algorithms to accomplish the function are not visible to the consumer of the function.[3]

Geomorphologic Instantaneous Unit Hydrograph (GIUH) - A modeling method for simulating the runoff for ungauged basins.[4]

Hydrologic Location or Hydrolocation - Any location of hydrologic significance located "on" a hydrologic network that is a hydrology-specific realization of a hydrologic nexus. [see HY\_Features](https://docs.opengeospatial.org/is/14-111r6/14-111r6.html#_hydro_logic_location)

HY_Features - The OGC Surface Hydrology Features (HY_Features) standard defines a common conceptual information model for identification of specific hydrologic features independent of their geometric representation and scale. [see HY\_Features](https://docs.opengeospatial.org/is/14-111r6/14-111r6.html)

Inflow Nexus - A nexus in a catchment with the role of getting flow from a contributing catchment. [see HY\_Features](https://docs.opengeospatial.org/is/14-111r6/14-111r6.html#figure25)

Model Domain - A subset of the reference domain. Specified as a catchment aggregate using an inflow nexus and outflow nexus. It uses a consistent catchment area formulation for all contained catchments. It is a catchment network with internal i/o hydrologic locations that link to reference domain i/o hydrologic locations.

Nexus - Generally, represents the place where a catchment interacts with another catchment. Specifically for this framework, it is a flux transfer object where one catchment interacts with one or more other catchments. All nexuses must be of degree 2 or more. A nexus can receive flow from one or more catchment and contribute it to one or more catchments. [see HY\_Features](https://docs.opengeospatial.org/is/14-111r6/14-111r6.html#_hydro_logic_nexus)

Outflow Nexus - A nexus in a catchment with the role of providing inflow to a receiving catchment. [see HY\_Features](https://docs.opengeospatial.org/is/14-111r6/14-111r6.html#figure25)

Realization - Generally, a hydrologic feature type that reflects a distinct hydrology-specific perspective of the catchment or hydro nexus feature types. Shares identity and catchment-nexus relationships with the catchment or nexus it realizes but has hydrologically determined topological properties that express unique ways of perceiving catchments and hydrologic nexuses. Distinct from representation in that it is a refinement of the holistic catchment, allowing for multiple geometric representations of each hydrologic realization. Specifically for this framework, it is an encapsulation of a specific model for a given catchment area. [see HY\_Features](https://docs.opengeospatial.org/is/14-111r6/14-111r6.html#_hydrologic_realization)

Reference Domain - A common hydrography catchment / nexus network for i/o locations and forcing / coupling interface standardization.

Software Development Kit (SDK) - A collection of software development tools in one package including documentation, examples, tools and scripts, development environments, static code analysis, and automated testing.

Waterbody - Mass of water distinct from other masses of water. Waterbodies either reside over (rivers and floodplains) or break apart the catchment area coverage (large lakes). [see HY\_Features](https://docs.opengeospatial.org/is/14-111r6/14-111r6.html#_waterbody_also_water_body)


# References

1. https://bmi.readthedocs.io/en/latest/
2. https://docs.opengeospatial.org/is/14-111r6/14-111r6.html
3. http://infolab.stanford.edu/~burback/watersluice/node147.html
4. https://www.mdpi.com/2073-4441/11/4/772
