# Testing BMI Python modules.
 - AORC_model.py: This file is the "model" it takes inputs and gives an output
 - AORC_bmi_model.py: This is the Basic Model Interface that talks with the model. This is what we are testing.
 - AORC_run_bmi_model.py: This is a file that mimics the framework, in the sense that it initializes the model with the BMI function. Then it runs the model with the BMI Update function, etc.
 - AORC_run_bmi_unit_test.py: This is a file that runs each BMI unit test to make sure that the BMI is complete and functioning as expected.
 - config.yml: This is a configuration file that the BMI reads to set inital_time (initial value of current_model_time) and time_step_seconds (time_step_size).
 - environment.yml: Environment file with the required Python libraries needed to run the model with BMI. Create the environment with this command: `conda env create -f environment.yml`, then activate it with `conda activate bmi_test`

# About
This is an implementation of a Python-based model that fulfills the Python language BMI interface and can be used in the Framework. It is intended to serve as a control for testing purposes, freeing the framework from dependency on any real-world model in order to test BMI related functionality.

# Implementation Details

## Test the complete BMI functionality
`python AORC_run_bmi_unit_test.py`

## Run the model
`python AORC_run_bmi_model.py`

## Sample output
model time ids RAINRATE T2D Q2D U2D V2D PSFC SWDOWN LWDOWN
3600 cat-298141 0.0 292.8053998709529 0.008956236803775719 -0.9000000134110451 -1.6072029353261428 98204.853515625 0.0 360.34036181480224
7200 cat-298141 0.0 291.4287885223148 0.009017457352766267 -1.3006835177642628 -1.600000023841858 98119.267578125 0.0 360.27102587626905
10800 cat-298141 0.0 290.0400922125591 0.009098991164183112 -1.7708498264975816 -1.600000023841858 98033.759765625 0.0 360.15591356986624
14400 cat-298141 0.0 288.66411562955545 0.009186683422710007 -2.1909006445244046 -1.5513213388818698 97948.603515625 0.0 371.8681207756417
