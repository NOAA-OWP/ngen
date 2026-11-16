# Distributed Processing

Distributed multi-process execution of the Nextgen driver (_ngen_) is possible through use of an implementation of MPI.  

* [Overview](#overview)
* [Building the MPI-Enabled Nextgen Driver](#building-the-mpi-enabled-nextgen-driver)
* [Running the MPI-Enabled Nextgen Driver](#running-the-mpi-enabled-nextgen-driver)
  * [Automatic Partitioning](#automatic-partitioning)
  * [Examples](#examples)
    * [Example 1 - Partition Config](#example-1---partition-config)
    * [Example 2 - Automatic Partitioning](#example-2---automatic-partitioning)
* [Partition Config Generator](#partitioning-config-generator)

# Overview

The basic design starts with partitioning: grouping the catchments and nexuses of the entire hydrofabric into separate collections, one per MPI rank.  Every nexus must be in the same partition as all of the catchments that drain to it, so that no rank needs to communicate with another during the simulation.  The driver can [partition automatically](#automatic-partitioning), or read the partitions from a partitioning configuration file.  In a file, partitions have an `id` property, and it is expected for partitions and MPI ranks to map 1-to-1 and have the same identifier.  A file whose partitions list remote connections between them is rejected.

Partition configurations are expected to be represented in JSON.  There is a [partition generator tool](#partitioning-config-generator) available separately from the main driver for creating these files.

When executed, the driver is provided a partitioning configuration file path [as a command line arg](../README.md#usage), or partitions automatically without one.  Each rank reads the features of its partition from the supplied hydrofabric, and then constructs the specific model formulations appropriate for the features within its partition.

# Building the MPI-Enabled Nextgen Driver

To enable distributed processing capabilities in the driver, the [CMake build must be generated](BUILDS_AND_CMAKE.md) with the `NGEN_WITH_MPI` variable set to `ON`, such as `NGEN_WITH_MPI:BOOL=ON` and the necessary MPI libraries must be available to CMake.

Otherwise, if using the standard build process with CMake, the driver is built using the same commands.  It can be built either as part of the entire project or the `ngen` CMake target.  CMake manages the necessary adjustments to, e.g., the compiler used, etc. 

# Running the MPI-Enabled Nextgen Driver

* The MPI-enabled driver is run by wrapping within the `mpirun` command, supplying also the number of processes to start.  
* An additional driver [CLI arg](../README.md#usage) may supply the path to the partitioner config. Without it, or given `--auto-partition` in its place, the driver [partitions automatically](#automatic-partitioning).

## Automatic Partitioning
Given no partition config path, or `--auto-partition` in its place, the driver partitions the hydrofabric itself during initialization. Every nexus is assigned to one rank, along with all of the catchments that drain to it. Rank 0 reads which nexus each divide drains to, and divides the nexuses among the ranks in contiguous blocks, in the order the hydrofabric stores them, weighting each nexus by the number of catchments that drain to it. The blocks are cut so that the heaviest is as light as possible, and the others are near even shares of the rest. Rank 0 then sends each rank its part. A divide that drains to no nexus is placed as if it were a nexus of its own.

If the catchment subset CLI arg names catchments, only those catchments are partitioned; the nexus subset arg is ignored. Every rank must receive at least one catchment, so there can be no more ranks than nexuses.

## Examples

### Example 1 - Partition Config
* the CMake build directory is named `cmake-build/`
* four MPI processes are started
* the catchment and nexus hydrofabric files, realization config, and partition config have intuitive names and are located in the current working directory
* each process reads the features of its partition from the hydrofabric files
```
mpirun -n 4 cmake-build/ngen catchment_data.geojson "all" nexus_data.geojson "all" realization_config.json partition_config.json
```

### Example 2 - Automatic Partitioning
* the CMake build directory is named `cmake-build/`
* sixteen MPI processes are started
* the hydrofabric GeoPackage and realization config are located in the current working directory
* the driver partitions the hydrofabric automatically, with no partition config
```
mpirun -n 16 cmake-build/ngen hydrofabric.gpkg "all" hydrofabric.gpkg "all" realization_config.json
```

# Partitioning Config Generator

A separate artifact can be built for generating partition configs.  This is the _partitionGenerator_ executable, built in the CMake build directory either when the entire project or specifically the `partitionGenerator` CMake target is built.  It partitions a hydrofabric as the driver does when it [partitions automatically](#automatic-partitioning), and writes the partitions to a file, which can then be inspected or reused.  The syntax for using is:

`<cmake-build-dir>/partitionGenerator <catchment_data_file> <nexus_data_file> <output_partition_config> <num_partitions> '' ''`

E.g.:

`./cmake-build-debug/partitionGenerator ./data/huc01_hydrofabric/catchment_data.geojson ./data/huc01_hydrofabric/nexus_data.geojson ./partition_config.json 4 '' ''`

The last two arguments are intended to allow for partitioning only a subset of the entire hydrofabric: the first lists catchment IDs to partition, while the second, for nexus IDs, is ignored, since each nexus is partitioned with the catchments that drain to it.  Note also that single-quotes must be used.  At this time, these are required, but it is recommended they be left as empty strings.
