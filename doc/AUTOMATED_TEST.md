# Github Automated Testing
- [What Are Automated Tests and How to Start Them](#what-are-automated-tests-and-how-to-start-them)
- [What Code Tests Are Performed](#what-code-tests-are-performed)
- [Local Testing](#local-testing)
- [What to Do When a Test Fail](#what-to-do-when-a-test-fail)

## What Are Automated Tests and How to Start Them

In the **ngen** github repo, we use github actions/workflows (for an online reference, see for [example](https://docs.github.com/en/actions/learn-github-actions)) to automatically test validity of committed codes by developers. The test is triggered when a developer pushes some code to a branch in their **ngen** fork and creates a `Pull Request`. The successful test is marked by a `green check` mark, a failed test is marked by a `red cross` mark. If a test fails, you have to debug your code (see [What to Do When a Test Fail](#what-to-do-when-a-test-fail) below) and commit/push to the same branch again. The automatic testing will restart in the `Pull Request`.

## What Code Tests Are Performed

- Unit tests: this includes every set of codes that serves a unique functionality. Unit test evolve as new codes are added.
- [BMI](https://bmi.readthedocs.io/en/stable/) (Basic Model Interface) based formulation tests for C, C++, Fortran, and Python.
- Running **ngen** executable on example hydrofabric with various realistic modules/models, initial condition, and forcing data.

## Local Testing

We strongly recommend you run all the tests on your local computer before you push up the new codes to the branch in your fork. To be able to run the tests, when building the executables (for `build` with `cmake` see [here](https://github.com/stcui007/ngen/blob/ngen_automated_test/doc/BUILDS_AND_CMAKE.md)), you need to set the test option to `ON` using `cmake` option:

    -DNGEN_WITH_TESTS:BOOL=ON

After `build` completes, assuming your build directory is `cmake_build`, you can check that the `ngen` executable is in `cmake_build` directory, and all `unit test` executables are in `cmake_build/test` directory. To run a unit test, for example, run the following command while in the top project directory:

    ./cmake_build/test/test_unit

There are many other unit test executables you need to run for a complete test.
For example, to run an **ngen** test job using the data set in the `data` directory, use the following command:

    ./cmake_build/ngen data/catchment_data.geojson '' data/nexus_data.geojson '' data/example_bmi_multi_realization_config.json

To run a multi-processors job with MPI, please see a complete description in [here](https://github.com/stcui007/ngen/blob/ngen_automated_test/doc/DISTRIBUTED_PROCESSING.md)

## What to Do When a Test Fail

- Before getting into debugging, first thing, we recommend that you perform all the necessary tests listed above on you local computer. If all tests are successful, then push up your codes to the intended branch in your fork.

- Sometimes, some tests may fail even they have passed tests on local computer. If that happens, you have to look into details why they failed. To do that, click on the word `Details` in blue on the right for a particular test. This will open a window with detailed information for that particular test, the error information are usually near the bottom. You can scroll up and down the side bar for more information. you can also search by key workds in `Search logs` menu entry at the upper right corner.

- Otherwise, if you are lucky (or unlucky depending on your perspective), the test may have failed due to time out or some unknown reasons, in those cases, you may rerun the test by placing the cursor on the test name, a cycling icon will appear and you can rerun your test by clicking on the icon. In any case, you may manually rerun any failed test by following procedure described above. But it is strongly recommended that you carefully examine the fail error first.
