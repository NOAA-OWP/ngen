# δHBV2.0 Submodule

## About

This directory wraps the *dhbv2* git submodule repo, containing a clone of the repo for the MHPI δHBV2.0 module library implementing BMI. From here, the shared library file for the dhbv2 module can be built for use in NGen.

#### Extra Outer Directory

Currently there are two directory layers beneath the top-level *extern/* directory.  This was done so that certain things used by NGen (i.e., a *CMakeLists.txt* file for building shared library files, this readme) can be placed alongside, but not within, the submodule.

## Working with the Submodule

Some simple explanations of several command actions are included below.  To better understand what these things are doing, consult the [Git Submodule documentation](https://git-scm.com/book/en/v2/Git-Tools-Submodules).

### Getting the Latest Changes

There are two steps to getting upstream submodule changes fully

  1. fetching and locally checking out the changes from the remote
  2. committing the new checkout revision for the submodule

To fetch and checkout the latest revision (for the [currently used branch](#viewing-the-current-branch)):

    ```bash
    git submodule add git@github.com:leoglonz/dhbv2_mts.git extern/dhbv2/dhbv2
    git submodule update --init --recursive
    ```

To commit the current submodule checkout revision to the CIROH UA NGen repo:

    ```bash
    git add extern/dhbv2/dhbv2
    git commit
    ```

### Viewing the Commit Hash

Git submodule configurations include the specific commit to be checked out (or an implicit default).  The current commit can be view with `git submodule status`:

    ```bash
    git submodule status -- extern/dhbv2/dhbv2/
    ```

This will show the **commit**, **submodule local path**, and the git description for the **commit**.  The specific configuration, including the configured branch, is set in the _.gitmodules_ file in the NGen project root.

### Changing the Commit Branch

The latest commit in the configured branch can be brought in as described here.  If it is ever necessary to change to a different branch, the following will do so:

    ```bash
    git config -f .gitmodules "submodule.extern/dhbv2/dhbv2.branch" <branchName>
    ```

Note that this will be done in the NGen repo configuration, so it can then be committed and push to remotes.  It is also possible to do something similar in just the local clone of a repo, by configuring `.git/config` instead of `.gitmodules`.  See the Git documentation for more on how that works if needed.

# Usage

## Building Libraries

Since dhbv2 is a Python submodule, dependencies located at `extern/dhbv2/dhbv2/pyproject.toml` will be installed automatically when ngen is installed. Notably, dhbv2 depends on two separate github modules to function:

- [`dmg`](https://github.com/mhpi/generic_deltamodel): Scafolds and constructs a differentiable model (neural network + physical model), and exposes this model structure to the dhbv2 BMI for use by NGen.
- [`hydrodl2`](https://github.com/mhpi/hydrodl2/tree/master): Contains the physical model HBV2.0 (`/hydrodl2/models/hbv/hbv_2.py`).

## Loading Model Weights

dhbv2 uses an LSTM which requires loading trained weights. Since these are too large to store with the module, they must be downloaded and installed from AWS S3 as follows. First, navigate to the submodule.

    ```bash
    cd extern/dhbv2/dhbv2
    ```

If AWS CLI is not installed on your system (check `aws --version`) see [AWS instructions](https://docs.aws.amazon.com/cli/latest/userguide/getting-started-install.html) for system-specific directions.

Then download model weights (and coupled normalization statistics file) from S3 like

    ```bash
    aws s3 cp s3://mhpi-spatial/mhpi-release/models/owp/dhbv_2_mts.zip ./temp/ --no-sign-request
    unzip ./temp/dhbv_2_mts.zip -d ./temp
    mv ./temp/dhbv_2_mts/. ./ngen_resources/data/dhbv_2_mts/models/dhbv_2_mts
    rm -r ./temp
    ```
