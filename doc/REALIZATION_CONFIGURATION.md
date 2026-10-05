# Realization Configuration

TODO: Link to Doxygen build

# Description and Top-Level Structure

A Realization Configuration needs to be in [JSON format (JavaScript Object Notation)](https://www.json.org/json-en.html)

## Required Top-Level Keys

The Configuration is a key-value object and must contain these three first level object keys:
* `global` 
  * is a key-value object that must include an object key for `formulations` that defines the default formulation(s) and also an object key for `forcing` that defines the default forcing file name pattern, path, and provider for any catchment that is not defined in `catchments`
  * Note: `global` can be omitted only if every catchment is assigned a formulation 
        
* `time`
  * is a key-value object that defines the simulation start and end times and the output interval

* `catchments` 
  *  is a key-value object that must include a list of individual catchments

## Optional Top-Level Keys

### `output`
The configuration may optionally contain an `output` object that controls where and how catchment and nexus output are written. All fields are optional:

* `root` — root output directory for both catchment and nexus output; created if it does not already exist (default: the working directory, `./`).
* `precision` — number of significant digits used when a text (CSV) backend renders values (default: `9`).
* `catchment` and `nexus` — per-domain settings, each an object with:
  * `enable` — whether this domain's output is written (default: `true`; set `catchment.enable` to `false` to skip catchment files).
  * `format` — serialization format, `"csv"` (default) or `"netcdf"`. NetCDF is currently supported for nexus output only.
  * `grouping` — how a domain's records are distributed across files (the file-*granularity* axis):
    * `"per_feature"` (default) — one file per catchment / per nexus.
    * `"per_formulation"` — the catchments of a formulation aggregate into one file with a leading `catchment_id` column (one file per formulation). Fewer, larger files, with columns uniform within each file by construction. (Nexus CSV aggregation is not yet implemented.)
  * `rank_subdir` — whether an MPI rank's files are placed under a `rank_<N>/` subdirectory (the filesystem-*layout* axis, orthogonal to `grouping`; default `false`). Only affects distributed (multi-rank) runs — a no-op in serial — and reduces filesystem contention when many ranks share a directory. For `per_formulation` under MPI it is applied automatically regardless of this setting, since each rank must write its own file to avoid collisions.

```
"output": {
    "root": "/path/to/output/",
    "catchment": { "enable": true, "format": "csv", "grouping": "per_formulation", "rank_subdir": true },
    "nexus":     { "enable": true, "format": "csv", "grouping": "per_feature",     "rank_subdir": true }
}
```

> [!NOTE]
> The `output` object supersedes the top-level `output_root`, `disable_catchment_output`, and `per_formulation_nexus_files` keys documented below. Those remain supported for backward compatibility and are honored only when no `output` object is present (with a deprecation warning); new configurations should use `output`.

### `output_root`
_Deprecated: prefer `output.root`._ The configuration may optionally contain an `output_root` key with a user-defined root output directory as the key, for nexus and catchment outputs.

### `disable_catchment_output`
_Deprecated: prefer `output.catchment.enable` set to `false`._ The configuration may optionally contain a `disable_catchment_output` key, with a boolean value.  When set to `true`, catchment output data files will not be written (default: `false`).

### `per_formulation_nexus_files`
_Deprecated: prefer `output.nexus.format` set to `"netcdf"`._ The configuration may optionally contain a `per_formulation_nexus_files` key with a boolean value to indicate per-formulation, NetCDF files should be used for writing nexus data, rather than the default of per-nexus CSV files.  Note that if `per_formulation_nexus_files` is set to `true`, the `catchments` cannot be used to define formulations for individual catchments, and the global formulation config must be used for all catchments.

> [!IMPORTANT]
> NetCDF support must be turned on for the ngen build to use this option for per-formulation NetCDF file.  This is done by including the `-DNGEN_WITH_NETCDF=ON` arg to CMake on the command line when generating a build directory.
> 
> In MPI builds (i.e., `-DNGEN_WITH_MPI=ON`), per-formulation NetCDF files are written by gathering each timestep's nexus data to rank 0, which writes the file via the standard NetCDF C API. To instead have every rank write its own slice in parallel via HDF5-parallel I/O, opt in with `-DNGEN_WITH_PARALLEL_NETCDF=ON`; this additionally requires the NetCDF library itself to provide [parallel I/O support](DEPENDENCIES.md#parallel-netcdf).

### `catchments`
The configuration may optionally contain a `catchments` key with a list of individual catchments that define their own formulations.  See [more details below](#catchments).

### `routing`
The configuration may optionally contain a `routing` key with a subobject that defines the path to the t-route config file (`t_route_config_file_with_path`).  It also optionally may contain a path to the t-route source code (`t_route_connection_path`), but this is reserved for advanced usage; generally, t-route should be installed as a package in the normal Python environment.

### `auxiliary_hydrofabric_attributes`
The configuration may optionally contain an `auxiliary_hydrofabric_attributes` key with a list of GeoPackage *attributes* tables whose columns are joined onto the catchment features before the formulations are built. Values that live outside the `divides` layer, such as regionalized parameters, then resolve for `model_params` entries with `"source": "hydrofabric"` like any other layer column.

Each list entry is an object with the following fields:

* `table` — name of the attributes table in the GeoPackage. **Required.**
* `alias` — short stand-in for the table name when namespacing the joined columns (default: none, i.e. the table name is used).
* `file` — path to the GeoPackage holding `table` (default: the catchment data file given on the command line).
* `key_column` — column whose values are matched against catchment feature ids (default: `divide_id`).
* `required` — whether a catchment with no row in `table` is an error rather than a warning (default: `false`).

Joined columns are namespaced: each non-key column of a matched row becomes the feature property `<prefix>.<column>`, where `<prefix>` is the entry's `alias` when declared and otherwise its `table` name, e.g. `donor.real_value`. That is what allows tables sharing column names to be joined together, so prefixes must be unique across entries; two entries resolving to the same prefix is a configuration error. A cell holding SQL `NULL`, or anything else that is not an integer, a real number or text, yields no property, rather than a stand-in valued one.

Strictness is per entry. A catchment with no matching row emits a `WARNING:` line on standard error, which a build configured with `-DNGEN_QUIET=ON` silences along with every other warning, and is left without those properties; `required` set to `true` makes it a fatal error instead. Rows whose key matches no catchment are ignored silently, so a table covering an entire hydrofabric can be used with a subset run. A property missing from a catchment is not fatal downstream either: the `model_params` entry referencing it is reported as a skipped parameter, leaving the model's own default in place.

Entries are applied in declared order, and only to the catchments being simulated, so under MPI each rank joins against its own partition's subset with no additional configuration.

Because the join reads a GeoPackage, the following are errors:

* the GeoPackage to be read cannot be opened, whether because `file` names nothing readable or because what it names is not a database;
* the named `table`, or its `key_column`, does not exist in the GeoPackage being read;
* `table` names one of SQLite's own internal tables (any name beginning `sqlite_`), which are not hydrofabric attributes;
* `table` holds more than one row keyed to a catchment being simulated, since nothing in the table says which of them the catchment's value comes from;
* a joined column's `<prefix>.<column>` name is already a property of the catchment, whether it came from the hydrofabric layer or an earlier entry, since the joined value would otherwise be dropped without a word;
* an entry declares no `file` while the catchment data file given on the command line is not a GeoPackage (an entry that does declare a `file` is fine in that case, and can pull attributes from a GeoPackage alongside a GeoJSON fabric);
* the key is present at all in a build without SQLite support (i.e., built without `-DNGEN_WITH_SQLITE=ON`), rather than the declarations being silently ignored.

A worked example, pairing a declaration with a `model_params` entry that consumes it:

```jsonc
{
    "auxiliary_hydrofabric_attributes": [
        {
            "table": "divide-attributes-regionalized",
            "alias": "reg",
            "file": "./data/hydrofabric/regionalization.gpkg",
            "key_column": "divide_id",
            "required": true
        }
    ],
    "global": {
        "formulations": [
            {
                "name": "bmi_c",
                "params": {
                    "model_type_name": "bmi_c_cfe",
                    // ... remaining formulation params ...
                    "model_params": {
                        // "reg" is the alias declared above, "bexp" a column of that table
                        "b": { "source": "hydrofabric", "from": "reg.bexp" },
                        // a column of the divides layer needs no prefix
                        "areasqkm": { "source": "hydrofabric", "from": "area_sqkm" }
                    }
                }
            }
        ]
    }
}
```

See [`model_params`](BMI_MODELS.md#optional-parameters) for the general form of a dynamic model parameter.

## Examples of Top-Level Structure
Note that these are not exhaustive examples.


```
{
   "global": {},
   "time": {},
   "catchments": {},
   "output": { "root": "/path/to/output/" }
} 
```
or, using the deprecated top-level keys (still supported when no `output` object is present):
```
{
   "global": {},
   "time": {},
   "output_root": "/path/to/output/",
   "per_formulation_nexus_files": true|false
} 
```

# The Global Section

The `global` key-value object must contain the following two object keys:
* `formulations` 
  * a list of formulation key-value objects that defines the default required formulation(s), and each formulation object has a key `name` and value of a model that is registered with the ngen framework and includes a key-value subobject for `params` 
  * Note: future versions could support breaking up `params` into additional key-value subobjects for `options` and `initial_conditions`
  * `params` must be a list that holds key-value pairs
* `forcing`
  * key-value object with keys for `file_pattern` and `path` that define the default CSV file pattern and path for the input forcings relative to the executable directory. More recently, `ngen` developed the capability to handle forcing data in different formats. Thus, a `provider` value parameter can be used to explicitly define the format of the forcing data, such as NetCDF format, in the form "provider": "NetCDF".

```
"global": {
  "formulations": [
    {
        "name": "tshirt_c",
        "params": {
            "maxsmc": 0.439,
            "wltsmc": 0.066,
            "satdk": 0.00000338
        ---continued---
    }
  ],
  "forcing": {
      "file_pattern": ".*{{id}}.*.csv",
      "path": "./data/forcing/"
  }
},  
```

# The Time Section

The `time` key-value object must contain the following three keys:
* `start_time`
  * defines the UTC start time of the simulation and must be in the form `yyyy-mm-dd hh:mm:ss`
* `end_time`
  * defines the UTC end time of the simulation and must be in the form `yyyy-mm-dd hh:mm:ss`
* `output_interval`
  * defines the time interval that model outputs are generated in seconds

```
"time": {
    "start_time": "2015-12-01 00:00:00",
    "end_time": "2015-12-30 23:00:00",
    "output_interval": 3600
},
```

# Individual Catchments

The `catchments` key-value object must contain a list of all of the catchment object keys that will have defined formulations, and each catchment key will have the following format:
* `cat-` 
  * followed by the unique integer identifier for the catchment

Each catchment is a key-value object and must have the following two object keys, similar to the `global` section:
* `formulations`
  * a list of formulation key-value objects that defines the required formulation(s), and each formulation object has a key `name` and value of a model that is registered with the ngen framework and includes a key-value subobject for `params`
  * Note: future versions could support breaking up `params` into additional key-value subobjects for `options` and `initial_conditions`
  * `params` must be a list that holds key-value pairs
     
* `forcing`
  * key-value object with a key for `path` that defines the CSV file name and path for the input forcings relative to the executable directory

```
"catchments": {
    "cat-27": {
        "formulations": [
            {
                "name": "bmi_c",
                "params": {
                    "model_type_name": "bmi_c_cfe",
                    "library_file": "./extern/cfe/cmake_cfe_lib/libcfemodel.so",
                    "forcing_file": "./data/forcing/cat-27_2015-12-01 00_00_00_2015-12-30 23_00_00.csv",
                    "init_config": "./data/bmi/c/cfe/cat_27_bmi_config.txt",
                    "main_output_variable": "Q_OUT",
                    "uses_forcing_file": true
                }
            }
        ],
        "forcing": {
            "path": "./data/forcing/cat-27_2015-12-01 00_00_00_2015-12-30 23_00_00.csv"
        }
    },
    "cat-52": {
      "formulations": [
        {
          "name": "simple_lumped",
          "params": {
              "sr": [
                  1.0,
                  1.0,
                  1.0
              ],
              "storage": 1.0,
              "max_storage": 1000.0,
              "a": 1.0,
              "b": 10.0,
              "Ks": 0.1,
              "Kq": 0.01,
              "n": 3,
              "t": 0
        }
      }
    ],
    "forcing": {
        "path": "./data/forcing/cat-52_2015-12-01 00_00_00_2015-12-30 23_00_00.csv"
      }
    },
```

# The Routing Section

```json
    "routing": {
        "t_route_config_file_with_path": "extern/t-route/test/input/yaml/ngen.yaml"
    }
```

# A Full Example

An [example realization configuration](https://github.com/NOAA-OWP/ngen/blob/master/data/example_realization_config.json).

# A Note on BMI Models

BMI is a commonly used model interface and formulation type used in ngen. [BMI documenation](https://github.com/NOAA-OWP/ngen/blob/master/doc/BMI_MODELS.md) with an example [for both Linux and macOS realizations](https://github.com/NOAA-OWP/ngen/blob/master/data/example_realization_config_w_bmi_c__lin_mac.json).

