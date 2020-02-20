# ngen HY\_Features-based data scheme

The `hy_geom.R` script in this directory creates a number of sample outputs for development and testing purposes. All dependencies of this script are available on CRAN with:
`install.packages(c("dplyr", "sf", "nhdplusTools", "jsonlite"))`
- `nhdplus_subset.gpkg` contains a complete set of nhdplus data for the area around the smaller subset in other files.
- `.csv` files contain edge lists for the data described below.
- `.geojson` files contain basic parameters and geometry for the data described below.

## Hydrologic and Hydrodynamic Graphs:

**Narrative**  
At the top level we have a hydrologic graph of catchments and nexuses and a hydrodynamic graph of waterbodies that “pins” to the catchment graph at nexuses. 

I think it would be wise to model both catchments and nexuses as labeled nodes with unlabeled directed edges between. 

Any catchment node of degree 1 with an outward edge is a headwater and degree 1 with an inward edge is an outlet. 

Any catchment node of degree 2 is a typical catchment that has a coincident set of (one or more) waterbodies in the waterbody graph.

Using this graph scheme, headwater catchments contribute either to the upstream end of a river (flow path) or to the shore of a waterbody that breaks up the coverage of catchment areas.

**Data**  
Two edge lists describing the graphs.

## Catchment:

**Narrative**  
Every catchment has a catchment area realization. The catchment area would be implemented as a local water budget model which takes inputs from the atmosphere and non-surficial hydro geologic systems and contributes outputs to an outlet nexus. There is a potential to have a catchment contribute flow incrementally along the waterbody(ies) that flow through it but this would be an advanced case.

Every catchment of degree 2 will have a flow path that has an association with an upstream and downstream extent of a hydrdynamic model represented as a waterbody.

**Data**  
Single valued catchment properties that correspond to catchment area.

## Nexus:

**Narrative**   
All nexuses must be of degree 2 or more. A nexus can receive flow from one or more catchment and contribute it to one or more catchments or waterbodies.

**Data**  
An edge list between nexuses in the catchment graph and nexuses in a hydrodynamic model graph. Edge list not required if shared nexus IDs are used.

## Waterbodies

**Narrative**   
Waterbodies either reside over (rivers and floodplains) or break apart the catchment area coverage (large lakes). Flow can be passed from catchments to waterbodies at nexuses.

**Data**  
Parameters of hydrodynamic model and/or storage discharge or reservoir operations.