# Testing BMI Python modules.
 - bmi_grid.py: This file is the module for supporting BMI grid meta data and functionality
 - AORC_model.py: This file is the "model" it takes inputs and gives an output
 - AORC_bmi_model.py: This is the Basic Model Interface that talks with the model. This is what we are testing.
 - AORC_run_bmi_model.py: This is a file that mimics the framework, in the sense that it initializes the model with the BMI function. Then it runs the model with the BMI Update function, etc.
 - AORC_run_bmi_unit_test.py: This is a file that runs each BMI unit test to make sure that the BMI is complete and functioning as expected.
 - config.yml: This is a configuration file that the BMI reads to set inital_time (initial value of current_model_time) and time_step_seconds (time_step_size).
 - environment.yml: Environment file with the required Python libraries needed to run the model with BMI. Create the environment with this command: `conda env create -f environment.yml`, then activate it with `conda activate bmi_test`

# About
This is an implementation of a Python-based model that fulfills the Python language BMI interface and can be used in the Framework. It not only serves as a control for testing purposes, but can also run ngen models directly within the ngen framework.

# Implementation Details

## Test the complete BMI functionality
`python AORC_run_bmi_unit_test.py`

## Run the model
`python AORC_run_bmi_model.py`

## Sample output
model time APCP_surface TMP_2maboveground SPFH_2maboveground UGRD_10maboveground VGRD_10maboveground PRES_surface DSWRF_surface DLWRF_surface  
3600 17.80000114440918 301.1000061035156 0.019700000062584877 12.5 10.699999809265137 104130.0 128.60000610351562 438.20001220703125  
7200 11.199999809265137 300.0 0.018799999728798866 12.90000057220459 10.800000190734863 104130.0 0.0 438.20001220703125  
10800 8.800000190734863 299.5 0.017899999395012856 13.300000190734863 11.199999809265137 104140.0 0.0 438.20001220703125  
14400 11.5 299.3999938964844 0.017899999395012856 13.800000190734863 11.600000381469727 104140.0 0.0 441.20001220703125  
18000 14.699999809265137 299.3999938964844 0.01769999973475933 14.199999809265137 11.5 104220.0 0.0 441.20001220703125  
21600 15.5 299.3999938964844 0.017799999564886093 14.600000381469727 11.5 104280.0 0.0 441.20001220703125  
25200 13.800000190734863 299.3999938964844 0.01809999905526638 15.100000381469727 11.5 104350.0 0.0 440.8000183105469  
28800 11.699999809265137 299.3999938964844 0.017899999395012856 15.300000190734863 11.199999809265137 104310.0 0.0 440.8000183105469  
32400 25.399999618530273 299.3999938964844 0.017799999564886093 15.5 11.0 104280.0 0.0 440.8000183105469  
36000 17.80000114440918 299.3999938964844 0.017899999395012856 15.800000190734863 11.100000381469727 104230.0 0.0 442.6000061035156

## Run the ngen model
`../cmake_serial_py/ngen ./catchment_data.geojson "cat-298118" ./nexus_data.geojson "nex-298119" ../data/example_realization_config_jinja_aorc.json`
Several additional input files are needed to run the above command. We have provided some example files for a test run, in addition to files used in "Testing BMI Python modules" step. These include forcing files in data/forcing/ and initialization files in data/bmi/ subdirectories.
