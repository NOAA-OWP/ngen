
import numpy as np
from pathlib import Path
# This is the BMI LSTM that we will be running
import bmi_model
import random

# creating an instance of a model
print('creating an instance of an BMI_MODEL model object')
model = bmi_model.bmi_model()

# Initializing the BMI
print('Initializing the BMI')
model.initialize(bmi_cfg_file=Path('./config.yml'))

# Now loop through the inputs, set the forcing values, and update the model
print('Now loop through the inputs, set the values, and update the model')
print('\n')
print('model time', 'input 1', 'input 2', 'output 1', 'output 2', 'output 3')
for x in range(10):

    # Create test case inputs from random values ###########
    model.set_value('input_var_1',random.uniform(2, 10))  ##
    model.set_value('input_var_2',random.uniform(1, 4))   ##
    ########################################################

    #########################################
    # UPDATE THE MODEL WITH THE NEW INPUTS ##
    model.update()     ######################
    #########################################

    # PRINT THE MODEL RESULTS FOR THIS TIME STEP#################################################
    print('{:.2f}, {:.2f}, {:.2f}, {:.2f}, {:.2f}, {:.2f}'.format(model.get_current_time(), 
                                                             model.get_value('input_var_1'), 
                                                             model.get_value('input_var_2'),
                                                             model.get_value('output_var_1'), 
                                                             model.get_value('output_var_2'), 
                                                             model.get_value('output_var_3')))


# Finalizing the BMI
print('Finalizing the BMI')
model.finalize()