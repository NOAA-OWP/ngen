# Testing BMI Python modules.
 - model.py: This file is the "model" it takes inputs and gives an output
 - bmi_model.py: This is the Basic Model Interface that talks with the model. This is what we are testing.
 - run_bmi_model.py: This is a file that mimics the framework, in the sense that it initializes the model with the BMI function. Then it runs the model with the BMI Update function, etc.
 - run_bmi_unit_test.py: This is a file that runs each BMI unit test to make sure that the BMI is complete and functioning as expected.
 - config.yml: This is a configuration file that the BMI reads to set inital_time (initial value of current_model_time) and time_step_seconds (time_step_size).
 - environment.yml: Environment file with the required Python libraries needed to run the model with BMI. Create the environment with this command: `conda env create -f environment.yml`, then activate it with `conda activate bmi_test`

# About
This is an implementation of a Python-based model that fulfills the Python language BMI interface and can be used in the Framework. It is intended to serve as a control for testing purposes, freeing the framework from dependency on any real-world model in order to test BMI related functionality.

# Implementation Details

## Test the complete BMI functionality
`python run_bmi_unit_test.py`

## Run the model
`python run_bmi_model.py`

## Sample output
model time, input_1, input_2, output_1, output_2, output_3  
3600.00, 8.07, 3.19, 8.07, 6.38, 0.00  
10800.00, 6.35, 2.16, 12.70, 8.65, 0.00  
25200.00, 3.85, 3.18, 15.40, 25.48, 0.00  
54000.00, 7.37, 1.40, 58.98, 22.39, 0.00  
111600.00, 7.08, 3.57, 113.22, 114.10, 0.00  
226800.00, 2.54, 2.79, 81.40, 178.63, 0.00  
457200.00, 3.57, 1.31, 228.50, 167.46, 0.00  
918000.00, 5.18, 1.77, 662.80, 453.24, 0.00  
1839600.00, 3.68, 3.75, 943.04, 1919.17, 0.00  
3682800.00, 8.62, 1.63, 4412.11, 1668.66, 0.00  