## Realization Configuration

TODO: Link to Doxygen build

A Realization Configuration needs to be in [JSON format (JavaScript Object Notation)](https://www.json.org/json-en.html)

The Configuration is a key-value object and must contain these three first level object keys:
* `global` 
  * is a key-value object that must include an object key for `formulations` that defines the default formulation(s) and also an object key for `forcing` that defines the default forcing file name pattern and path for any catchment that is not defined in `catchments`
  * Note: `global` can be omitted only if every catchment is assigned a formulation 
        
* `time`
  * is a key-value object that defines the simulation start and end times and the output interval

* `catchments` 
  *  is a key-value object that must include a list of individual catchments

```
{
   "global": {},
   "time": {},
   "catchments": {}
} 
```

The `global` key-value object must contain the following two object keys:
* `formulations` 
  * a list of formulation key-value objects that defines the default required formulation(s), and each formulation object has a key `name` and value of a model that is registered with the ngen framework and includes a key-value subobject for `params` 
  * Note: future versions could support breaking up `params` into additional key-value subobjects for `options` and `initial_conditions`
  * `params` must be a list that holds key-value pairs
* `forcing`
  * key-value object with keys for `file_pattern` and `path` that define the default CSV file pattern and path for the input forcings relative to the executable directory

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

The `catchments` key-value object must contain a list of all of the catchment object keys that will have defined formulations, and each catchment key will have the following format:
* `cat-` 
  * followed by the unique integer identifier for the catchment

Each catchment is a key-value object and must have the following two object keys:
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

An [example realization configuration](https://github.com/NOAA-OWP/ngen/blob/master/data/example_realization_config.json).

BMI is a commonly used model interface and formulation type used in ngen. [BMI documenation](https://github.com/NOAA-OWP/ngen/blob/master/doc/BMI_MODELS.md) with example [Linux realization](https://github.com/NOAA-OWP/ngen/blob/master/data/example_realization_config_w_bmi_c__linux.json) and [macOS realization](https://github.com/NOAA-OWP/ngen/blob/master/data/example_realization_config_w_bmi_c__macos.json).

