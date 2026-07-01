# Testing ngen and t-route on a given hydrofabric

Create and activate a virtual environment
```sh
python -m venv venv
source venv/bin/activate
```

Update pip and install required modules
```sh
pip install -U pip
pip install -r requirements.txt
```

## Make sure t-route modules are installed in this venv

If you have already built/installed t-route in another environment, you can reuse the packages and install them in this test environment

```sh
pip install t-route/src/ngen_routing/
pip install t-route/src/nwm_routing/
pip install t-route/src/python_framework_v02/
pip install t-route/src/python_routing_v02/
```
Also, install a couple t-route dependencies that are needed
```sh
pip install deprecated
pip install -r t-route/requirements.txt
pip install tables
```
### Important note on tables install
You may need to ensure tables is linked against the correct hdf5 library, you can do that by setting the environemnt variable `HDF5_DIR`
```sh
HDF5_DIR=~/homebrew/opt/hdf5 pip install tables
```
or
```sh
export HDF5_DIR=~/homebrew/opt/hdf5
pip install tables
```

# Configuring the test

Configure the test to run.  The defaults in `example_config.yaml` should work for must test cases.
The setting you will need to provide inputs for are

- `ngen` : the binary to test.  Can be a complete path or just the binary name if it is already in your $PATH
- `hydrofabric`:  The hydrofabric to test.  This can be a single hydrofabric geopkg, either locally on your system or an `s3` path.
                  This can also be a bucket or a local directory containing multiple geopkg files, in which case all `*.gpkg` in the bucket/directory
                  will be tested.
- `formulation`:
    - `config`: The path to a bmi configuation file for the formulation being tested.
    - `library`: The path to the bmi library for the formulation being tested.  To use the default CFE formulation, provide a path to `libbmicfe.<so|dylib>`

Everything else can be left as is in the example config and ngen and t-route will be run on.  This is NOT intenteded to produce any kind of usable result from the model runs!  Success is defined solely on the ability of the systems to run to completion without errors using the provided hydrofabric.

# Running the test

To run the tests, pass the coniguration to `test_ngen.py`

```sh
python test_ngen.py example_config.yaml
```
or
```sh
 ./test_ngen.py example_config.yaml
```