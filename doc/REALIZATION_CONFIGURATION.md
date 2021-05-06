## Realization Configuration

A Realization Configuration needs to be in [JSON format (JavaScript Object Notation)](https://www.json.org/json-en.html)

The Configuration must contain these three first level objects:
* `"global"` - defines the default formulation and input parameters for any catchment that is not defined in `"catchments"`

* `"time"` - defines the simulation start and end times and the output interval

* `"catchments"` - defines the formulation and input parameters for each individual catchment

The `"global"` object must contain the following two objects:
* `"formulations"` - defines the default required formulation name and also includes subobjects/lists for `parameters`, `options`, `initial_conditions` depending on the required and optional inputs for a given formulation
* `"forcing"` - defines the default file pattern and path relative to the main ngen directory where the driver is executed for the input forcings 

The `"time"` object must contain the following three objects:
* `"start_time"` - The UTC start time of the simulation and must be in the form `"yyyy-mm-dd hh:mm:ss"`
* `"end_time"` -  The UTC end time of the simulation and must be in the form `"yyyy-mm-dd hh:mm:ss"`
* `"output_interval"` - The time interval that model outputs are generated in seconds

The `"catchments"` object must contain a list of all of the catchment objects that will have defined formulations, and each catchment name will have the following format:
* `"cat-"` followed by the unique integer identifier for the catchment

Each catchment object will have the following two objects:
* `"formulations"` - defines the required formulation name and also includes subobjects/lists for `parameters`, `options`, `initial_conditions` depending on the required and optional inputs for a given formulation     
* `"forcing"` - defines the file name and path relative to the main ngen directory where the driver is executed for the input forcings 

An [example realization configuration](https://github.com/NOAA-OWP/ngen/blob/master/data/example_realization_config.json)

BMI is a commonly used model interface and formulation type used in ngen. [BMI documenation](https://github.com/NOAA-OWP/ngen/blob/master/doc/BMI_MODELS.md) with example [Linux realization](https://github.com/NOAA-OWP/ngen/blob/master/data/example_realization_config_w_bmi_c__linux.json) and [macOS realization](https://github.com/NOAA-OWP/ngen/blob/master/data/example_realization_config_w_bmi_c__macos.json)

