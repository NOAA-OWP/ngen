from pathlib import Path

import numpy as np
np.set_printoptions(precision=2)
# This is the BMI model that we will be running
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
    grid_data = np.ones((5,5))
    grid_origin = (0,0)
    grid_spacing = (1,1)
    num_grids = np.array(0, dtype=np.int16)

    model.get_value("grid:count", num_grids)
    grids = np.zeros((num_grids), dtype=np.int32)
    model.get_value("grid:ids", grids)
    ranks = np.zeros((num_grids), dtype=np.int16)
    ranks = model.get_value("grid:ranks", ranks)
    
    variables = model.get_input_var_names() # + model.get_output_var_names()
    #Example of determining grid mapping/compatibility.  Mininmum requirement -- rank/dimensions are the same
    for var in variables:
        grid = model.get_var_grid(var)
        idx = np.flatnonzero(grids == grid)[0]
        if ranks[idx] == grid_data.ndim and ranks[idx] != 0:
            print(f"Initiating {var} with grid_data shape, origin, and spacing")
            #FIXME this can init more than once...
            model.set_value(f'grid_{grid}_shape', grid_data.shape)
            model.set_value(f'grid_{grid}_origin', grid_origin)
            model.set_value(f'grid_{grid}_spacing', grid_spacing)

    #Verify some extra grid meta data is available...
    xs = np.array(())
    model.get_grid_x(0, xs)
    print("Grid 0 x coordinates: ", xs)
    ys = np.ndarray( grid_data.shape[1] )
    model.get_grid_y(1, ys) 
    print("Grid 1 y coordinates: ", ys)
    try:
        model.get_grid_z(1, ys)
        print("Any z's???", ys)
    except ValueError:
        print("No z's")

    for x in range(10):

        # Create test case inputs from random values ###########
        model.set_value('INPUT_VAR_1', np.random.uniform(2, 10, model.var_array_lengths))  ##
        model.set_value('INPUT_VAR_2', np.random.uniform(1, 4, model.var_array_lengths))   ##
        ########################################################
        model.set_value('GRID_VAR_1', grid_data*x)
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
        print("Grid input\n", query_bmi_var(model, "GRID_VAR_1"))
        print("Grid output\n", query_bmi_var(model, "GRID_VAR_2"))

    # Finalizing the BMI
    print('Finalizing the BMI')
    model.finalize()


if __name__ == '__main__':
    execute()
