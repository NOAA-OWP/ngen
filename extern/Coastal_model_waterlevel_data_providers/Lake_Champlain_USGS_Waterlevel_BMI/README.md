# Testing BMI Python modules.
 - model.py: This file is the "model" it takes inputs and gives an output
 - bmi_model.py: This is the Basic Model Interface that talks with the model. This is what we are testing.
 - run_bmi_model.py: This is a file that mimics the framework, in the sense that it initializes the model with the BMI function. Then it runs the model with the BMI Update function, etc.
 - run_bmi_unit_test.py: This is a file that runs each BMI unit test to make sure that the BMI is complete and functioning as expected.
 - config.yml: This is a configuration file that the BMI reads to set inital_time (initial value of current_model_time) and time_step_seconds (time_step_size).
 - environment.yml: Environment file with the required Python libraries needed to run the model with BMI. Create the environment with this command: `conda env create -f environment.yml`, then activate it with `conda activate bmi_test`

# About
This is an implementation of a Python-based model that fulfills the Python language BMI interface and can be used in the Framework. This Python BMI interface servers as a USGS water level observation station data provider to force NextGen coastal models (SCHISM, DFlowFM) with a single offshore water level boundary upstream of the Lake Champlain domain.

# Implementation Details
The USGS data provider BMI here serves to provide USGS station water level data to coastal models all the up until 2023-01-01 00:00:00. Any time beyond the last observation time of the USGS station data will serve as a constant value over future forecast times for a given coastal model application for the Lake Champlain domain. We will be updating the USGS observation csv dataset in the near future with close to realtime values.
## Test the complete BMI functionality
`python run_bmi_unit_test.py`

## Run the model
`python run_bmi_model.py`

## Sample output
model time, ETA2_bnd
3600.00, 29.84
7200.00, 30.45
10800.00, 30.89
