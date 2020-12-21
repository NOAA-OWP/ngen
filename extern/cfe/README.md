# CFE BMI Shared C Library Example

* [About](#about)
* [Building](#building)
  * [Generating a Build System Directory](#generating-a-build-system-directory)
  * [Building the Shared Library](#building-the-shared-library)
* [Initialization File Structure](#initialization-file-structure)
  * [General Form](#general-form)
  * [Value Formats](#value-formats)
  * [Params](#params)
    * [Required](#required)
    * [Optional](#optional)
* [Implementation Details](#implementation-details)
  * [Empty Functions](#empty-functions)
  * [Model Time Units](#model-time-units)
  * [The `update_until` Function](#the-update_until-function)

# About

This directory holds sources and build files for a C-language implementation of the CFE (or T-shirt) model that also fulfills the BMI specification.  The BMI fulfillment is strictly valid, though only an essential subset of functions are currently fully implemented.  

The build configuration is also set up to generate a shared library artifact that can be used in the main NextGen framework.  

# Building

To generate the shared library files, build the `cfemodel` target in the generated build system.  This needs to be separate from the main NextGen build system.

#### Generating a Build System Directory

Run from the project root directory:

    cmake -B extern/cfe/cmake_cfe_lib -S extern/cfe
    
To regenerate, simply remove the `extern/cfe/cmake_cfe_lib` directory and run the command again.
 
#### Building the Shared Library

Again, from the project root directory:

    cmake --build cmake_cfe_lib --target cfemodel

This should generate the appropriate library files for your system.

# Initialization File Structure

### General Form

* it is a text file of key-value pairs separated by `=`
* it *does not* (necessarily) support spaces between the key/value and the `=`
  * i.e., do not include spaces except as part of a value if appropriate
* it *does not* support comments
  * i.e., don't include a line unless it is intended to configure something
* there are some keys that have two possible forms that may be used
* there is no intelligence built in to handle cases when a param is supplied more than once
  * i.e., the last appearance will be used, but please don't do this
  
### Value Formats
    * `forcing_file` expects a string path to a file
    * `gw_storage` and `soil_storage` expect either a numeric literal or a percentage of the maximum storage as a special string
        * a percentage string will be something like `66.7%`, where the last character is `%`
    * `nash_storage` and `giuh_ordinates` expect a comma-separated series of numeric literals
    * all other values expect numeric literals (e.g. 1.0, 4.678)

### Params

#### Required

* `forcing_file`
* `soil_params.depth` or `soil_params.D`
* `soil_params.b` or `soil_params.bb`
* `soil_params.multiplier` or `soil_params.mult`
* `soil_params.satdk`
* `soil_params.satpsi`
* `soil_params.slope` or `soil_params.slop`
* `soil_params.smcmax` or `soil_params.maxsmc`
* `soil_params.wltsmc`
* `max_gw_storage`
* `Cgw`
* `expon`
* `gw_storage`
* `alpha_fc`
* `soil_storage`
* `K_nash`
* `giuh_ordinates`

#### Optional

* `refkdt`
    * defaults to `3.0`
* `nash_storage` 
    * defaults to `0.0` in all reservoirs
* `number_nash_reservoirs` or `N_nash`
    * if `nash_storage` is present, it implies this value
    * otherwise, a default of `2` is used
    
# Implementation Details

As a reminder, the BMI spec for C is implemented as a C struct declaration.  The struct has a `void*` member for holding a pointer to some data structure for the model (here, a `cfe_model` struct, declared in [include/cfe.h](include/cfe.h)), and a series of function pointers to the functions necessary to fulfill BMI.  A separate function in the implementing model library - here `register_bmi_cfe()` in [src/bmi_cfe.c](src/bmi_cfe.c) - must then sets those function pointers to backing function definitions.

For this CFE model, the implemented functions in general follow the BMI documented spec, so in most cases that [documentation](https://bmi.readthedocs.io/en/latest/) is sufficient for understanding this model's operation.  There are a few things worth of more detailed explanation however, which are covered here.

#### Empty Functions

Several BMI functions currently have incomplete implementations.  In such cases, the `BMI_FAILURE` code will always be immediately returned.  

These can be identified by examining the setting of the function pointers for the BMI API C struct, handled by the `register_bmi_cfe()` function in [src/bmi_cfe.c](src/bmi_cfe.c).  The functions in question have comments noting their incomplete implementations.

#### Model Time Units

The time unit used by the model is seconds.


#### The `update_until()` Function

The `update_until()` function can update the model to a semi-arbitrary (but valid) future time, instead of just to the point in time after the next time step.  This future time can be passed explicitly as a model time or implicitly as a valid number of time steps into the future.  

Such a time must be valid for this particular CFE model instance, however.  To be valid, it must be possible to arrive at this same time by making some number of calls to `update()`.

The model tests to determine if the param is a valid explicit value first, before considering whether it is a valid implicit value.

##### Behavioral Notes

* No changes to model state will occur until the function has determined the parameter is a valid future time
* While valid and resulting in successful return, the model does not change state if the time param is equal to the current model time
* A parameter value less than the current model time can not be a valid explicit time, but it can be a valid implicit time
    * Corollary 1: the function will either return error or behave unexpectedly if an explicit model time in the past is supplied
    * Corollary 2: if the model's `current_time` is at or beyond its `end_time`, this function will always return error
* Negative parameter values are not valid for either explicit or implicit representations
* To be valid, an implicit time must be an integral values
* To be valid, an implicit time must not take the model beyond its expected total number of time steps