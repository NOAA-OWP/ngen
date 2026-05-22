# BMI Conventions

The [BMI Documentation](https://bmi.readthedocs.io/en/stable/) enumerates all of the functions in the BMI specification and the [BMI Best Practices](https://bmi.readthedocs.io/en/stable/bmi.best_practices.html#best-practices) page goes into additional detail about how the interface should be used, but in many cases BMI is not prescriptive about behavior, leaving choices up to model and tool developers. For instance, the Best Practices document states,

> All functions in the BMI must be implemented. For example, even if a model operates on a [uniform rectilinear](https://bmi.readthedocs.io/en/stable/model_grids.html#uniform-rectilinear) grid, a [get_grid_x](https://bmi.readthedocs.io/en/stable/index.html#get-grid-x) function has to be written. This function can be empty and simply return the BMI_FAILURE status code or raise a NotImplemented exception, depending on the language.

Strictly speaking, while all functions must have some implementation, *none* of the functions in the interface are specifically called out as required to do anything other than return a failure--though that would not be a very useful model. So, in these and other cases, we must specify if any of the BMI functions are required to behave in a certain way to function properly with the NextGen Water Resources Modeling Framework's Model Engine (hereafter "ngen" the executable name), and these requirements are what this document aims to spell out.

# Compliance with the BMI v2.0 Standard

BMI modules written in C, Fortran, and C++ *must* be compiled with the exact same header files as those used with ngen itself, and this means the BMI v2.0 header files provided in the CSDMS [`bmi-c`](https://github.com/csdms/bmi-c/blob/e6f9f8a0ab326218831000b4a5571490ebc21ea2/bmi.h), [`bmi-fortran`](https://github.com/csdms/bmi-fortran/blob/e34cd026f57cabd009ef0f662fc672baee66e442/bmi.f90), and [`bmi-cxx`](https://github.com/csdms/bmi-cxx/blob/be631dc510b477b3bc3eb3c8bbecf3d04ec4005c/bmi.hxx) repositories. Python models must inherit from the `Bmi` class in the [`bmipy`](https://pypi.org/project/bmipy/2.0/) package. Python and C++ classes can be subclasses of the standard BMI interface, but such subclasses must inherit from the standard versions listed here.

This means&mdash;among other things&mdash;that it is not possible to "extend" the BMI interface simply by adding functions to the base interface or header files. In any case, even with subclassed BMI models in Python and C++ with additional functions, ngen would not call any such additional functions because it does not know that they exist. 

# Time Control

## Set and Get Values Per Timestep 

Importantly, for your model to work in ngen, all input values and output values pertain to the processing to be done *in a single engine timestep*. Put another way, a model running in ngen should always follow this general execution pattern:

 * `initialize(...)` and any other setup at the beginning of the model, such as `get_input_var_names(...)`
 * Now we will loop over the whole simulation time one timestep per iteration...
   * `set_value(...)` set the inputs for the next timestep
   * `update_until(...)` increment to the next timestep--computation happens here
   * `get_value(...)` get the model outputs for the just-processed timestep

Other patterns may be allowed by the BMI without being prescriptive, such as passing a whole time series of inputs as an array to `set_value(...)` but this is not supported by ngen. Passing of arrays as input is supported but is applicable in cases where the whole array applies to one timestep (e.g. soil moisture profile or snow layer information for multiple depths).

## `update` vs. `update_until`

For the foreseeable future, ngen will always call `update_until(...)` and will not call a model's `update()` method. Per the [BMI documentation](https://bmi.readthedocs.io/en/stable/#update-until) for `update_until`, an absolute time is passed as the argument, not an incremental time; i.e., if the model start time is `0.0` and ngen is incrementing timesteps by 3600 seconds, the first three iterations will call `update_until` with the values `3600.0`, `7200.0`, and `10800.0`.

At present, ngen will always pass a value greater than the previous value passed to `update_until` to any model instance (it will move only forward in time).

## Time Units

At present, all BMI modules must use seconds as the time units. The output of `get_time_units()` should be `seconds` and the value passed to `update_until(...)` should be interpreted as seconds past time `0` (see below). This is something that we intend to make more flexible in the future.

## Model time begin

At present, all BMI models *must* use `0.0` as their start time. The result of `get_start_time()` should be `0.0`. This is something that we intend to make more flexible in the future. 

## Model time end

Usually, models should not need to provide an end time and should be able to compute results for any range that ngen provides input values for. Unless your model has some specific maximum time, you should return the system's double/float max value (probably about `1.79769e+308`) as the result of `get_end_time()`.

# Metadata

## Variable Units

Models should implement `get_var_units(...)` for *all* supported variables, including all names returned by `get_input_var_names(...)` and `get_output_var_names(...)` or other names that may be used with `get_var(...)` and `set_var(...)` (see below). Unit strings should be parsable and convertible by the UDUNITS2 library using its [provided units library](https://ncics.org/portfolio/other-resources/udunits2/).

> **HINT:** A common pitfall is using `C` for "Celsius", but this is not valid and may be being interpreted as the prefix "centi" or as "number of times the speed of light". Use `degC` or `celsius` instead.

Per the [BMI documentation]() for `get_var_units(...)`:
> * Dimensionless quantities should use "" or "1" as the unit.
> * Variables without units should use "none".

We prefer `1` for dimensionless units.

If units are *not* provided (i.e. `get_var_units(...)` returns `BMI_FAILURE` or throws an exception) or the unit string is not parsable, this will not prevent the use of the model in ngen, but ngen will report a large number of warning messages and **the values will be passed to other parts of the system without any unit conversion**. The same will happen if you configure ngen to couple any pair of quantities whose units are not convertible.

Note that the `get_var_units(...)` function will be called regardless and that there is a fast-passthrough optimization for cases where the unit conversion strings are exactly the same, so there is effectively no downside to providing units in every case, and doing so is **strongly encouraged**.


## Variable Types and Sizes

Models *must* implement [`get_var_itemsize(...)`](https://bmi.readthedocs.io/en/stable/#get-var-itemsize), [`get_var_nbytes(...)`](https://bmi.readthedocs.io/en/stable/#get-var-nbytes), and [`get_var_type(...)`](https://bmi.readthedocs.io/en/stable/#get-var-type) for *all* supported variables, including all names returned by `get_input_var_names(...)` and `get_output_var_names(...)` or other names that may be used with `get_var(...)` and `set_var(...)` (see below).

Since the [BMI Documentation] simply states that, "Use of native language type names is encouraged...", the values you return for this function will depend on the language of the model *module*. However, ngen (written in C++) has to recognize each type. Currently the following data type identifiers are supported:

* C
    * all permissible C basic types
    * `get_var_itemsize` result **must** match the size of the data type in the compiler used to build ngen
* C++
    * all permissible C basic types
    * `get_var_itemsize` result **must** match the size of the data type in the compiler used to build ngen
* Fortran
    * `integer`, `real`, `double precision` 
    * also accepts `int`, `float`, and `double`
    * `get_var_itemsize` result **must** match the size of the data type in the compiler used to build ngen
* Python
    * `int`, `long`, `long long`, `int64`, `longlong`, `float`, `float64`, `long double`
    * also accepts the following numpy types
        - `numpy.float64` and `np.float64` for double precision floats
        - `numpy.float32`, `np.float32`, `numpy.single`, and `np.single` for single precision floats
    
    Use of these namespaced types should be limited to variables that are implemented as numpy arrays in the model, as the data type is part of the of the array metadata.  E.g.
    ```python
        import numpy as np
        variable = np.ndarray((1,1), dtype=np.single)
        print(variable.dtype)
    ```
    will produce `dtype('float32')`.

    For variables where the dtype is not directly accessed from the numpy meta data, this usage is discouraged! Please use the non-namespaced names above.

# Input and output published variables

The BMI documentation says that input variables should be declared for a model by returning the list of inputs via `get_input_var_names`, and outputs likewise via `get_output_var_names`. However, it does not specify any difference in behavior for variables published by these functions. In practice this has few technical implications--for instance, it is not explicitly forbidden to call `get_value` on an input variable, nor even to call `set_value` on an output variable. 

Also, it is not forbidden to call `get_value` and `set_value` for variable names that appear in neither returned list. In the remainder of this document, such variables will be referred to as "unpublished" variables, and variables appearing in either `get_*_var_names` list as "published".

The openness of the BMI in these areas both adds some danger and some flexibility, which ngen will mitigate and leverage with the conventions defined below.

## Input/output variables and the time loop

Variables published via `get_input_var_names` and `get_output_var_names` are assumed to be part of the function of the time loop execution of the simulation, e.g. forcing inputs or model quantity outputs. Other types of variables (configuration settings, file names, a version identifier, etc.) should not be published in these functions. See below for impacts of this assumption and potential pitfalls.

## Validation of formulations for input variables

For all model variables returned from `get_input_var_names`, a realization configuration *must* provide inputs for those variables (either by implicit name matching or explicit name mapping). If a module in a realization publishes a variable name as an input requirement, and an input source to provide the value cannot be found, validation will fail and the simulation will not run.

## Validation of formulations for output variables

Unlike input variables, variables with the names returned from `get_output_var_names` are not required to be consumed by another module or output writer, and an unused/unconnected output variable will not cause a validation failure. However, if a variable from a module is consumed in the realization config (by another module or an output writer), that variable *must* be listed in `get_output_var_names`--otherwise validation will fail *even if the source module could respond to a request for it via* `get_value`. In other words, you cannot use an unpublished variable as input to another module or to be captured as output.

## Calling `set_value` on output variables

The ngen process should not call `set_value` on an output variable. Attempting to create a formulation that connects forcing or another module's output to an input name that is in `get_output_var_names` risks undefined behavior.

## Calling `get_value` on input variables

The ngen process *may* call `get_value` on a variable that is returned from `get_input_var_names`. A model *should* return the last value provided to the input variable, unmodified, in this case. At a minimum, a model *must* not crash if this occurs, and should return some reasonable value, rather than throwing an exception or returning BMI_FAILURE (throwing an exception or returning BMI_FAILURE *is* however an appropriate and preferred response if no value has yet been set for the variable).

## Input and output with the same name not permitted

A single variable name cannot be published in both `get_var_input_names` and `get_var_output_names`--this will cause a validation error. Furthermore, it is not presently possible to work around this situation with variable name mapping, because <!-- TODO: This may change with a config file format refactor? --> a variable name mapping does not specify whether it is an input or output variable and could therefore not map only one or the other.

# Unpublished variables

As discussed above, it is not defined by the BMI specification that you may *only* call `get_value` and `set_value` on variable names returned by either `get_input_var_names` or `get_output_var_names`. We refer to variables that appear in neither list--but are understood by a BMI module--as "unpublished" variables. Because ngen uses the input and output variable listings to validate formulations, there are many use cases for such variables, for instance:

* Initialization parameters
* Calibratable parameters
* State inspection (for serialization or debugging)
* Metadata variables (spatial reference information, hyperparameters, etc.) 

These and other uses may be applied by ngen. Specifically, ngen will make use of specified, reserved unpublished variables enumerated later in this document. Therefore, a module should be able to communicate via unpublished variables or at least not malfunction in the case of unpublished variables being used, as described below.

Note that there is currently no discoverability mechanism for unpublished variables, they must be described in documentation (such as this document or a module's  documentation) and often manually implemented or configured for use (such as in a realization config file).

## Unknown/unexpected variables passed to `set_value`

Importantly, if an unexpected variable name is passed to `set_value` this *must* not cause an error or instability! Consider the unknown unpublished value to be "offered" and it can be ignored<!-- TODO: " (unless otherwise specified for specific variable names later in this document)"? We might have a required one at some point. -->. Generally, the preferred behavior in the case of an unknown variable is to do nothing (i.e. throw no exception) and return `BMI_FAILURE`&mdash;ngen will interpret this return value as indicating that the variable just set is not supported by the module.

This scenario should be avoided by ngen, but there may be cases where ngen does not yet know whether a module supports an unpublished variable and will try to set it. These attempts can be ignored if the module does not support or understand the variable. (Returning `BMI_SUCCESS` from `set_value` does *not* imply that the module understands/supports the variable, see below section on probing.)

## Must support variable metadata functions

If a module *does* support an unpublished variable, it is not sufficient to *only* implenent it in `set_value`--as described above, the variable metadata functions (`get_var_type`, `get_var_itemsize`, `get_var_nbytes`) *must* be implemented and return correct values for the variable. Implementation of `get_var_units` is also strongly encouraged.

# Array representation

## Contiguousness

In order to pass arrays back and forth between modules and ngen (and indrectly from BMI modules to other BMI modules), it is necessary that the arrays being passed are stored in contiguous memory blocks in a known layout. This does not necessarily require that this is how data is stored in memory and used for computation within the model, but when passing data through BMI functions array data must conform to this constraint.

## Zero-Indexing

Arrays should be zero-indexed (the index of the first item is 0, not 1 or some other number) and indexes appearing in the BMI functions (e.g. [`get_value_at_indices`](https://bmi.readthedocs.io/en/stable/#get-value-at-indices)) must be treated as zero-based indices. Of the supported languages, this mainly affects Fortran developers, as Fortran uses 1-based indices by default.

## Layout and Flattening

### C ordering required

The [BMI best practices](https://bmi.readthedocs.io/en/stable/bmi.best_practices.html) document states:

> BMI functions always use flattened, one-dimensional arrays...It’s the developer’s responsibility to ensure that array information is flattened/redimensionalized in the correct order.

However, strictly speaking, *how* a multi-dimensional array should be flattened into a one-dimensional one is never directly addressed. **For ngen, it is required that flattening happens as if the array was a contiguous C array, sometimes referred to as "row major order".** That is, if you create a contiguous (i.e. not using pointers) multi-dimensional array in C (or a C array in C++) it will already be in the appropriate order and layout such that if the data is copied into a 1D array (or the pointer is passed as the result of `get_value_ptr`) it is already in the correct order/layout and will "just work".

However, if you have a multi-dimensional array in Fortran or in Python/NumPy it may *not* be in the correct order. In Fortran, arrays are created in memory in "column major order"--however if you treat the first (left-most) index as the fastest changing, it is the same thing as C ordering where the C code would treat the last (right-most) index as the fastest changing. In Python, NumPy `ndarray`s are created as contiguous C-ordered blocks by default, but it is possible to create Fortran-ordered arrays, and if you take a view or slice of an array it is no longer a contiguous array and can't be passed without copying.

### Proper ordering example

The simplest way to explain the proper ordering is by example. Consider a `float` array with dimensions X = 4, Y = 3, and Z = 2. Such an array could be created and populated with the same values in the correct ordering and layout in the following ways:

(See also https://www.visitusers.org/index.php?title=C_vs_Fortran_memory_order )

C/C++
```C
float var[2][3][4];
int x, y, z;
float v = 0.0;
for(z = 0; z < 2; z++)
    for(y = 0; y < 3; y++)
        for(x = 0; x < 4; x++)
            var[z][y][x] = v += 0.01;
```
<!--
float var_flat[24];
memcpy(var_flat, var, 24*sizeof(float));
printf("%f %f %f %f %f\n", var[0][0][0], var[0][0][1], var[0][0][2], var[0][1][0], var[1][0][0] );
printf("%f %f %f %f %f\n", var_flat[0],  var_flat[1],  var_flat[2],  var_flat[4],  var_flat[12] );
-->

Fortran:
<!--
real, dimension(0:1,0:2,0:3) :: var
integer:: x,y,z
real:: v = 0.0
zloop: do z = 0, 1
   yloop: do y = 0, 2
      xloop: do x = 0, 3
        v = v + 0.01
        var(z,y,x) = v ! Note the indices ordering here
      end do xloop
   end do yloop  
end do zloop
-->
```Fortran
real, dimension(0:3,0:2,0:1) :: var ! Note the reversal of the dimension sizes
integer:: x,y,z
real:: v = 0.0
zloop: do z = 0, 1
    yloop: do y = 0, 2
        xloop: do x = 0, 3
            v = v + 0.01
            var(x,y,z) = v ! Note the indices ordering here
        end do xloop
    end do yloop  
end do zloop
```
<!--
print *, var(0,0,0), " ", var(1,0,0), " ", var(2,0,0), " ", var(3,0,0), " ", var(0,1,0), " ", var(0,0,1)
-->

Python:
```Python
var = np.zeros((2,3,4))
v = 0.0
for z in range(0,2):
    for y in range(0,3):
        for x in range(0,4):
            v += 0.01
            var[z,y,x] = v
# OR...
var = np.arange(0.01, 0.25, 0.01)
var = var.reshape((2,3,4))
```

These all will produce contiguous arrays with the following contents and layouts: 

Represented as X, Y, and Z:

| index       | 0, *, 0 | 1, *, 0 | 2, *, 0 | 3, *, 0 |
| ------------|---------|---------|---------|---------|
| ***, 0, 0** |    0.01 |    0.02 |    0.03 |    0.04 |
| ***, 1, 0** |    0.05 |    0.06 |    0.07 |    0.08 |
| ***, 2, 0** |    0.09 |    0.10 |    0.11 |    0.12 |

| index       | 0, *, 1 | 1, *, 1 | 2, *, 1 | 3, *, 1 |
| ------------|---------|---------|---------|---------|
| ***, 0, 1** |    0.13 |    0.14 |    0.15 |    0.16 |
| ***, 1, 1** |    0.17 |    0.18 |    0.19 |    0.20 |
| ***, 2, 1** |    0.21 |    0.22 |    0.23 |    0.24 |

Flattened, or as in the contiguous memory block:

| index     |    0 |    1 |    2 | ... |   21 |   22 |   23 |
|-----------|------|------|------|-----|------|------|------|
| **value** | 0.01 | 0.02 | 0.03 | ... | 0.22 | 0.23 | 0.24 |


Notably, the BMI [`get_grid_shape`](https://bmi.readthedocs.io/en/stable/#get-grid-shape) result for this structure should be:
```
2, 3, 4
```
Which is consistent with the C and NumPy array shapes.

### When this matters

#### Fortran models using z,y,x index ordering

If you have a Fortran model and are using `(y,x)` or `(z,y,x)` ordering for your array indices, then the flattened, linear contiguous memory representation of your array is not the same as C ordering. In this case, the BMI interface layer in your module will have to copy/reorganize the memory in the array to be in C order whenever responding to BMI functions--essentially, recopy `(z,y,x)` arrays to be in `(x,y,z)` order. This is necessary when exposing an array outside of your model module because other models and ngen will assume that the `get_value_at_index` and `set_value_at_index` operations work a certain way on the memory representation and that the array is in a specific layout when it represents spatial data. See below for details on both of these scenarios.

#### Clarification for `get_value_at_indices` and `set_value_at_indices`

The BMI documentation for [`get_value_at_indices`]() and [`set_value_at_indices`]() states that the `inds` parameter designates:

> ...the locations specified by the one-dimensional array indices in the `inds` argument...Both `dest` and `inds` are flattened arrays....

However if read perfectly literally, `inds` is by necessity already a one-dimensional array and therefore can't be flattened--rather, the implication is that the indexes in `inds` are *indexes into a flattened representation of the target variable array*. That is, in the example above, an `inds` array containing `[2, 21]` should return `[0.03, 0.22]`. However, if the flattening is assumed to be done differently by different parts of the system, the wrong values may be retrieved or set.

The ngen BMI driver assumes that the documentation implies that `inds` is a zero-based index into a flattened array and that the array is flattened according to C memory ordering as demonstrated above.

#### Spatial data arrays for structured grids

The BMI documentation for [`get_grid_shape`](https://bmi.readthedocs.io/en/stable/#get-grid-shape) specifies:

> Note that this function (as well as the other grid functions) returns information ordered with “ij” indexing (as opposed to “xy”). For example, consider a two-dimensional rectilinear grid with four columns (nx = 4) and three rows (ny = 3). The get_grid_shape function would return a shape of [ny, nx], or [3,4]. If there were a third dimension, the length of the z-dimension, nz, would be listed first.

This matches the C and NumPy examples above, with a shape of `(2,3,4)`, and clarifies that the last shape ordinate is the X dimension.

Importantly, besides needing to pass multidimentional array data (spatial or otherwise) between BMI modules in a known flattened form, ngen will perform some spatial operations--such as grid data aggregation or re-gridding--on array data from modules if it represents spatial data. Since all BMI arrays are flattened, the flattening order must be consistent, or ngen will perform the spatial operations incorrectly.

# Grid metadata functions

TODO

## Required metadata functions

## Ranks and sizes for scalar values

# Specific ngen uses of unpublished variables

TODO

## The `model_params` list

TODO

## Grid sizing parameters

TODO: Thinking we should add `ngen_` to the front of these, BTW...

### `ngen_grid_N_shape`

### `ngen_grid_N_spacing`

### `ngen_grid_N_origin`



