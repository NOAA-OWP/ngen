from datetime import datetime
import pandas as pd
import numpy as np

class usgs_lake_champlain_model():

    def run(self, model: dict, future_time: float):
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

        current_time = pd.Timestamp(model['start_timestamp']) + pd.TimedeltaIndex(np.array([future_time],dtype=float),'s')[0]

        fdate = datetime.strptime(current_time.strftime("%Y-%m-%d %H:%M:%S"), "%Y-%m-%d %H:%M:%S")

        edates = model['Obs_datetimes']

        # Find time index for BMI timestamp to extract waterlevels
        time_diff = np.abs([date - fdate for date in edates])

        index = time_diff.argmin(0)

        print(f"Forecast date is {fdate}")
    
        #Update ETA2 water level boundary fields for SCHISM
        model['ETA2_bnd'] = np.array([model['Obs_Data'][index]],dtype=float)
        print('model output')
        print(model['ETA2_bnd'])
