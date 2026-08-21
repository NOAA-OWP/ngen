All notable changes to this project will be documented in this file.
We follow the [Semantic Versioning 2.0.0](http://semver.org/) format.


## x.y.z - YYYY-MM-DD

### Added

- An `output` configuration block for realization configs: `root`, per-domain (`catchment`/`nexus`) `enable`/`format`/`grouping`/`rank_subdir`, and a global value `precision`. See [Realization Configuration](doc/REALIZATION_CONFIGURATION.md#output).
- Catchment output routed through a pluggable output manager (CSV backend), with an optional `grouping: per_formulation` that aggregates a formulation's catchments into one file with a leading `catchment_id` column (columns uniform per file by construction).
- Per-domain `rank_subdir` option placing each MPI rank's CSV output under a `rank_<N>/` subdirectory (applied automatically for `per_formulation` under MPI).
- Initial hydrofabric v4.0 support, alongside the existing v2.2 and v4.0beta1 schemas.
- A `HydrofabricReader` abstraction covering both supported hydrofabric formats, with GeoJSON and GeoPackage implementations selected by `make_hydrofabric_reader()`. Both drivers now obtain their catchment and nexus features through it instead of dispatching on the file extension themselves.

### Changed

- A GeoPackage with no `nexus` layer is no longer read as a hydrofabric. It was previously treated as v2.2 and read anyway; it is now reported as not a hydrofabric. Reading such a file as a plain GeoPackage of arbitrary layers is still supported, through `ngen::geopackage::GeoPackageReader`, which carries no hydrofabric knowledge at all.
- Catchment and nexus data paths must name the same format. Passing a GeoPackage for one and a GeoJSON file for the other is now an error rather than being honored per path.
- The `INFO: hydrofabric detected: ...` line now reports for either format and fires exactly once per run. GeoJSON hydrofabrics previously reported nothing, and a single-file GeoPackage hydrofabric reported twice, once per file opened.

### Deprecated

- Top-level realization config keys `output_root`, `disable_catchment_output`, and `per_formulation_nexus_files`, superseded by the `output` block (still honored when no `output` block is present).

### Removed

- Nothing.

### Fixed

- Hydrofabric schema detection now reads each layer's columns from the file that holds it. A v4.0 hydrofabric split across a catchment file and a nexus file was previously detected as v4.0beta1, because `divides.flowpath_toid` -- the column that separates the two -- is absent from the nexus file; its divides were then joined through a `flowpaths` table that need not be there, leaving them with no downstream reference.
