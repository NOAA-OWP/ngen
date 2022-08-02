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
  * the string value of the primary output variable
  * this is the value that returned by the realization's `get_response()`
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
  * can specify a mapping of model variable names (input or output) to supported standard names
  the [Bmi_Formulation.hpp](..include/realizations/catchment/Bmi_Formulation.hpp) file has a section where several supported standard names are defined and notes  
  * this can be useful in particular for informing the framework how to provide the input a model needs for execution
  * e.g.,  `"variables_names_map": {"model_variable_name": "standard_variable_name"}`
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

Python integration is controlled with the CMake build flag `NGEN_ACTIVATE_PYTHON`, however this currently defaults to "On"--you would need to turn this off if Python is not available in your environment. See [the Dependencies documentation](DEPENDENCIES.md#python-3-libraries) for specifics on Python requirements, but in summary you will need a working Python environment with NumPy installed. You can set up a Python environment anywhere with the usual environment variables, but note that ngen will look for a Python environment in the build directory named `.venv` or `venv` by default. The appropriate Python environment should be active in the shell when ngen is run.

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

To enable Fortran integration functionality, the CMake build system has to be [generated](BUILDS_AND_CMAKE.md#generating-a-build-system) with the `NGEN_BMI_FORTRAN_ACTIVE` CMake variable set to `ON`.

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
It is possible to configure a formulation to be a combination of several different individual BMI module components.  This is the `bmi_multi` formulation type.

As described in [Required Parameters](#required-parameters), a BMI `init_config` does not need to be specified for this formulation type, but a nested list of sub-formulation configs (in `modules`) does.  Execution of a formulation time step update proceeds through each module in list order.

A few other items of note:

* there are some constraints on input and output variables of the sub-modules of a multi-module formulation
  * output variables must be uniquely traceable
    * there must not be any output variable from a sub-module for which there is another output variable in a different sub-module that has the same config-mapped alias
    * when nothing is set for a variable in ``variables_names_map``, its alias is equal to its name
    * when this doesn't hold, a unique mapped alias must be configured for one of the two (i.e., either an alias added, or changed to something different)
  * input variables must have previously-identified data providers
    * every input variable must have its alias match a property from an output data source 
    * one way this works is if the alias is equal to the name of an externally available forcing property (e.g., AORC read from file) defined before the sub-module 
    * another way is if the input variable's alias matches the alias of an output variable from an earlier sub-module
      * in this case, one sub-module's output serve as a later sub-modules input for each multi-module formulation update
      * since modules get executed in order of configuration, "earlier" and "later" are with respect to the order they are defined in the ``modules`` config list
* the framework allows independent configuration of the `uses_forcing_file` property among the individual sub-formulations, although this is not generally recommended
* configuration of `variables_names_map` maps a given variable to a variable name of the directly 
* it is now possible to have an earlier nested module use as a provider (for one of its inputs) a later nested module, as long as a default value is configured
  * a collection of variable default values can be given in the formulation config at the top level by providing an entry in `default_output_values` with the variable's mapped configuration alias (or just variable name if it is unique) and the default value:
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
              "name": "QINSUR",
              "value": 42.0
            }
          ],
          "modules": [
...
```



