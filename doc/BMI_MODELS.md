# BMI External Models

* [Summary](#summary)
* [Formulation Config](#formulation-config)
    * [Required Parameters](#required-parameters)
    * [Semi-Optional Parameters](#semi-optional-parameters)
    * [Optional Parameters](#optional-parameters)
* [BMI Models Written in C](#bmi-models-written-in-c)
    * [BMI C Model As Shared Library](#bmi-c-shared-library)
    * [Example: CFE Shared Library](#bmi-c-cfe-example)
    * [BMI C Caveats](#bmi-c-caveats)

## Summary

The basic outline of steps needed to work with an external BMI model is:
  * Configure the main formulation/realization config properly for the catchments that will use the generalized BMI realization(s) 
  * Make sure all the necessary model-specific BMI initialization files are valid and in place
  * Take appropriate steps to make model source files accessible as needed (e.g., making sure shared library files are in a known location)
  * Be aware of any model-language-specific caveats 
    * [Caveats for C language models](#bmi-c-caveats)

[//]: # (TODO: what does the realization config need to look like?)

[//]: # (TODO: Python, C++, and Fortran )

## Formulation Config

The catchment entry in the formulation/realization config must be set to used the appropriate type for the associated BMI realization, via the formulation's `name` JSON element.  E.g.:

      ...
      "cat-87": {
           "formulations": [
               {
                   "name": "bmi_c",
                   "params": { ... }
               }
           }
      ...
  
Valid name values for the currently implemented BMI formulation types are:

* `bmi_c`

Because of the generalization of the interface to the model, the required and optional parameters for all the BMI formulation types are the same.  

### Required Parameters
The following must be present in the formulation/realization JSON config for all catchment entries using the BMI formulation type:

* `model_type_name`
  * string name for the particular backing model type
  * may not be utilized in all cases, but still required
* `forcing_file`
  * string path to the forcing data file for the catchment
  * the `init_config` file below will likely reference this file also, and the two should properly correspond
  * this is needed in here so the realization can directly access things implicit in the file, like times, time step amounts, and time step sizes
* `uses_forcing_file`
  * boolean indicating whether the backing BMI model is written to read input data from the forcing file (as opposed to receiving it via getters)
* `init_config` 
  * the string path to the BMI initialization config file for the catchment
* `main_output_variable` 
  * the string value of the primary output variable
  * this is the value that returned by the realization's `get_response()`
  * the string must match an item return by the relevant variant of the BMI `get_output_var_names()` function

### Semi-Optional Parameters
There are some special config parameters which are not *always* required in BMI formulation configs, but are in some circumstances.  Thus, they do not strictly behave exactly as *Required* params do in the configuration, but they should be thought of as required (and will trigger errors when missing) in certain situations.

* `library_file`
  * Path to the library file for the BMI library
  * Required for C-based BMI model formulations

### Optional Parameters
* `other_input_variables`
  * this may be provided to set certain model variables more directly after its `initialize` function
  * JSON structure should be one or more nested JSON nodes, keyed by the variable name, and with the variable values contained as a list
  * e.g.,  `"other_input_variables": {"ex_var_1": [0, 1, 2]}`
* `output_variables`
  * can specify the particular set and order of output variables to include in the realization's `get_output_line_for_timestep()` (and similar) function
  * JSON structure should be a list of strings
  * if not present, defaults to whatever it returned by the model's BMI `get_output_var_names()` function *the first time* it is invoked
* `output_header_fields`
  * can specify the header strings to use for the realization's printed output (i.e., the value returned by `get_output_header_line()`)
  * JSON structure should be a list of strings
  * when not present, the literal variable names are used
  * when present, does not do any checking for ordering/correspondence compared to the output ordering of the variable values, so users must take care that ordering is consistent
* `allow_exceed_end_time`
  * boolean value to specify whether a model is allowed to execute `Update` calls that go beyond its end time (or the max forcing data entry)
  * implied to be `false` by default
* `fixed_time_step`
  * boolean value to indicate whether this model has a fixed time step size
  * implied to be `true` by default
  
## BMI Models Written in C

* [Model As Shared Library](#bmi-c-model-as-shared-library)
* [Example](#bmi-c-cfe-example)
* [Caveats](#bmi-c-caveats)

### BMI C Model As Shared Library

For **C** models, the model must be packaged as a pre-compiled shared library.  A CMake cache variables can be configured for controlling whether the framework functionality for working with BMI C libraries is activated.  This is found in, or must be added to, the _CMakeCache.txt_ file in the build system directory:

* `BMI_C_LIB_ACTIVE` 
  * type: `BOOL` 
  * must be set to `ON` (or equivalent in CMake) for BMI C shared library functionality to be compiled and active
  
The CMake build system may need to be [regenerated](BUILDS_AND_CMAKE.md#regenerating) after changing these settings.

See the CMake documentation on the *[set](https://cmake.org/cmake/help/latest/command/set.html)* function or [variables](https://cmake.org/cmake/help/latest/manual/cmake-language.7.html#cmake-language-variables) for more information on working with CMake variables. 

When CMake is able to find the library for the given name, it will automatically set up [the dependent, internal, static library](../src/realizations/catchment/CMakeLists.txt) to dynamically link to the external shared library at runtime.  

#### Dynamic Loading

Additionally, as noted [above](#semi-optional-parameters), the path to the shared library must be provided in the configuration. This is because C libraries must be loaded dynamically within the execution of the NextGen framework, or else certain limitations of C would prevent using more than one such external C BMI model library at a time.

### BMI C CFE Example

An example implementation for an appropriate BMI model as a **C** shared library is provided in the project [here](../extern/cfe).

### BMI C Caveats

* [Activation/Deactivation in CMake Required](#bmi-c-activatedeactivation-required-in-cmake-build)
* [Additional Bootstrapping Function Needed](#additional-bootstrapping-function-needed)

#### BMI C Activate/Deactivation Required in CMake Build

BMI C functionality will not work (i.e., will not be compiled or executable) unless set to be active in the CMake build.  This requires setting the `BMI_C_LIB_ACTIVE` CMake cache variable to `ON` or `TRUE` (or equivalent).  

Conversely, built executables (and perhaps certain build targets) may not function as expected if `BMI_C_LIB_ACTIVE` is `ON` but the configured shared library is not available.

#### Additional Bootstrapping Function Needed

BMI models written in **C** should provide one extra function in order to be compatible with NextGen: 
    
    Bmi* register_bmi(Bmi *model);

This function must set the member pointers of the passed `Bmi` struct param to the appropriate analogous functions.  E.g., the `initialize` member of the the model, defined fully as:
 
    int (*initialize)(struct Bmi *self, const char *config_file)
 
 needs to be set to the function for initializing models.  This will probably be something like:
 
    static int Initialize (Bmi *self, const char *file)

Examples for how to write this registration function can be found in the local CFE BMI implementation, specifically in [extern/cfe/src/bmi_cfe.c](../extern/cfe/src/bmi_cfe.c), or in the official CSDMS *bmi-example-c* repo near the bottom of the [bmi-heat.c](https://github.com/csdms/bmi-example-c/blob/master/heat/bmi_heat.c) file.

##### Why?
This is needed both due to the design of the **C** language variant of BMI, and the limitations of C regarding duplication of function names.  The latter becomes significant when more than one BMI C library is used at once.  Even if that is actively the case, NextGen is designed to accomodate that case, so this requirement is in place.  

Future versions of NextGen will provide alternative ways to declaratively configure function names from a BMI C library so they can individually be dynamically loaded.
