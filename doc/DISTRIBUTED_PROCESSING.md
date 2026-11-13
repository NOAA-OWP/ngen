# Distributed Processing

Distributed multi-process execution of the Nextgen driver (_ngen_) is possible through use of an implementation of MPI.  

* [Overview](#overview)
* [Building the MPI-Enabled Nextgen Driver](#building-the-mpi-enabled-nextgen-driver)
* [Running the MPI-Enabled Nextgen Driver](#running-the-mpi-enabled-nextgen-driver)
  * [Automatic Partitioning](#automatic-partitioning)
  * [Subdivided Hydrofabric](#subdivided-hydrofabric)
      * [Driver Runtime Differences](#driver-runtime-differences)
      * [File Names](#file-names)
      * [On-the-fly Generation](#on-the-fly-generation)
  * [Examples](#examples)
    * [Example 1 - Full Hydrofabric](#example-1---full-hydrofabric)
    * [Example 2 - Subdivided Hydrofabric](#example-2---subdivided-hydrofabric)
    * [Example 3 - Automatic Partitioning](#example-3---automatic-partitioning)
* [Partition Config Generator](#partitioning-config-generator)

# Overview

The basic design starts with partitioning: grouping the catchments and nexuses of the entire hydrofabric into separate collections, one per MPI rank.  Every nexus must be in the same partition as all of the catchments that drain to it, so that no rank needs to communicate with another during the simulation.  The driver can [partition automatically](#automatic-partitioning), or read the partitions from a partitioning configuration file.  In a file, partitions have an `id` property, and it is expected for partitions and MPI ranks to map 1-to-1 and have the same identifier.  A file whose partitions list remote connections between them is rejected.

Partition configurations are expected to be represented in JSON.  There is a [partition generator tool](#partitioning-config-generator) available separately from the main driver for creating these files.

When executed, the driver is provided a valid partitioning configuration file path [as a command line arg](../README.md#usage).  Each rank loads [all or part](#subdivided-hydrofabric) of the supplied hydrofabric, and then constructs the specific model formulations appropriate for the features within its partition.

# Building the MPI-Enabled Nextgen Driver

To enable distributed processing capabilities in the driver, the [CMake build must be generated](BUILDS_AND_CMAKE.md) with the `NGEN_WITH_MPI` variable set to `ON`, such as `NGEN_WITH_MPI:BOOL=ON` and the necessary MPI libraries must be available to CMake.  Additionally, [certain features](#on-the-fly-generation) are only supported if `NGEN_WITH_PYTHON` is also `ON`.

Otherwise, if using the standard build process with CMake, the driver is built using the same commands.  It can be built either as part of the entire project or the `ngen` CMake target.  CMake manages the necessary adjustments to, e.g., the compiler used, etc. 

# Running the MPI-Enabled Nextgen Driver

* The MPI-enabled driver is run by wrapping within the `mpirun` command, supplying also the number of processes to start.  
* An additional driver [CLI arg](../README.md#usage) may supply the path to the partitioner config. Without it, or given `-` in its place, the driver [partitions automatically](#automatic-partitioning).
* Another flag may also optionally be provided as a CLI arg to [adjust the way the driver processes load the hydrofabric](#subdivided-hydrofabric).

## Automatic Partitioning
Given no partition config path, or `-` in its place, the driver partitions the hydrofabric itself during initialization. Every nexus is assigned to one rank, along with all of the catchments that drain to it. Rank 0 reads only the divides' connectivity, without their geometry or other attributes, and divides the nexuses among the ranks in contiguous blocks, in the order the hydrofabric stores them, weighting each nexus by the number of catchments that drain to it. The blocks are cut so that the heaviest is as light as possible, and the others are near even shares of the rest. Rank 0 then sends each rank its part. A divide that drains to no nexus is placed as if it were a nexus of its own.

If the catchment subset CLI arg names catchments, only those catchments are partitioned; the nexus subset arg is ignored. Every rank must receive at least one catchment, so there can be no more ranks than nexuses. This cannot be combined with `--subdivided-hydrofabric`, which requires a partition config.

## Subdivided Hydrofabric
Certain hydrofabrics may currently require too much memory to be fully loaded by each individual MPI rank/process, which is the default behavior prior to initializing individual catchment formulations.  To work around this, it is possible to include the following flag as the final command line argument:

`--subdivided-hydrofabric`

This will indicate to the driver processes that the provided, full hydrofabric files should not be directly loaded.  Instead, rank/process/partition specific files should be used.  Each of these contain a subdivided, non-overlapping portion of the entire hydrofabric and are expected to correspond directly to a partition.

### Driver Runtime Differences

When subdivided hydrofabrics should be used, the driver processes will first check to see if the necessary subdivided hydrofabric files already exist.  If they do not, and the functionality for doing so is enabled, the driver will [generate them](#on-the-fly-generation).  If a subdivided hydrofabric is required, but files are not available and cannot be generated, the driver exits in error.

### File Names

The name of an expected or generated subdivided hydrofabric file is based on two things:
* the name of the complete hydrofabric file from which is its data is obtained
* the partition/rank id
* 
This will have a partition specific suffix but otherwise have the same name as the full hydrofabric files.  E.g., _catchment_data.geojson.0_ would be the subdivided hydrofabric file for _catchment_data.geojson_ specific to rank 0.  

### On-the-fly Generation
Driver processes may, under certain conditions, be able to self-subdivide a hydrofabric and generate the files when necessary.  For this to be possible, the executable must have been built with Python support (via the CMake `NGEN_WITH_PYTHON` being set to `ON`), and the [required package](DEPENDENCIES.md#the-dmodsubsetservice-package) must be installed within the Python environment available to the driver processes.

## Examples

### Example 1 - Full Hydrofabric
* the CMake build directory is named `cmake-build/`
* four MPI processes are started
* the catchment and nexus hydrofabric files, realization config, and partition config have intuitive names and are located in the current working directory
* all processes completely load the entire hydrofabric
```
mpirun -n 4 cmake-build/ngen catchment_data.geojson "all" nexus_data.geojson "all" realization_config.json partition_config.json
```

### Example 2 - Subdivided Hydrofabric
* the CMake build directory is named `cmake-build/`
* eight MPI processes are started
* the catchment and nexus hydrofabric files, realization config, and partition config have intuitive names and are located in the current working directory
* each process only load files for a subdivided portion of the hydrofabric that corresponds to a given process's partition
```
mpirun -n 8 cmake-build/ngen catchment_data.geojson "all" nexus_data.geojson "all" realization_config.json partition_config.json --subdivided-hydrofabric
```

### Example 3 - Automatic Partitioning
* the CMake build directory is named `cmake-build/`
* sixteen MPI processes are started
* the hydrofabric GeoPackage and realization config are located in the current working directory
* the driver partitions the hydrofabric automatically, with no partition config
```
mpirun -n 16 cmake-build/ngen hydrofabric.gpkg "all" hydrofabric.gpkg "all" realization_config.json
```

# Partitioning Config Generator

A separate artifact can be built for generating partition configs.  This is the _partitionGenerator_ executable, built in the CMake build directory either when the entire project or specifically the `partitionGenerator` CMake target is built.  The syntax for using is:

`<cmake-build-dir>/partitionGenerator <catchment_data_file> <nexus_data_file> <output_partition_config> <num_partitions> '' ''`

E.g.:

`./cmake-build-debug/partitionGenerator ./data/huc01_hydrofabric/catchment_data.geojson ./data/huc01_hydrofabric/nexus_data.geojson ./partition_config.json 4 '' ''`

The last two arguments are intended to allow for partitioning only a subset of the entire hydrofabric.  Note also that single-quotes must be used.  At this time, these are required, but it is recommended they be left as empty strings.  
