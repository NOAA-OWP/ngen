from pathlib import Path

import numpy as np
np.set_printoptions(precision=2)
# This is the BMI LSTM that we will be running
from bmi_model import bmi_model

from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from bmipy import Bmi

def query_bmi_var(model: 'Bmi', name: str) -> np.ndarray:
    """_summary_

    Args:
        model (Bmi): the Bmi model to query
        name (str): the name of the variable to query

    Returns:
        ndarray: numpy array with the value of the variable marshalled through BMI
    """
    #TODO most (if not all) of this can be cached...unless the grid can change?
    rank = model.get_var_rank(name)
    grid = model.get_var_grid(name)
    shape = np.zeros(rank, dtype=int)
    model.get_grid_shape(grid, shape)
    #TODO call model.get_var_type(name) and determine the correct type of nd array to create
    result = model.get_value(name, np.zeros(shape))
    return result

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
    print('IN 1:{}, IN 2:{}, OUT 1:{}, OUT 2{}, OUT 3{}'.format(model.get_current_time(),
                                                                model.get_value('INPUT_VAR_1', np.zeros(0)),
                                                                model.get_value('INPUT_VAR_2',np.zeros(0)),
                                                                model.get_value('OUTPUT_VAR_1',np.zeros(0)),
                                                                model.get_value('OUTPUT_VAR_2',np.zeros(0)),
                                                                model.get_value('OUTPUT_VAR_3',np.zeros(3))))

    # Finalizing the BMI
    print('Finalizing the BMI')
    model.finalize()


if __name__ == '__main__':
    execute()
