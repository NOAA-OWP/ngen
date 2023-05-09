import netCDF4 as nc
import pandas as pd
import numpy as np
import os
from os.path import join
import wget

class ngen_AORC_model():
    # TODO: refactor the bmi_model.py file and this to have this type maintain its own state.
    #def __init__(self):
    #    super(ngen_model, self).__init__()
    #    #self._model = model

    def run(self, model: dict, dt: int, date, base_url, aorc_beg, aorc_end, aorc_new_end, ERRDAP_data, AORC_data_pathway, AORC_files, AORC_met_vars, scale_factor, add_offset, missing_value, _grid_x, _grid_y):
        """
        Run this model into the future.

        Run this model into the future, updating the state stored in the provided model dict appropriately.

        Note that the model assumes the current values set for input variables are appropriately for the time
        duration of this update (i.e., ``dt``) and do not need to be interpolated any here.

        Parameters
        ----------
        model: dict
            The model state data structure.
        dt: int
            The number of seconds into the future to advance the model.

        Returns
        -------
    
        """

        # Get current datetime stamp
        current_time = date + pd.TimedeltaIndex(np.array([model['current_model_time']],dtype=float),'s')

        # Create AORC url link and filename from ERRDAP server based on current timestamp
        # and flag for file extenstion difference based on year of timestamp
        if(current_time.year[0] > 2019):
            AORC_url_link = base_url + current_time.strftime("%Y%m")[0] +"/" + aorc_beg + current_time.strftime("%Y%m%d%H")[0] + aorc_new_end
            AORC_file = aorc_beg + current_time.strftime("%Y%m%d%H")[0] + aorc_new_end
        else:
            AORC_url_link = base_url + current_time.strftime("%Y%m")[0] +"/" + aorc_beg + current_time.strftime("%Y%m%d%H")[0] + aorc_end
            AORC_file = aorc_beg + current_time.strftime("%Y%m%d%H")[0] + aorc_end
     
        # Boolean flag to indicate method for either
        # grabbing AORC data file off the ERRDAP server
        # or grabbing the current time AORC file off the
        # user specified directory
        if(ERRDAP_data):
            # Download the AORC forcing file for current timestamp using wget 
            filename = wget.download(AORC_url_link,bar=None)
            # load AORC netcdf file
            AORC_ncfile = nc.Dataset(AORC_file)
        else:
            # Search and find current time for AORC hourly file
            AORC_current_time_file = [s for s in AORC_files if AORC_file in s][0]
            # load AORC netcdf file
            AORC_ncfile = nc.Dataset(join(AORC_data_pathway,AORC_current_time_file))

        # Send AORC gridded data back to model output
        for i, var in enumerate(AORC_met_vars):           
            #fill in the missing values with 0.0 or np.nan
            #TODO eliminate the if block all together
            if(var == 'APCP_surface'):
                model[var] = np.where(AORC_ncfile.variables[var][:].data == missing_value[i], 0.0, AORC_ncfile.variables[var][:].data)
            else:
                model[var] = np.where(AORC_ncfile.variables[var][:].data == missing_value[i], np.nan, AORC_ncfile.variables[var][:].data)
                #change first two elements from np,nan to real values to show the model[var] are set up correctly
                model[var][0][0][0] = -0.12345
                model[var][0][0][1] = -0.12345

                #swap between model[var][0][0][0] and model[var][0][_grid_y][_grid_x] so than ngen framework run on real values
                tmp_var = model[var][0][0][0]
                #model[var][0][0][0] = model[var][0][2101][4201]
                #model[var][0][2101][4201] = tmp_var
                model[var][0][0][0] = model[var][0][_grid_y][_grid_x]
                model[var][0][_grid_y][_grid_x] = tmp_var

                num_nan = np.count_nonzero(~np.isnan(model[var]))
                #print("In AORC_model: num_nan = ", num_nan)

        # Update the model time for next NextGen model iteration
        model['current_model_time'] = model['current_model_time'] + dt

        # remove ERRDAP file download to free memory
        # if option is selected
        if(ERRDAP_data):
            # Now remove AORC netcdf file since its no longer needed
            os.remove(AORC_file)
      
        #return model
