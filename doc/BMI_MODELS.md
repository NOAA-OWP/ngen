# BMI External Models

* [Summary](#summary)
* [BMI Models Written in C](#bmi-models-written-in-c)

## Summary

The basic outline of steps needed to work with an external BMI model is:
  * Configure the main realization config properly for the catchments that will use the generalized BMI realization(s) 
  * Make sure all the necessary model-specific BMI initialization files are valid and in place
  * Take appropriate steps to make model source files accessible as needed (e.g., making sure shared library files are in a known location)
  * Be aware of any model-language-specific caveats 
    * [Caveats for C language models](#bmi-c-caveats)

[//]: # (TODO: what does the realization config need to look like?)

[//]: # (TODO: Python, C++, and Fortran )

## BMI Models Written in C

* [Shared Library](#bmi-c-shared-library)
* [Example](#bmi-c-cfe-example)
* [Caveats](#bmi-c-caveats)

### BMI C Shared Library

For **C** models, the model must be packaged as a pre-compiled shared library.  Several CMake cache variables must be configure for controlling whether to expect such a library and how to find it:

* `BMI_C_LIB_ACTIVE` 
  * type: `BOOL` 
  * must be set to `ON` (or equivalent in CMake) for BMI C shared library functionality to be compiled and active
* `BMI_C_LIB_NAME`
  * type: `STRING `
  * must be set if `BMI_C_LIB_ACTIVE` is `ON` to supply the appropriate name for the shared library
* `BMI_C_LIB_HINT_DIR`
  * type: `STRING`
  * may be set to provide a [hint](https://cmake.org/cmake/help/latest/command/find_library.html) to CMake when it tries to find the library
  
The CMake build system may need to be [regenerated](BUILDS_AND_CMAKE.md#regenerating) after changing these settings.

See the CMake documentation on the *[set](https://cmake.org/cmake/help/latest/command/set.html)* function or [variables](https://cmake.org/cmake/help/latest/manual/cmake-language.7.html#cmake-language-variables) for more information on working with CMake variables. 

When CMake is able to find the library for the given name, it will automatically set up [the dependent, internal, static library](../src/realizations/catchment/CMakeLists.txt) to dynamically link to the external shared library at runtime.  

If `BMI_C_LIB_ACTIVE` is set to `ON`, but either `BMI_C_LIB_NAME` is not set or no library of that name can be found, builds for most (if not all) targets will fail.

See [caveat](#only-one-generic-bmi-c-at-a-time) about only using one **C** BMI model at a time with the built-in realization.

### BMI C CFE Example

An example implementation for an appropriate BMI model as a **C** shared library is provided in the project [here](../extern/cfe).

### BMI C Caveats

* [Only One Generic BMI C at a Time](#only-one-generic-bmi-c-at-a-time)
* [Additional Bootstrapping Function Needed](#additional-bootstrapping-function-needed)

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
