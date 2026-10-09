from pathlib import Path

import numpy as np
np.set_printoptions(precision=20)
# This is the BMI model that we will be running
from bmi_model import bmi_model
import time

from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from bmipy import Bmi

def execute():
    # creating an instance of a model
    print('creating an instance of an BMI_MODEL model object')
    model = bmi_model()

    # Initializing the BMI
    print('Initializing the BMI')
    current_dir = Path(__file__).parent.resolve()
    model.initialize(bmi_cfg_file_name=str(current_dir.joinpath('config.yml')))

    ETA2_BND = np.zeros(model.grid_0._size,dtype=float)
    # Now loop through the inputs, set the forcing values, and update the model
    print('Now loop through the inputs, set the values, and update the model')
    print('\n')
    print('model time', 'ETA2_bnd')
    start = time.time()
    for x in range(24):

        # Create test case inputs from random values ###########
        #model.set_value('INPUT_VAR_1', np.random.uniform(2, 10, model.var_array_lengths))  ##
        #model.set_value('INPUT_VAR_2', np.random.uniform(1, 4, model.var_array_lengths))   ##
        
        #########################################
        # UPDATE THE MODEL WITH THE NEW INPUTS ##
        model.update()     ######################
        #########################################

        # Get value for ETA2 BND and print data
        ETA2_BND = model.get_value('ETA2_bnd',ETA2_BND)

        # PRINT THE MODEL RESULTS FOR THIS TIME STEP#################################################
        print('model time','ETA2_bnd')
        print(model.get_current_time(), ETA2_BND)

    print('BMI module time to loop through NWM Medium range (120 hours) operational configuration')
    print(time.time() - start)

    # Finalizing the BMI
    print('Finalizing the BMI')
    model.finalize()



if __name__ == '__main__':
    execute()
