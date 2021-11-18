from pathlib import Path

import numpy as np

# This is the BMI LSTM that we will be running
from . import bmi_model


def execute():
    # creating an instance of a model
    print('creating an instance of an BMI_MODEL model object')
    model = bmi_model()

    # Initializing the BMI
    print('Initializing the BMI')
    current_dir = Path(__file__).parent.resolve()
    model.initialize(bmi_cfg_file_name=str(current_dir.joinpath('config.yml')))

    # Now loop through the inputs, set the forcing values, and update the model
    print('Now loop through the inputs, set the values, and update the model')
    print('\n')
    print('model time', 'input 1', 'input 2', 'output 1', 'output 2', 'output 3')
    for x in range(10):

        # Create test case inputs from random values ###########
        model.set_value('INPUT_VAR_1', np.random.uniform(2, 10, model.var_array_lengths))  ##
        model.set_value('INPUT_VAR_2', np.random.uniform(1, 4, model.var_array_lengths))   ##
        ########################################################

        #########################################
        # UPDATE THE MODEL WITH THE NEW INPUTS ##
        model.update()     ######################
        #########################################

        # PRINT THE MODEL RESULTS FOR THIS TIME STEP#################################################
        print('{:.2f}, {:.2f}, {:.2f}, {:.2f}, {:.2f}, {:.2f}'.format(model.get_current_time(),
                                                                 model.get_value('INPUT_VAR_1'),
                                                                 model.get_value('INPUT_VAR_2'),
                                                                 model.get_value('OUTPUT_VAR_1'),
                                                                 model.get_value('OUTPUT_VAR_2'),
                                                                 model.get_value('OUTPUT_VAR_3')))


    # Finalizing the BMI
    print('Finalizing the BMI')
    model.finalize()


if __name__ == '__main__':
    execute()
