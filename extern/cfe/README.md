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