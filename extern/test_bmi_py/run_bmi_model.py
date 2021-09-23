
import numpy as np
from pathlib import Path
from netCDF4 import Dataset
# This is the BMI LSTM that we will be running
import bmi_model

# creating an instance of a model
print('creating an instance of an BMI_MODEL model object')
model = bmi_model.bmi_model()

# Initializing the BMI
print('Initializing the BMI')
model.initialize(bmi_cfg_file=Path('./config.yml'))

# Now loop through the inputs, set the forcing values, and update the model
print('Now loop through the inputs, set the values, and update the model')
for x in range(100):

    model.set_value('model_input',x)

    model.update()

    print('model_input, and model_output at time {} are {:.2f}, {:.2f}'.format(model.get_current_time(), 
                                model.get_value('model_input'), model.get_value('model_output')))

    if model.t > 10:
        print('stopping the loop')
        break

# Finalizing the BMI
print('Finalizing the BMI')
model.finalize()