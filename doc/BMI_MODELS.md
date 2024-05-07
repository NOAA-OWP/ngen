# BMI External Models

- [BMI External Models](#bmi-external-models)
  - [Summary](#summary)
  - [Formulation Config](#formulation-config)
    - [Required Parameters](#required-parameters)
        - [Parameter Details:](#parameter-details)
    - [Semi-Optional Parameters](#semi-optional-parameters)
    - [Optional Parameters](#optional-parameters)
  - [BMI Models Written in C](#bmi-models-written-in-c)
    - [BMI C Model As Shared Library](#bmi-c-model-as-shared-library)
      - [Dynamic Loading](#dynamic-loading)
    - [BMI C CFE Example](#bmi-c-cfe-example)
    - [BMI C Caveats](#bmi-c-caveats)
      - [BMI C Activate/Deactivation Required in CMake Build](#bmi-c-activatedeactivation-required-in-cmake-build)
      - [Additional Bootstrapping Function Needed](#additional-bootstrapping-function-needed)
        - [Why?](#why)
  - [BMI Models Written in C++](#bmi-models-written-in-c-1)
    - [BMI C++ Model As Shared Library](#bmi-c-model-as-shared-library-1)
      - [Dynamic Loading](#dynamic-loading-1)
      - [Additional Bootstrapping Functions Needed](#additional-bootstrapping-functions-needed)
        - [Why?](#why-1)
    - [BMI C++ Example](#bmi-c-example)
  - [BMI Models Written in Fortran](#bmi-models-written-in-fortran)
    - [Enabling Fortran Integration](#enabling-fortran-integration)
    - [ISO C Binding Middleware](#iso-c-binding-middleware)
    - [A Compiled Shared Library](#a-compiled-shared-library)
      - [Required Additional Fortran Registration Function](#required-additional-fortran-registration-function)
  - [BMI Models Written in Python](#bmi-models-written-in-python)
    - [Enabling Python Integration](#enabling-python-integration)
    - [BMI Python Model as Package Class](#bmi-python-model-as-package-class)
    - [BMI Python Example](#bmi-python-example)
  - [Multi-Module BMI Formulations](#multi-module-bmi-formulations)
    - [Passing Variables Between Nested Formulations](#passing-variables-between-nested-formulations)
      - [How Values Are Orchestrated](#how-values-are-orchestrated)
      - [Look-back and Default Values](#look-back-and-default-values)

## Summary

The basic outline of steps needed to work with an external BMI model is:
* Configure the main formulation/realization config properly for the catchments that will use the generalized BMI realization(s)
* Make sure all the necessary model-specific BMI initialization files are valid and in place
* Take appropriate steps to make model source files accessible as needed (e.g., making sure shared library files are in a known location)
* Be aware of any model-language-specific caveats
  * [Caveats for C language models](#bmi-c-caveats)

[//]: # (TODO: what does the realization config need to look like?)

[//]: # (TODO: Python )

## Formulation Config

The catchment entry in the formulation/realization config must be set to used the appropriate type for the associated BMI realization, via the formulation's `name` JSON element.  E.g.:

```javascript
      //...
      "cat-87": {
           "formulations": [
               {
                   "name": "bmi_c",
                   "params": { ... }
               }
           }
      //...
```

Valid name values for the currently implemented BMI formulation types are:

* `bmi_c++`
* `bmi_c`
* `bmi_fortran`
* `bmi_python`
* `bmi_multi`

Because of the generalization of the interface to the model, the required and optional parameters for all the BMI formulation types are the same.

### Required Parameters
Certain parameters are strictly required in the formulation/realization JSON config for a catchment entry using a BMI formulation type.  Note that there is a slight distinction in "required" between single-module (e.g., `bmi_c`) and multi-module formulations (i.e., `bmi_multi`).  These are summarized in the following table, with the details of the parameters list below.

| Param | Single-Module | Multi-Module |
| ----- | ------------- | ------------ |
| `model_type_name` | :heavy_check_mark: | :heavy_check_mark: |
| `init_config` | :heavy_check_mark: | |
| `uses_forcing_file` | :heavy_check_mark: | |
| `main_output_variable` | :heavy_check_mark: | :heavy_check_mark: |
| `modules` | | :heavy_check_mark: |

##### Parameter Details:

* `model_type_name`
  * string name for the particular backing model type
  * may not be utilized in all cases, but still required
* `init_config`
  * the string path to the BMI initialization config file for the catchment
* `uses_forcing_file`
  * boolean indicating whether the backing BMI model is written to read input forcing data from a forcing file (as opposed to receiving it via getters calls made by the framework)
* `main_output_variable`
  * the name of the BMI variable returned by the catchment formulation's `get_response()` function
  * this is colloquially referred to as the "main" output from the formulation, but is not directly related to catchment output files
    * see details on the [optional](#optional-parameters) `output_variables` config parameter for configuring output
  * the string must match an item return by the relevant variant of the BMI `get_output_var_names()` function
* `modules`
  * a list of individual formulation configs for component modules of a BMI multi-module formulation
  * each item in the list will be another nested JSON config object for a BMI formulation

### Semi-Optional Parameters
There are some special BMI formulation config parameters which are required in certain circumstances, but which are neither *always* required nor required for either all single- or multi-module formulations.  Thus, they do not behave exactly as *Required* params do in the configuration.  However, they should be thought of as de facto required (and will trigger errors when missing) in the specific situations in which they are applicable.

* `forcing_file`
  * string path to the forcing data file for the catchment
  * must be set whenever a model needs to read its own forcings directly
    * this is set/indicated using `uses_forcing_file` as described above
  * the BMI model's initialization config (i.e., `init_config` above) may define an analogous property, and the two should properly correspond in such cases
* `library_file`
  * Path to the library file for the BMI library
  * Required for C-based BMI model formulations
* `registration_function`
  * Name of the [bootstrapping pointer registration function](#additional-bootstrapping-function-needed) in the external module 
  * required for C-based BMI modules if the module's implemented function is not named `register_bmi` as discussed [here](#additional-bootstrapping-function-needed)
  * only needed for C-based BMI modules
* `python_type`
  * Name of the Python class that represents a BMI model, including the package name as appropriate.
  * Required for Python-based BMI modules
  * Only needed for Python-based modules

### Optional Parameters
* `variables_names_map`
  * can specify a mapping of one or more model variable names (input or output) to aliases used as a recognizable identifiers for those variables within the framework
    * e.g.,  `"variables_names_map": {"bmi_var_name_1": "framework_alias_1", "bmi_var_name_2": "framework_alias_2"}`
  * commonly, this is used to map a module's BMI variable names to standard names
    * this kind of mapping is often necessary to inform the framework how to provide an input - e.g., a forcings value - that a BMI module needs for execution
    * the [AorcForcing.hpp](../include/forcing/AorcForcing.hpp) file has a section where several supported standard names are defined and notes  
    * these typically conform to [CSDMS Standard Names](https://csdms.colorado.edu/wiki/CSDMS_Standard_Names)
* `model_params`
  * can specify static or dynamic parameters passed to models as model variables.
  * static parameters are defined inline in the realization config.
  * dynamic parameters are derived from a given source, such as hydrofabric data.
  * if specified, must be within the **params** config level, i.e. within a `"formulations": [..., {..., "params": {..., "model_params": {...}, ...}, ...}, ...]` object.
  * if specified for multi-BMI, must be within the **module-params** config level, i.e. in the **params** config level for a given **module**.
  * e.g.,
    ```jsonc
    // Format: { <variable_name>: <value> }
    "model_params": {
      // Static parameter
      "APCP_Surface": 3.0,

      // Dynamic parameter
      "areasqkm": {
        // where this variable is deriving from, only "hydrofabric" is supported currently
        "source": "hydrofabric",
        // the property name of this value,
        // i.e. what property (area_sqkm) in the source (hydrofabric) maps to our variable (areasqkm)?
        "from": "area_sqkm"
      }
    }
    ```
* `output_variables`
  * can specify the particular set and order of output variables to include in the realization's `get_output_line_for_timestep()` (and similar) function
  * JSON structure should be a list of strings
  * if not present, defaults to whatever it returned by the model's BMI `get_output_var_names()` function *the first time* it is invoked
  * if specified, must be at the **root** level of a formulation object.
  * if specified for multi-BMI, it should be within the **formulation-root** config level, i.e. in the **root** config level for a given **formulation**.
  * e.g.,
    ```jsonc
    // Example for CFE, which has 13 output variables
    "output_variables": ["RAIN_RATE", "Q_OUT"]
    ```
* `output_header_fields`
  * can specify the header strings to use for the realization's printed output (i.e., the value returned by `get_output_header_line()`)
  * JSON structure should be a list of strings
  * when not present, the literal variable names are used
  * when present, does not do any checking for ordering/correspondence compared to the output ordering of the variable values, so users must take care that ordering is consistent
  * if specified, must be at the **root** level of a formulation object.
  * if specified for multi-BMI, it should be within the **formulation-root** config level, i.e. in the **root** config level for a given **formulation**.
  * e.g.,
  ```jsonc
  // Same as `output_variables` example with CFE, but we want to change the formatting
  "output_header_fields": ["rain_rate", "Q"]
  ```
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

* `NGEN_WITH_BMI_C`
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

BMI C functionality will not work (i.e., will not be compiled or executable) unless set to be active in the CMake build.  This requires setting the `NGEN_WITH_BMI_C` CMake cache variable to `ON`.

Conversely, built executables (and perhaps certain build targets) may not function as expected if `NGEN_WITH_BMI_C` is `ON` but the configured shared library is not available.

#### Additional Bootstrapping Function Needed

BMI models written in **C** should implement an extra "registration" function in order to be compatible with NextGen.  By default, this registration function is expected to be:

    Bmi* register_bmi(Bmi *model);

It is possible to configure a different name for the function within the NGen realization config, but the return type and parameter list must be as noted here.

The implemented function must set the member pointers of the passed `Bmi` struct to the appropriate analogous functions inside the model.  E.g., the `initialize` member of the struct: 

    int (*initialize)(struct Bmi *self, const char *bmi_init_config)

needs to be set to the module's function the performs the BMI initialization.  This will probably be something like:

    static int Initialize (Bmi *self, const char *file)

So the registration function may look something like:

    Bmi* register_bmi_cfe(Bmi *model) {
        if (model) {
            ...
            model->initialize = Initialize;
            ...

Full examples for how to write this registration function can be found in the local CFE BMI implementation, specifically in [extern/cfe/src/bmi_cfe.c](../extern/cfe/src/bmi_cfe.c), or in the official CSDMS *bmi-example-c* repo near the bottom of the [bmi-heat.c](https://github.com/csdms/bmi-example-c/blob/master/heat/bmi_heat.c) file.

##### Why?
This is needed both due to the design of the **C** language variant of BMI, and the limitations of C regarding duplication of function names.  The latter becomes significant when more than one BMI C library is used at once.  Even if that is actively the case, NextGen is designed to accomodate that case, so this requirement is in place.

Future versions of NextGen will provide alternative ways to declaratively configure function names from a BMI C library so they can individually be dynamically loaded.

## BMI Models Written in C++

- [BMI C++ Model As Shared Library](#bmi-c-model-as-shared-library-1)
  - [Dynamic Loading](#dynamic-loading-1)
  - [Additional Bootstrapping Functions Needed](#additional-bootstrapping-functions-needed)
    - [Why?](#why-1)
- [BMI C++ Example](#bmi-c-example)

You can implement a model in C++ by writing an object which implements the [BMI C++ interface](https://github.com/csdms/bmi-cxx).

### BMI C++ Model As Shared Library

For **C++** models, the model should be packaged as a pre-compiled shared library. Support for loading of C++ modules/libraries is always enabled, so no build system flags are required. 
#### Dynamic Loading

As noted [above](#semi-optional-parameters), the path to the shared library must be provided in the configuration so that the module can be loaded at runtime.

#### Additional Bootstrapping Functions Needed

BMI models written in **C++** should implement two **C** functions declared with `extern "C"`. These functions instantiate and destroy a **C++** BMI model object. By default, these functions are expected to be named `bmi_model_create` and `bmi_model_destroy`, and have signatures like the following:

```c++
    extern "C"
    {
      /**
      * @brief Construct this BMI instance as a normal C++ object, to be returned to the framework.
      * @return A pointer to the newly allocated instance.
      */
      MyBmiModelClass *bmi_model_create()
      {
        /* You can do anything necessary to set up a model instance here, but do NOT call `Initialize()`. */
        return new MyBmiModelClass(/* e.g. any applicable constructor parameters */);
      }

      /**
        * @brief Destroy/free an instance created with @see bmi_model_create
        * @param ptr 
        */
      void bmi_model_destroy(MyBmiModelClass *ptr)
      {
        /* You can do anything necessary to dispose of a model instance here, but note that `Finalize()` 
         * will already have been called!
        delete ptr;
      }
    }
```

It is possible to configure different *names* for the functions within the NGen realization config by using the keys `create_function` and `destroy_function`, but the return types and parameters must be as shown above.

An example of implementing these functions can be found in the test harness implementation at [/extern/test_bmi_cpp/include/test_bmi_cpp.hpp](../extern/test_bmi_cpp/include/test_bmi_cpp.hpp).
##### Why?

Counterintuitively, loading C++ shared libraries into a C++ executable (such as the NextGen framework) requires the use of standard C functions. This is because all C++ compilers "mangle" the names of C++ functions and classes in order to support polymorphism and other scenarios where C++ symbols are allowed to have the same name (which is not possible in standard C). This "mangling" algorithm is not specified or defined so different compilers may use different methods--and even different versions of the same compiler can vary--such that it is not possible to predict the symbol name for any C++ class or function in a compiled shared library. Only by using `extern "C"` will the compiler produce a library with a predictable symbol name (and no two functions having the `extern "C"` declaration may have the same name!), so this mechanism is used whenever dynamic loading of C++ library classes is needed. 

Similarly, different compilers (or different compiler versions) may implement `delete` differently, or layout private memory of an object differently. This is why the `bmi_model_destroy` function should be implemented in the library where the object was instantiated: to prevent compiler behavior differences from potentially freeing memory incorrectly.

### BMI C++ Example

An example implementation for an appropriate BMI model as a **C++** shared library is provided in the project [here](../extern/test_bmi_cpp).

## BMI Models Written in Python

  - [Enabling Python Integration](#enabling-python-integration)
  - [BMI Python Model as Package Class](#bmi-python-model-as-package-class)
  - [BMI Python Example](#bmi-python-example)

### Enabling Python Integration

Python integration is controlled with the CMake build flag `NGEN_WITH_PYTHON`, however this currently defaults to "On"--you would need to turn this off if Python is not available in your environment. See [the Dependencies documentation](DEPENDENCIES.md#python-3-libraries) for specifics on Python requirements, but in summary you will need a working Python environment with NumPy installed. You can set up a Python environment anywhere with the usual environment variables. The appropriate Python environment should be active in the shell when ngen is run.

For Python BMI models specifically, you will also need to install the [bmipy](https://github.com/csdms/bmi-python) package, which provides a base class for Python BMI models.

### BMI Python Model as Package Class

To use a Python BMI model, the model needs to be installed as a package in the Python environment and the package must have a class that extends bmipy, like so

```python
from bmipy import Bmi
class bmi_model(Bmi):
    ...
```

**TIP:** If you are actively developing a Python BMI model, you may want to [install your package with the `-e` flag](https://pip.pypa.io/en/stable/topics/local-project-installs/#editable-installs).

As noted above, Python modules require the package and class name to be specified in the realization config via the `python_class` key, such as:

```javascript
{
  "global": {
    "formulations": [
        { 
            "name": "bmi_python",
            "params": {
                "python_type": "mypackage.bmi_model",
                "model_type_name": "bmi_model",
                //...
```

### BMI Python Example

An example implementation for an appropriate BMI model as a **Python** class is [provided in the project](../extern/test_bmi_py), or you can examine the CSDMS-provided [example Python model](https://github.com/csdms/bmi-example-python).

## BMI Models Written in Fortran

* [Enabling Fortran Integration](#enabling-fortran-integration)
* [ISO_C_BINDING Middleware](#iso-c-binding-middleware)
* [A Compiled Shared Library](#a-compiled-shared-library)
  * [Required Additional Fortran Registration Function](#required-additional-fortran-registration-function)

### Enabling Fortran Integration

To enable Fortran integration functionality, the CMake build system has to be [generated](BUILDS_AND_CMAKE.md#generating-a-build-system) with the `NGEN_WITH_BMI_FORTRAN` CMake variable set to `ON`.

### ISO C Binding Middleware
Nextgen takes advantage of the Fortran `iso_c_binding` module to achieve interoperability with Fortran modules.  In short, this works through use of an intermediate middleware module maintained within Nextgen.  This module handles the ([majority of the](#required-additional-fortran-registration-function)) binding through proxy functions that make use of the actual external BMI Fortran module.  

The middleware module source is located in _extern/iso_c_fortran_bmi/_.

The proxy functions require an opaque handle to a created BMI Fortran object to be provided as an argument, so such an object and its opaque handle must be setup and returned via a
[`register_bmi` function](#required-additional-fortran-registration-function).

### A Compiled Shared Library
Because of the use of `iso_c_bindings`, integrating with a Fortran BMI module works very similarly to integrating with a C BMI module, where a [shared library](#bmi-c-model-as-shared-library) is [dynamically loaded](#dynamic-loading).  An extra [bootstrapping registration function](#required-additional-fortran-registration-function) is also, again, required.

#### Required Additional Fortran Registration Function
[As with C](#additional-bootstrapping-function-needed), a registration function must be provided by the module, beyond what is implemented for BMI.  It should look very similar to the example below.  In fact, it is likely sufficient to simply modify the `use bminoahowp` and `type(bmi_noahowp), target, save :: bmi_model` lines to suit the module in question.

This function should receive an opaque pointer and set it to point to a created BMI object of the appropriate type for the module.  Note that while `save` is being used in a way that persists only the initial object, since this will be used within the scope of a dynamic library loaded specifically for working with a particular catchment formulation, it should not cause issues.

```fortran
function register_bmi(this) result(bmi_status) bind(C, name="register_bmi")
      use, intrinsic:: iso_c_binding, only: c_ptr, c_loc, c_int
      use bminoahowp
      implicit none
      type(c_ptr) :: this ! If not value, then from the C perspective `this` is a void**
      integer(kind=c_int) :: bmi_status
      !Create the momdel instance to use
      type(bmi_noahowp), target, save :: bmi_model !should be safe, since this will only be used once within scope of dynamically loaded library
      !Create a simple pointer wrapper
      type(box), pointer :: bmi_box

      !allocate the pointer box
      allocate(bmi_box)
      !allocate(bmi_box%ptr, source=bmi_model)
      bmi_box%foobar = 42 
      !associate the wrapper pointer the created model instance
      bmi_box%ptr => bmi_model
      !Return the pointer to box
      this = c_loc(bmi_box)
      bmi_status = BMI_SUCCESS
    end function register_bmi
```

## Multi-Module BMI Formulations
It is possible to configure a formulation to be a combination of several different individual BMI module components.  This is the `bmi_multi` formulation type.  At each time step, formulations of this type proceed through each nested module _in configured order_ and call either the BMI `update()` or `update_until()` function for each.

<details>
<summary>Click here for `bmi_multi` example</summary>

```json
        "formulations": [
            {
                "name": "bmi_multi",
                "params": {
                    "model_type_name": "bmi_multi_pet_cfe",
                    "forcing_file": "",
                    "init_config": "",
                    "allow_exceed_end_time": true,
                    "main_output_variable": "Q_OUT",
                    "modules": [
                        {
                            "name": "bmi_c",
                            "params": {
                                "model_type_name": "PET",
                                "library_file": "bmi_module_libs/libpetbmi.so",
                                "forcing_file": "",
                                "init_config": "config_dir/cat-10_pet_config.txt",
                                "allow_exceed_end_time": true,
                                "main_output_variable": "water_potential_evaporation_flux",
                                "registration_function":"register_bmi_pet",
                                "variables_names_map": {
                                    "water_potential_evaporation_flux": "EVAPOTRANS"
                                },
                                "uses_forcing_file": false
                            }
                        },
                        {
                            "name": "bmi_c",
                            "params": {
                                "model_type_name": "CFE",
                                "library_file": "bmi_module_libs/libcfebmi.so",
                                "forcing_file": "", 
                                "init_config": "config_dir/cat-10_bmi_config_cfe_pass.txt",
                                "allow_exceed_end_time": true,
                                "main_output_variable": "Q_OUT",
                                "registration_function": "register_bmi_cfe",
                                "variables_names_map": {
                                    "atmosphere_water__liquid_equivalent_precipitation_rate": "RAINRATE",
                                    "water_potential_evaporation_flux": "EVAPOTRANS",
                                    "ice_fraction_schaake": "sloth_ice_fraction_schaake",
                                    "ice_fraction_xinan": "sloth_ice_fraction_xinan",
                                    "soil_moisture_profile": "sloth_smp"
                                },  
                                "uses_forcing_file": false
                            }   
                        }, 
                            {
                                "name": "bmi_c++",
                                "params": {
                                    "name": "bmi_c++",
                                    "model_type_name": "SLOTH",
                                    "main_output_variable": "z",
                                    "library_file": "bmi_module_libs/libslothmodel.so",
                                    "init_config": "/dev/null",
                                    "allow_exceed_end_time": true,
                                    "fixed_time_step": false,
                                    "uses_forcing_file": false,
                                    "model_params": {
                                    "sloth_ice_fraction_schaake(1,double,m,node)": 0.0,
                                    "sloth_ice_fraction_xinan(1,double,1,node)": 0.0,
                                    "sloth_smp(1,double,1,node)": 0.0
                                }
                            }
                        }
                    ],
```
</details>

As described in [Required Parameters](#required-parameters), a BMI `init_config` does not need to be specified for this formulation type, but a nested list of sub-formulation configs (in `modules`) does.  Execution of a formulation time step update proceeds through each module in list order.

### Passing Variables Between Nested Formulations
In addition to using framework-supplied forcings as module inputs, a `bmi_multi` formulation orchestrate the output variable values of one nested module for use as the input variable values of another. This imposes some extra conditions and requirements on the configuration.

#### How Values Are Orchestrated

The `bmi_multi` formulation orchestrates values by pairing all nested module input variables with some nested module or framework-provided output variable.  Paring is done by examining the identifiers for the variables - either a variable's alias configured via `variables_names_map` (see [here](#optional-parameters)) or, if the former wasn't provided, its name - and providing each input with values from an output with a matching identifier.  

E.g., if _module_1_ has an output variable with either a name or configured alias of _et_, and _module_2_ has an input variable with a name or an alias of _et_, then the formulation will know to use _module_1.et_ at each time step to set _module_2.et_.

This imposes several constraints on the configuration:

* each nested module output variable identifier must be unique among available outputs in the formulation
  * i.e., there cannot be two nested modules that have an output variable with the same identifier
  * a variable's name is its default identifier
  * if two nest modules both have an output variable with the same name, a mapped alias must be configured for at least one of them via `variables_names_map` (see [here](#optional-parameters))
  * an alias is also needed if an output name matches a framework-provided forcing value
* each input variable identifier must match some output identifier
  * inputs do not require configured aliases, though it is possible to configure an alias to match with the appropriate output identifier (especially for framework-provided forcings)

#### Look-back and Default Values
Any nested module, regardless of its position in the configured order of the modules, may have its provided outputs used as the input values for any other nested module in that formulation.  Because modules are updated in order, configuring an earlier module - e.g., _module_1_ - to receive an input value from an output variable of a later module - e.g., _module_2_ - induces a "look-back" capability.  At the time _module_1_ needs its input for the current time step, _module_2_ will have not yet processed the current time step.  As such, _module_2_'s output variable values will be those from the previous time step.

This introduces a special case for when there is no previous time step.  Not every BMI module used in this kind of "look-back" scenario will be able to provide a valid output value before processing the first time step.  To account for this, an optional default value can be configured for output variables, associated via the identifier (i.e., mapped alias or variable name).  

> [!NOTE]
> Currently only `double` default values are supported.

##### Example Look-Back Config

Below is a partial config example illustrating a look-back setup involving CFE and SoilMoistureProfile (SMP).  CFE relies upon the soil moisture profile value from SMP (mapped in both as `soil_moisture_profile__smp_output__cfe_input`) that was calculated in the previous time step.  Because SMP wasn't implemented to have its own default values for variables, a default value is supplied in the configuration that CFE will use in the first time step.

```javascript
{
  "global": {
    "formulations": [
      {
        "name": "bmi_multi",
        "params": {
          "model_type_name": "bmi_multi_noahmp_cfe",
          "forcing_file": "",
          "init_config": "",
          "allow_exceed_end_time": true,
          "main_output_variable": "Q_OUT",
          "default_output_values": [
            {
              "name": "soil_moisture_profile__smp_output__cfe_input",
              "value": 1.2345
            }
          ],
          "modules": [
            {
              "name": "bmi_c",
              "params": {
                "model_type_name": "bmi_c_cfe",
                "library_file": "./ngen/extern/cfe/cmake_build/libcfebmi",
                "forcing_file": "",
                "init_config": "./configs/cfe/cat-20521.txt",
                "allow_exceed_end_time": true,
                "main_output_variable": "Q_OUT",
                "registration_function": "register_bmi_cfe",
                "variables_names_map": {
                  "soil_moisture_profile": "soil_moisture_profile__smp_output__cfe_input"
                  "SOIL_STORAGE": "soil_storage__cfe_output__smp_input",
                  "SOIL_STORAGE_CHANGE": "soil_storage__cfe_output__smp_input",
                },
                "uses_forcing_file": false
              }
            },
            {
              "name": "bmi_c++",
              "params": {
                "model_type_name": "bmi_smp",
                "library_file": "./ngen/extern/SoilMoistureProfiles/cmake_build/libsmpbmi",
                "init_config": "./configs/smp_cfe/cat-20521.txt",
                "allow_exceed_end_time": true,
                "main_output_variable": "soil_water_table",
                "variables_names_map" : {
                  "soil_storage" : "soil_storage__cfe_output__smp_input",
                  "soil_storage_change" : "soil_storage__cfe_output__smp_input",
                  "soil_moisture_profile": "soil_moisture_profile__smp_output__cfe_input"
                },
                "uses_forcing_file": false
              }
            }
          ]
...
```

> [!IMPORTANT]
> As mentioned, even for such look-back scenarios, default output values are not strictly required.
> 
> The BMI specification does not restrict when values for a variable may be available, except to say that the `initialize()` function may need to be called first.  In other words, some modules may be able to provide their own appropriate default variable values, before the first time step update.
> 
> To provide general support, `bmi_multi` formulation must allow for this scenario, although users should be very careful to not accidentally omit configuring default values for a module that does not supply them on its own.

> [!WARNING]  
> Users should not assume that the error-free execution of a configuration with nested module look-back and without default values implies that the module was designed to provided _correct_ default values.
