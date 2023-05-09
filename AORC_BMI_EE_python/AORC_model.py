# load python C++ binds from ExactExtract module library
from exactextract import GDALDatasetWrapper, GDALRasterWrapper, Operation, MapWriter, FeatureSequentialProcessor, GDALWriter
# must import gdal to properly read and partiton rasters
# from AORC netcdf files
from osgeo import gdal
import pandas as pd
import numpy as np
import os
import wget

class ngen_AORC_model():
    # TODO: refactor the bmi_model.py file and this to have this type maintain its own state.
    #def __init__(self):
    #    super(ngen_model, self).__init__()
    #    #self._model = model

    def run(self, model: dict, dt: int, date, base_url, aorc_beg, aorc_end, aorc_new_end, AORC_met_vars, scale_factor, add_offset, hyfabfile):
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
     
        # Download the AORC forcing file for current timestamp using wget 
        filename = wget.download(AORC_url_link,bar=None)

        # load AORC netcdf file into gdal dataframe to
        # partition out meterological variables into rasters
        AORC_ncfile = gdal.Open(AORC_file)

        # Get gdal sub-datasets, which will seperate each AORC
        # variable into their own raster wrapper
        nc_rasters = AORC_ncfile.GetSubDatasets()

        # Define gdal writer to only return ExactExtract
        # regrid results as a python dict
        writer = MapWriter()
        # Define operation and raster dictionaries for each AORC met variable
        op_dict = {}
        rsw_dict = {}
        # loop over each meteorological variable and call
        # ExactExtract to regrid raster to lumped sum for
        # a given NextGen catchment
        for i in np.arange(len(AORC_met_vars)):
            # Get variable name in netcdf file
            variable = nc_rasters[i][0].split(":")[-1]
            # Get the gdal netcdf syntax for netcdf variable
            # Example syntax: 'NETCDF:"AORC-OWP_2012050100z.nc4":APCP_surface'
            nc_dataset_name = nc_rasters[i][0]
            # Define raster wrapper for AORC meteorological variable
            # and specify nc_file attribute to be True. Otherwise,
            # this function will expect a .tif file. Assign data for dict variable
            rsw_dict["rsw{0}".format(i)] = GDALRasterWrapper(nc_dataset_name,nc_file=True)
            # Define operation to use for raster and assign data for dict variable
            op_dict["op{0}".format(i)] = Operation.from_descriptor('mean('+variable+')', raster=rsw_dict["rsw{0}".format(i)])
        # For each AORC met variable, we must redefine the
        # hydrofabric raster dataset to regrid forcings
        # based on user operation below
        dsw = GDALDatasetWrapper(hyfabfile)
        # Process the data and write results to writer instance
        processor = FeatureSequentialProcessor(dsw, writer, list(op_dict.values()))
        processor.process()
        # convert dict results to pandas dataframe
        AORC_df = pd.DataFrame(writer.output.values(),columns = AORC_met_vars)
        # Loop through AORC met variables and acount for their scale factor
        # and offset (if any) within the netcdf metadata
        met_loop = 0
        for column in AORC_df:
            AORC_df[column] = AORC_df[column]*scale_factor[met_loop] + add_offset[met_loop]
            met_loop += 1

        # Assign catchment ids of regridded data from writer output keys
        AORC_df['ids'] = writer.output.keys()

        # Flush changes to disk
        writer = None

        # Now remove AORC netcdf file since its no longer needed
        os.remove(AORC_file)
      
        # Send regridded AORC data back to model output
        for var in AORC_df.columns:
            model[var] = AORC_df[var].values
           
        # Update the model time for next NextGen model iteration
        model['current_model_time'] = model['current_model_time'] + dt
        
        #return model
