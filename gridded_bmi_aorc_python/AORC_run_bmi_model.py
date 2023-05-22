from pathlib import Path

import numpy as np

np.set_printoptions(precision=2)
# This is the BMI model that we will be running
from AORC_bmi_model import AORC_bmi_model


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
    if name == "APCP_surface":
        print("shape = ", shape)
        print("grid = ", grid)
    #TODO call model.get_var_type(name) and determine the correct type of nd array to create
    result = model.get_value(name, np.zeros(model.get_grid_size(0),float))
    return result

def execute():
    # creating an instance of a model
    print('creating an instance of an BMI_MODEL model object')
    model = AORC_bmi_model()

    # Initializing the BMI
    print('Initializing the BMI')
    current_dir = Path(__file__).parent.resolve()
    model.initialize(bmi_cfg_file_name=str(current_dir.joinpath('config.yml')))

    # Now loop through the inputs, set the forcing values, and update the model
    print('Now loop through the inputs, set the values, and update the model')
    print('\n')
    print('model time', 'APCP_surface', 'TMP_2maboveground', 'SPFH_2maboveground', 'UGRD_10maboveground', 'VGRD_10maboveground', 'PRES_surface', 'DSWRF_surface', 'DLWRF_surface')
 
    for x in range(10):

        #########################################
        # UPDATE THE MODEL WITH THE NEW INPUTS ##
        model.update()     ######################
        #########################################

        # Query model update arrays after model ran to extract AORC forcing data
        APCP_surface = query_bmi_var(model, "APCP_surface")
        TMP_2maboveground = query_bmi_var(model, "TMP_2maboveground")
        SPFH_2maboveground = query_bmi_var(model, "SPFH_2maboveground")
        UGRD_10maboveground = query_bmi_var(model, "UGRD_10maboveground")
        VGRD_10maboveground = query_bmi_var(model, "VGRD_10maboveground")
        PRES_surface = query_bmi_var(model, "PRES_surface")
        DSWRF_surface = query_bmi_var(model, "DSWRF_surface")
        DLWRF_surface = query_bmi_var(model, "DLWRF_surface")


        # PRINT THE MODEL RESULTS FOR THIS TIME STEP#################################################
        print(model.get_current_time(), np.nanmax(APCP_surface), np.nanmax(TMP_2maboveground), np.nanmax(SPFH_2maboveground), np.nanmax(UGRD_10maboveground), np.nanmax(VGRD_10maboveground), np.nanmax(PRES_surface), np.nanmax(DSWRF_surface), np.nanmax(DLWRF_surface))
    
    # Finalizing the BMI
    print('Finalizing the BMI')
    model.finalize()


if __name__ == '__main__':
    execute()
