# BMI External Models

* [Summary](#summary)
* [Formulation Config](#formulation-config)
    * [Required Parameters](#required-parameters)
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

For **C** models, the model must be packaged as a pre-compiled shared library.  Several CMake cache variables must be configured for controlling whether to expect such a library and how to find it.  These are found in, or must be added to, the _CMakeCache.txt_ file in the build system directory:

* `BMI_C_LIB_ACTIVE` 
  * type: `BOOL` 
  * must be set to `ON` (or equivalent in CMake) for BMI C shared library functionality to be compiled and active
* `BMI_C_LIB`
  * type: `FILEPATH`
  * library target value used for linking 
  * typically the compiled library file
  * when not set, can potentially be derived/found using `BMI_C_LIB_NAME` and `BMI_C_LIB_DIR`
* `BMI_C_LIB_NAME`
  * type: `STRING`
  * must be set if `BMI_C_LIB_ACTIVE` is `ON` but `BMI_C_LIB` is not set
  * supplies the name for the shared library for use when finding
* `BMI_C_LIB_DIR`
  * type: `STRING`
  * may be set to provide an explicit path for CMake to use when attempting to find the library
  * only searched after "default" paths (e.g., those in `CMAKE_PREFIX_PATH`)

  
The CMake build system may need to be [regenerated](BUILDS_AND_CMAKE.md#regenerating) after changing these settings.

See the CMake documentation on the *[set](https://cmake.org/cmake/help/latest/command/set.html)* function or [variables](https://cmake.org/cmake/help/latest/manual/cmake-language.7.html#cmake-language-variables) for more information on working with CMake variables. 

When CMake is able to find the library for the given name, it will automatically set up [the dependent, internal, static library](../src/realizations/catchment/CMakeLists.txt) to dynamically link to the external shared library at runtime.  

If `BMI_C_LIB_ACTIVE` is set to `ON`, but either `BMI_C_LIB_NAME` is not set or no library of that name can be found, builds for most (if not all) targets will fail.

See [caveat](#only-one-generic-bmi-c-at-a-time) about only using one **C** BMI model at a time with the built-in realization.

### BMI C CFE Example

An example implementation for an appropriate BMI model as a **C** shared library is provided in the project [here](../extern/cfe).

### BMI C Caveats

* [Activation/Deactivation in CMake Required](#bmi-c-activatedeactivation-required-in-cmake-build)
* [Only One Generic BMI C at a Time](#only-one-generic-bmi-c-at-a-time)
* [Additional Bootstrapping Function Needed](#additional-bootstrapping-function-needed)

#### BMI C Activate/Deactivation Required in CMake Build

BMI C functionality will not work (i.e., will not be compiled or executable) unless set to be active in the CMake build.  This requires setting the `BMI_C_LIB_ACTIVE` CMake cache variable to `ON` or `TRUE` (or equivalent).  It will probably also be necessary to configure other settings, as [described here](#bmi-c-model-as-shared-library). 

Conversely, built executables (and perhaps certain build targets) will not work if `BMI_C_LIB_ACTIVE` is `ON` but the configured shared library is not available.

#### Only One Generic BMI C at a Time

At the time of this writing, NextGen can only support one generic external **C** BMI model library in use at a time.  

This strictly applies to using a model with the built-in generalized BMI realization, which is the only external **C** model integration currently supported.  Other experimental strategies may be possible, but they currently are outside the scope of this doc and not (yet) officially supported.

#### Additional Bootstrapping Function Needed

BMI models written in **C** must provide one extra function in order to be compatible with NextGen: 
    
    Bmi* register_bmi(Bmi *model);

This function must set the member pointers of the passed `Bmi` struct param to the appropriate analogous functions.  E.g., the `initialize` member of the the model, defined fully as:
 
    int (*initialize)(struct Bmi *self, const char *config_file)
 
 needs to be set to the function for initializing models.  This will probably be something like:
 
    static int Initialize (Bmi *self, const char *file)

##### Why?
In the **C** language variant of BMI, the interface is  provided by the `Bmi` struct definition, with it essentially having  function pointers as its members.  However, because of the nature of **C**, the struct definition cannot provide implementations for the functions themselves.

This is easy enough to remedy via a bootstrapping function that assigns function implementations to the pointer for a particular `Bmi` struct instance.  This function is not strictly part of the BMI spec, though, but it is necessary to have a BMI **C** model work using the generic NextGen realization.
