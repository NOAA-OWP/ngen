# BMI Conventions

The [BMI Documentation](https://bmi.readthedocs.io/en/stable/) enumerates all of the functions in the BMI specification and the [BMI Best Practices](https://bmi.readthedocs.io/en/stable/bmi.best_practices.html#best-practices) page goes into additional detail about how the interface should be used, but in many cases BMI is not prescriptive about behavior, leaving choices up to model and tool developers. For instance, the Best Practices document states,

> All functions in the BMI must be implemented. For example, even if a model operates on a [uniform rectilinear](https://bmi.readthedocs.io/en/stable/model_grids.html#uniform-rectilinear) grid, a [get_grid_x](https://bmi.readthedocs.io/en/stable/index.html#get-grid-x) function has to be written. This function can be empty and simply return the BMI_FAILURE status code or raise a NotImplemented exception, depending on the language.

Strictly speaking, while all functions must have some implementation, *none* of the functions in the interface are specifically called out as required to do anything other than return a failure--though that would not be a very useful model. So, in these and other cases, we must specify if any of the BMI functions are required to behave in a certain way to function properly with the NextGen Water Resources Modeling Framework's Model Engine (hereafter "ngen" the executable name), and these requirements are what this document aims to spell out.

# Compliance with the BMI v2.0 Standard

BMI modules written in C, Fortran, and C++ *must* be compiled with the exact same header files as those used with ngen itself, and this means the BMI v2.0 header files provided in the CSDMS [`bmi-c`](https://github.com/csdms/bmi-c/blob/e6f9f8a0ab326218831000b4a5571490ebc21ea2/bmi.h), [`bmi-fortran`](https://github.com/csdms/bmi-fortran/blob/e34cd026f57cabd009ef0f662fc672baee66e442/bmi.f90), and [`bmi-cxx`](https://github.com/csdms/bmi-cxx/blob/be631dc510b477b3bc3eb3c8bbecf3d04ec4005c/bmi.hxx) repositories. Python models must inherit from the `Bmi` class in the [`bmipy`](https://pypi.org/project/bmipy/2.0/) package. Python and C++ classes can be subclasses of the standard BMI interface, but such subclasses must inherit from the standard versions listed here.

This means among other things that it is not possible to "extend" the BMI interface simply by adding functions to the base interface or header files. In any case, even with subclassed BMI models in Python and C++ with additional functions, ngen would not call any such additional functions because it does not know that they exist. 

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
    * also accepts `numpy.float64` and `np.float64` but this usage is discouraged! Please use the non-namespaced names above.

## Array representation

### Contiguousness

In order to pass arrays back and forth between modules and ngen (and indrectly from BMI modules to other BMI modules), it is necessary that the arrays being passed are stored in contiguous memory blocks in a known layout. This does not necessarily require that this is how data is stored in memory and used for computation within the model, but when passing data through BMI functions array data must conform to this constraint.

### Zero-Indexing

Arrays should be zero-indexed (the index of the first item is 0, not 1 or some other number) and indexes appearing in the BMI functions (e.g. [`get_value_at_indices`](https://bmi.readthedocs.io/en/stable/#get-value-at-indices)) must be treated as zero-based indices. Of the supported languages, this mainly affects Fortran developers, as Fortran uses 1-based indices by default.

### Layout and Flattening

The [BMI best practices](https://bmi.readthedocs.io/en/stable/bmi.best_practices.html) document states:

> BMI functions always use flattened, one-dimensional arrays...It’s the developer’s responsibility to ensure that array information is flattened/redimensionalized in the correct order.

However, strictly speaking, *how* a multi-dimensional array should be flattened into a one-dimensional one is never directly addressed. **For ngen, it is required that flattening happens as if the array was a C array, sometimes referred to as "row major order".** That is, if you create a multi-dimensional array in C (or a C array in C++) it will already be in the appropriate order and layout such that if the data is copied into a 1D array (or the pointer is passed as the result of `get_value_ptr`) it is already in the correct order/layout and will "just work".

However, if you have a multi-dimensional array in Fortran or in Python/NumPy it may *not* be in the correct order. In Fortran, arrays are created in memory in "column major order"--however if you treat the last index as the fastest changing, it is the same thing as C ordering. In Python, NumPy `ndarray`s are created as contiguous C-ordered blocks by default, but it is possible to create Fortran-ordered arrays, and if you take a view or slice of an array it is no longer a contiguous array and can't be passed without copying.

The simplest way to explain the proper ordering is by example. Consider a `float` array with dimensions X = 4, Y = 3, and Z = 2. Such an array could be created and populated with the same values in the correct ordering and layout in the following ways:

***THE BELOW NEEDS A SANITY CHECK. I HAVE BEEN STARING AT THIS STUFF TOO LONG AND MAY HAVE SOMETHING WRONG.***

C/C++
```C
float var[4][3][2];
int x, y, z;
float v = 0.0;
for(z = 0; z < 2; z++) for(y = 0; y < 3; y++) for(x = 0; x < 4; x++)
    var[x][y][z] = v += 0.01;
```

Fortran:
```Fortran
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
```

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

Represented as X, Y and Z:

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


Notably, the BMI [`get_grid_shape`]() result for this structure should be:
```
4, 3, 2
```




