from pathlib import Path

import numpy as np

# This is the BMI LSTM that we will be running
from AORC_bmi_model import AORC_bmi_model


def execute():
    # creating an instance of a model
    print('creating an instance of the  AORC_BMI_MODEL model object')
    model = AORC_bmi_model()

    # Initializing the BMI
    print('Initializing the AORC Forcings BMI')
    current_dir = Path(__file__).parent.resolve()
    model.initialize(bmi_cfg_file_name=str(current_dir.joinpath('config.yml')))

    # Now loop through the time steps, update the AORC Forcings model, and set output values
    print('Now loop through the time steps, run the AORC forcings model (update), and set output values')
    print("\n")
    #print('model time', 'ids', 'RAINRATE', 'T2D', 'Q2D', 'U2D', 'V2D', 'PSFC', 'SWDOWN', 'LWDOWN')

    #print out the state variables in the order as in AORC csv forcing file
    print("{},{},{},{},{},{},{},{},{}".format("time","APCP_surface","DLWRF_surface","DSWRF_surface","PRES_surface","SPFH_2maboveground","TMP_2maboveground","UGRD_10maboveground","VGRD_10maboveground"))

    num_cats = len(model.get_value_ptr('ids'))
    print("Number of catchments = ", num_cats)

    for x in range(6):

        #########################################
        # UPDATE THE MODEL WITH THE NEW INPUTS ##
        model.update()     ######################
        #########################################

        # PRINT THE MODEL RESULTS FOR THIS TIME STEP#################################################
        #print('{:.2f}, {s}, {:.2f}, {:.2f}, {:.2f}, {:.2f}, {:.2f}, {:.2f}, {:.2f}, {:.2f}'.format(model.get_current_time(),
        #                                                         model.get_value_ptr('ids'),
        #                                                         model.get_value_ptr('RAINRATE'),
        #                                                         model.get_value_ptr('T2D'),
        #                                                         model.get_value_ptr('Q2D'),
        #                                                         model.get_value_ptr('U2D'),
        #                                                         model.get_value_ptr('V2D'),
        #                                                         model.get_value_ptr('PSFC'),
        #                                                         model.get_value_ptr('SWDOWN'),
        #                                                         model.get_value_ptr('LWDOWN')))

        for i in range(num_cats):
            print("{},{},{},{},{},{},{},{},{},{}".format(model.get_current_time(),
                                                  model.get_value_ptr('ids')[i],
                                                  model.get_value_ptr('APCP_surface')[i],
                                                  model.get_value_ptr('DLWRF_surface')[i],
                                                  model.get_value_ptr('DSWRF_surface')[i],
                                                  model.get_value_ptr('PRES_surface')[i],
                                                  model.get_value_ptr('SPFH_2maboveground')[i],
                                                  model.get_value_ptr('TMP_2maboveground')[i],
                                                  model.get_value_ptr('UGRD_10maboveground')[i],
                                                  model.get_value_ptr('VGRD_10maboveground')[i]))

    # Finalizing the BMI
    print('Finalizing the AORC Forcings BMI')
    model.finalize()


if __name__ == '__main__':
    execute()
