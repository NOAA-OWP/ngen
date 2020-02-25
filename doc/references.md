## [HY_Features](https://docs.opengeospatial.org/is/14-111r6/14-111r6.html)

The OGC Surface Hydrology Features (HY_Features) standard defines a common conceptual information model for identification of specific hydrologic features independent of their geometric representation and scale. The model describes types of surface hydrologic features by defining fundamental relationships among various components of the hydrosphere. This includes relationships such as hierarchies of catchments, segmentation of rivers and lakes, and the hydrologically determined topological connectivity of features such as catchments and waterbodies. The standard also defines normative requirements for HY_Features implementation schemas and mappings to meet in order to be conformant with the conceptual model.

The HY_Features model is based on an abstract catchment feature type that can have multiple alternate hydrology-specific realizations and geometric representations. It supports referencing information about a hydrologic feature across disparate information systems or products to help improve data integration within and among organizations. The model can be applied to cataloging of observations, model results, or other study information involving hydrologic features. The ability to represent the same catchment, river, or other hydrologic feature in several ways is critical for aggregation of cross-referenced or related features into integrated data sets and data products on global, regional, or basin scales.

## [BMI](https://bmi-spec.readthedocs.io/en/latest/)

The Basic Model Interface (BMI) is a library specification created by the Community Surface Dynamics Modeling System (CSDMS) to facilitate the conversion of a model or dataset into a reusable, plug-and-play component. Recall that, in this context, an interface is a named set of functions with prescribed arguments and return values. The BMI functions make a model self-describing and fully controllable by a modeling framework or application. By design, the BMI functions are straightforward to implement in any language, using only simple data types from standard language libraries. Also by design, the BMI functions are noninvasive. This means that a model’s BMI does not make calls to other components or tools and is not modified to use any framework-specific data structures. A BMI, therefore, introduces no dependencies into a model, so the model can still be used in a stand-alone manner.

## Formulations
# HYMOD

<img src="https://raw.githubusercontent.com/NOAA-OWP/ngen/master/doc/images/hymod.png" width=50%>

# T-shirt

<img src="https://raw.githubusercontent.com/NOAA-OWP/ngen/master/doc/images/t-shirt.png" width=80%>

# GIUH

![](https://raw.githubusercontent.com/NOAA-OWP/ngen/master/doc/images/watershed_schematic.jpg)

## Modeling References

[Schaake et al](https://agupubs.onlinelibrary.wiley.com/doi/abs/10.1029/95JD02892)

[Noah-MP Technote](http://www.jsg.utexas.edu/noah-mp/files/Noah-MP_Technote_v0.2.pdf)
