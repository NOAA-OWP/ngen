All notable changes to this project will be documented in this file.
We follow the [Semantic Versioning 2.0.0](http://semver.org/) format.


## x.y.z - YYYY-MM-DD

### Added

- An `output` configuration block for realization configs: `root`, per-domain (`catchment`/`nexus`) `enable`/`format`/`grouping`/`rank_subdir`, and a global value `precision`. See [Realization Configuration](doc/REALIZATION_CONFIGURATION.md#output).
- Catchment output routed through a pluggable output manager (CSV backend), with an optional `grouping: per_formulation` that aggregates a formulation's catchments into one file with a leading `catchment_id` column (columns uniform per file by construction).
- Per-domain `rank_subdir` option placing each MPI rank's CSV output under a `rank_<N>/` subdirectory (applied automatically for `per_formulation` under MPI).

### Deprecated

- Top-level realization config keys `output_root`, `disable_catchment_output`, and `per_formulation_nexus_files`, superseded by the `output` block (still honored when no `output` block is present).

### Removed

- Nothing.

### Fixed

- Nothing.
