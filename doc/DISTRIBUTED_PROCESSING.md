# Distributed Processing

Distributed multi-process execution of the Nextgen driver (_ngen_) is possible through use of an implementation of MPI.  

* [Overview](#overview)
* [Building the MPI-Enabled Nextgen Driver](#building-the-mpi-enabled-nextgen-driver)
* [Running the MPI-Enabled Nextgen Driver](#running-the-mpi-enabled-nextgen-driver)
  * [Automatic Partitioning](#automatic-partitioning)
  * [Examples](#examples)
    * [Example - Automatic Partitioning](#example---automatic-partitioning)

# Overview

The basic design starts with partitioning: grouping the catchments and nexuses of the entire hydrofabric into separate collections, one per MPI rank.  Every nexus must be in the same partition as all of the catchments that drain to it, so that no rank needs to communicate with another during the simulation.  The driver [partitions automatically](#automatic-partitioning) during initialization.

Each rank reads the features of its partition from the supplied hydrofabric, and then constructs the specific model formulations appropriate for the features within its partition.

# Building the MPI-Enabled Nextgen Driver

To enable distributed processing capabilities in the driver, the [CMake build must be generated](BUILDS_AND_CMAKE.md) with the `NGEN_WITH_MPI` variable set to `ON`, such as `NGEN_WITH_MPI:BOOL=ON` and the necessary MPI libraries must be available to CMake.

Otherwise, if using the standard build process with CMake, the driver is built using the same commands.  It can be built either as part of the entire project or the `ngen` CMake target.  CMake manages the necessary adjustments to, e.g., the compiler used, etc. 

# Running the MPI-Enabled Nextgen Driver

* The MPI-enabled driver is run by wrapping within the `mpirun` command, supplying also the number of processes to start.  

## Automatic Partitioning
The driver partitions the hydrofabric itself during initialization. Every nexus is assigned to one rank, along with all of the catchments that drain to it. Rank 0 reads only the divides' connectivity, without their geometry or other attributes, and divides the nexuses among the ranks in contiguous blocks, in the order the hydrofabric stores them, weighting each nexus by the number of catchments that drain to it. The blocks are cut so that the heaviest is as light as possible, and the others are near even shares of the rest. Rank 0 then sends each rank its part. A divide that drains to no nexus is placed as if it were a nexus of its own.

If the catchment subset CLI arg names catchments, only those catchments are partitioned; the nexus subset arg is ignored. Every rank must receive at least one catchment, so there can be no more ranks than nexuses.

## Examples

### Example - Automatic Partitioning
* the CMake build directory is named `cmake-build/`
* sixteen MPI processes are started
* the hydrofabric GeoPackage and realization config are located in the current working directory
* the driver partitions the hydrofabric automatically
```
mpirun -n 16 cmake-build/ngen hydrofabric.gpkg "all" hydrofabric.gpkg "all" realization_config.json
```
