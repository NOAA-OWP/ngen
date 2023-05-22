# Need these for BMI
# This is needed for get_var_bytes
from pathlib import Path
from os.path import join, isfile
from os import listdir
# import data_tools
# Basic utilities
import numpy as np
import pandas as pd
# Configuration file functionality
import yaml
from bmipy import Bmi

from bmi_grid import Grid, GridType

# Here is the model we want to run
from AORC_model import ngen_AORC_model
import wget
import netCDF4 as nc
import os
import geopandas as gpd
import ssl

class UnknownBMIVariable(RuntimeError):
    pass

class AORC_bmi_model(Bmi):

    def __init__(self):
        """Create a model that is ready for initialization."""
        super(AORC_bmi_model, self).__init__()
        self._values = {}
        self._var_loc = "node"      # JG Edit
        self._var_grid_id = 0       # JG Edit
        self._start_time = 0.0
        #this line causes ngen model_end_time to be a negative number so it throw an exception
        self._end_time = np.finfo(float).max
        print("np.finfo(float).max = ", np.finfo(float).max)
        self._end_time = 1.0e+15
        print("self._end time = ", self._end_time)
        self._model = None
        self.var_array_lengths = 1

        # Initalize AORC dataset as a 2D rectilinear
        # gridded product
        self.grid_1: Grid = Grid(0, 2, GridType.rectilinear) #Grid 1 is a 2 dimensional grid
        print("In AORC_bmi_model: self.grid_1 = ",  self.grid_1)

    #----------------------------------------------
    # Required, static attributes of the model
    #----------------------------------------------
    _att_map = {
        'model_name':         'AORC_Gridded_Forcings_BMI',
        'version':            '1.0',
        'author_name':        'Jason Ducker',
        'grid_type':          'rectilinear',
        'time_units':         'seconds',
               }

    #---------------------------------------------
    # Input variable names (CSDMS standard names)
    #---------------------------------------------
    #will use these implicitly for grid meta data, could in theory advertise them???
    #grid_1_shape, grid_1_size, grid_1_origin
    _input_var_names = []
    _input_var_types = {}

    #---------------------------------------------
    # Output variable names (CSDMS standard names)
    #---------------------------------------------
    #will use these implicitly for grid meta data, could in theory advertise them?
    # grid_1_rank -- could probably establish a pattern grid_{id}_rank for managing different ones?
    #_output_var_names = ['ids', 'APCP_surface', 'TMP_2maboveground', 'SPFH_2maboveground', 'UGRD_10maboveground', 'VGRD_10maboveground', 'PRES_surface', 'DSWRF_surface', 'DLWRF_surface']
    _output_var_names = ['APCP_surface', 'TMP_2maboveground', 'SPFH_2maboveground', 'UGRD_10maboveground', 'VGRD_10maboveground', 'PRES_surface', 'DSWRF_surface', 'DLWRF_surface']

    #------------------------------------------------------
    # Create a Python dictionary that maps CSDMS Standard
    # Names to the model's internal variable names.
    # This is going to get long, 
    #     since the input variable names could come from any forcing...
    #------------------------------------------------------
    #_var_name_map_long_first = {
    #_var_name_units_map = {'ids':['ids','-'],
    _var_name_units_map = {'APCP_surface':['APCP_surface','kg/m^2'],
                           'TMP_2maboveground':['TMP_2maboveground','K'],
                           'SPFH_2maboveground':['SPFH_2maboveground','kg/kg'],
                           'UGRD_10maboveground':['UGRD_10maboveground','m/s'],
                           'VGRD_10maboveground':['VGRD_10maboveground','m/s'],
                           'PRES_surface':['PRES_surface','Pa'],
                           'DSWRF_surface':['DSWRF_surface','W/m^2'],
                           'DLWRF_surface':['DLWRF_surface','W/m^2']
                            }
    #------------------------------------------------------
    # A list of static attributes/parameters.
    #------------------------------------------------------
    _model_parameters_list = []

    #------------------------------------------------------------
    #------------------------------------------------------------
    # BMI: Model Control Functions
    #------------------------------------------------------------ 
    #------------------------------------------------------------

    #-------------------------------------------------------------------
    def initialize( self, bmi_cfg_file_name: str ):
        # TODO: should this be set in some way via config?
        #self.var_array_lengths = 1

        # -------------- Read in the BMI configuration -------------------------#
        if not isinstance(bmi_cfg_file_name, str) or len(bmi_cfg_file_name) == 0:
            raise RuntimeError("No BMI initialize configuration provided, nothing to do...")

        bmi_cfg_file = Path(bmi_cfg_file_name).resolve()
        if not bmi_cfg_file.is_file():
            raise RuntimeError("No configuration provided, nothing to do...")

        with bmi_cfg_file.open('r') as fp:
            cfg = yaml.safe_load(fp)
        self.cfg_bmi = self._parse_config(cfg)

        # ----- Create some lookup tabels from the long variable names --------#
        self._var_name_map_long_first = {long_name:self._var_name_units_map[long_name][0] for long_name in self._var_name_units_map.keys()}
        self._var_name_map_short_first = {self._var_name_units_map[long_name][0]:long_name for long_name in self._var_name_units_map.keys()}
        self._var_units_map = {long_name:self._var_name_units_map[long_name][1] for long_name in self._var_name_units_map.keys()}


        #-------------- Initalize AORC grid specifications for NextGen -----#
        # First, load and read AORC config file configurations
        #with bmi_cfg_file.open('r') as fp:
        #    cfg = yaml.safe_load(fp)
        #self.cfg_bmi = self._parse_config(cfg)

        # Set attributes that relate to standard
        # AORC url links for grabbing data from ERRDAP server
        self._aorc_beg = self.cfg_bmi['aorc_beg']
        self._aorc_end = self.cfg_bmi['aorc_end']
        self._aorc_new_end = self.cfg_bmi['aorc_new_end']
        self._base_url = self.cfg_bmi['base_url']

        # Set AORC data pathway variable in config file
        # which may specify where AORC data is located
        # or is just a blank string
        self._AORC_data_pathway = self.cfg_bmi['AORC_data']

        # Set flag for config file specification on
        # whether user specifies AORC data or requests
        # ERRDAP data download of AORC data
        if(self.cfg_bmi['ERRDAP_data'].lower() in ['true','t', 'y', 'yes']):
            self._ERRDAP_data = True
            self._AORC_files = []
        else:
            self._ERRDAP_data = False
            self._AORC_files = [f for f in listdir(self._AORC_data_pathway) if isfile(join(self._AORC_data_pathway, f))]

        #-------------- Specify a dummy start time for AORC Forcings BMI -------#
        self._date = pd.Timestamp(self.cfg_bmi['start_time'])

        # If user is requesting data from ERRDAP server
        # then pull AORC netcdf start time file
        # to assign netcdf metadta to BMI
        if(self._ERRDAP_data):
            # Flag for change in AORC file extension format
            if(self._date.year > 2019):
                # Get standard AORC file from ERRDAP server
                url_link = self._base_url + self._date.strftime('%Y') + self._date.strftime('%m')  + '/' + self._aorc_beg + self._date.strftime('%Y') + self._date.strftime('%m') + self._date.strftime('%d') + self._date.strftime('%H') + self._aorc_new_end
                # Get AORC file name
                aorc_file = self._aorc_beg + self._date.strftime('%Y') + self._date.strftime('%m') + self._date.strftime('%d') + self._date.strftime('%H') + self._aorc_new_end
            else:
                # Get standard AORC file from ERRDAP server
                url_link = self._base_url + self._date.strftime('%Y') + self._date.strftime('%m')  + '/' + self._aorc_beg + self._date.strftime('%Y') + self._date.strftime('%m') + self._date.strftime('%d') + self._date.strftime('%H') + self._aorc_end
                # Get AORC file name
                aorc_file = self._aorc_beg + self._date.strftime('%Y') + self._date.strftime('%m') + self._date.strftime('%d') + self._date.strftime('%H') + self._aorc_end

            # Initialize ssl default context to connect to url server when downloading files
            ssl._create_default_https_context = ssl._create_unverified_context

            # Download AORC netcdf file from ERRDAP server using wget module
            filename = wget.download(url_link,bar=None)
            # Open AORC netcdf file to get metadata for BMI
            nc_file = nc.Dataset(aorc_file)

        else:
            # Open AORC netcdf file to get metadata for BMI
            nc_file = nc.Dataset(join(self._AORC_data_pathway,self._AORC_files[0]))

        # Get variable list from AORC file
        nc_vars = list(nc_file.variables.keys())
        # Get indices corresponding to Meteorological data
        indices = [nc_vars.index(i) for i in nc_vars if '_' in i]
        # Make array with variable names to use for ExactExtract module
        self._AORC_met_vars = np.array(nc_vars)[indices]

        # Assing fixed grid origin for AORC forcing data
        self.grid_1._origin = (-130.004166499999997,55.002766499999993)

        # Assign fixed pixel size for AORC forcing data
        self.grid_1._spacing = (0.008332999999993,0.008333000000000)

        # Get variable grid dimensions
        self.grid_1._shape = np.array(nc_file.variables['PRES_surface'][:][0,:,:].shape,np.int32)
        print("In AORC_bmi_model: self.grid_1._shape = ", self.grid_1._shape)
   
        # Get grid coordinates for AORC Forcings BMI functions
        self.grid_1._grid_x = nc_file.variables['longitude'][:]
        self.grid_1._grid_y = nc_file.variables['latitude'][:]

        # get scale_factor and offset keys if available
        # and append missing values to BMI class
        # (AORC-OWP files for HUC01 scenario has this metadata)
        self._add_offset = np.zeros([len(self._AORC_met_vars)])
        self._scale_factor = np.zeros([len(self._AORC_met_vars)])
        self._missing_value = np.zeros([len(self._AORC_met_vars)])
        i = 0
        for key in self._AORC_met_vars:
            try:
                self._scale_factor[i] = nc_file.variables[key].scale_factor
            except AttributeError as e:
                self._scale_factor[i] = 1.0
            try:
                self._add_offset[i] = nc_file.variables[key].add_offset
            except AttributeError as e:
                self._add_offset[i] = 0.0

            self._missing_value[i] = nc_file.variables[key].missing_value

            i += 1

        # Close netcdf file
        nc_file.close()

        if(self._ERRDAP_data):
            # remove netcdf file from directory
            os.remove(aorc_file)

        # ------------- Initialize the AORC grid class and set class variables ----------#

        self._grids = [self.grid_1]

        #TODO this can be done more elegantly using a more coherent data structure representing a BMI variable
        self._grid_map = {'APCP_surface': self.grid_1, 'TMP_2maboveground': self.grid_1, 'SPFH_2maboveground': self.grid_1, 'PRES_surface': self.grid_1,
                          'UGRD_10maboveground': self.grid_1, 'VGRD_10maboveground': self.grid_1, 'DSWRF_surface': self.grid_1, 'DLWRF_surface': self.grid_1}

        # Need to reproject the hydrofabric crs to the meteorological forcing
        # dataset crs for ExactExtract to properly regrid the data
        self._hyfabfile = join(os.getcwd(),"hyfabfile_final.json")
        hyfab_data = gpd.read_file(self.cfg_bmi['hyfabfile'],layer='divides')
        hyfab_data = hyfab_data.to_crs('WGS84')
        hyfab_data.to_file(self._hyfabfile,driver="GeoJSON")

        # Open the hydrofabric file to get the number of catchments in NextGen to initalize arrays
        cat_df_full = gpd.read_file(self._hyfabfile)
        h = [i for i in cat_df_full.id]
        #self.var_array_lengths = len(h)
        g = [i for i in cat_df_full.divide_id]
        #print("Number of Catchments in the hydrofabric file = ", len(g))

        # ------------- Set catchment id -----------------------#
        self._values['cat_id'] = self.cfg_bmi['cat_id']
        self.var_array_lengths = len(np.atleast_1d(self._values['cat_id']))
        #print("self.var_array_lengths = ", self.var_array_lengths)

        # Find the index number for the specified cat-id in the catchment array
        array = np.array(g)
        if np.all(array != None):
            arr_index = np.where(array == self._values['cat_id'])
            arr_idx = (arr_index[0])[0]
            self.arr_index = arr_idx
        #print("self.arr_index = ", self.arr_index)
       
        # ------------- Initialize the parameters, inputs and outputs ----------#
        for parm in self._model_parameters_list:
            self._values[self._var_name_map_short_first[parm]] = self.cfg_bmi[parm]
        for model_input in self.get_input_var_names():
            #print("model_input names: ", model_input)
            # cat-id still needed for now in ngen framework
            #if model_input == "ids":
            #    self._values[model_input] = np.empty(self.var_array_lengths, dtype=object)
            #else:
            #This won't actually allocate the size, just the rank...
            if self._grid_map[model_input].type == GridType.scalar:
                self._values[model_input] = np.zeros( (), dtype=float)
            else: 
                self._values[model_input] = np.zeros(self._grid_map[model_input].shape, dtype=float)        
        #for model_input in self._input_var_types:
        #    self._values[model_input] = np.zeros(self.var_array_lengths, dtype=self._input_var_types[model_input])
        for model_output in self.get_output_var_names():
            # cat-id still needed for now in ngen framework
            #print("model_output names: ", model_output)
            #if model_output == "ids":
            #    self._values[model_output] = np.empty(self.var_array_lengths, dtype=object)
            #else:
            if self._grid_map[model_output].type == GridType.scalar:
                self._values[model_output] = np.zeros( (), dtype=float)
            else: 
                self._values[model_output] = np.zeros(self._grid_map[model_output].shape, dtype=float)
        #print(self._values)

        # ------------- Set time to initial value -----------------------#
        self._values['current_model_time'] = self.cfg_bmi['initial_time']
        
        # ------------- Set time step size -----------------------#
        self._values['time_step_size'] = self.cfg_bmi['time_step_seconds']

        """
        # ------------- Set catchment id -----------------------#
        self._values['cat_id'] = self.cfg_bmi['cat_id']

        # Find the index number for the specified cat-id in the catchment array
        array = np.array(g)
        if np.all(array != None):
            arr_index = np.where(array == self._values['cat_id'])
            arr_idx = (arr_index[0])[0]
            self.arr_index = arr_idx
        print("self.arr_index = ", self.arr_index)
        """

        # ------------- Initialize a model ------------------------------#
        #self._model = ngen_model(self._values.keys())
        self._model = ngen_AORC_model()

    #------------------------------------------------------------ 
    def update(self):
        """
        Update/advance the model by one time step.
        """
        model_time_after_next_time_step = self._values['current_model_time'] + self._values['time_step_size']
        self.update_until(model_time_after_next_time_step)
    
    #------------------------------------------------------------ 
    def update_until(self, future_time: float):
        """
        Update the model to a particular time

        Parameters
        ----------
        future_time : float
            The future time to when the model should be advanced.
        """
        update_delta_t = future_time - self._values['current_model_time']
        self._model.run(self._values, update_delta_t, self._date, self._base_url, self._aorc_beg, self._aorc_end, self._aorc_new_end,  self._ERRDAP_data, self._AORC_data_pathway, self._AORC_files, self._AORC_met_vars, self._scale_factor, self._add_offset, self._missing_value)
    #------------------------------------------------------------    
    def finalize( self ):
        """Finalize model."""
        self._model = None

    #-------------------------------------------------------------------
    #-------------------------------------------------------------------
    # BMI: Model Information Functions
    #-------------------------------------------------------------------
    #-------------------------------------------------------------------
    
    def get_attribute(self, att_name):
    
        try:
            return self._att_map[ att_name.lower() ]
        except:
            print(' ERROR: Could not find attribute: ' + att_name)

    #--------------------------------------------------------
    # Note: These are currently variables needed from other
    #       components vs. those read from files or GUI.
    #--------------------------------------------------------   
    def get_input_var_names(self):

        return self._input_var_names

    def get_output_var_names(self):
 
        return self._output_var_names

    #------------------------------------------------------------ 
    def get_component_name(self):
        """Name of the component."""
        return self.get_attribute( 'model_name' ) #JG Edit

    #------------------------------------------------------------ 
    def get_input_item_count(self):
        """Get names of input variables."""
        return len(self._input_var_names)

    #------------------------------------------------------------ 
    def get_output_item_count(self):
        """Get names of output variables."""
        return len(self._output_var_names)

    #------------------------------------------------------------ 
    def get_value(self, var_name: str, dest: np.ndarray) -> np.ndarray:
        """Copy of values.
        Parameters
        ----------
        var_name : str
            Name of variable as CSDMS Standard Name.
        dest : ndarray
            A numpy array into which to place the values.
        Returns
        -------
        array_like
            Copy of values.
        """
        if var_name == "grid:count":
            dest[...] = 2
        elif var_name == "grid:ids":
            dest[:] = [self.grid_0.id, self.grid_1.id]
        elif var_name == "grid:ranks":
            dest[:] = [self.grid_0.rank, self.grid_1.rank]
        else:
            dest[:] = self.get_value_ptr(var_name)
            #print("In AORC_bmi_model, dest = ", dest[:])
            #if var_name == "ids":
            #    print("dest = ", dest[:])
        return dest

    #-------------------------------------------------------------------
    def get_value_ptr(self, var_name: str) -> np.ndarray:
        """Reference to values.
        Parameters
        ----------
        var_name : str
            Name of variable as CSDMS Standard Name.
        Returns
        -------
        array_like
            Value array.
        """

        #Make sure to return a flattened array
        if(var_name == "grid_1_shape"): # FIXME cannot expose shape as ptr, because it has to side affect variable construction...
            return self.grid_1.shape
        if(var_name == "grid_1_spacing"):
            return self.grid_1.spacing
        if(var_name == "grid_1_origin"):
            return self.grid_1.origin
        if(var_name == "grid_1_units"):
            return self.grid_1.units
        #grid 2 meta
        if(var_name == "grid_2_shape"): # FIXME cannot expose shape as ptr, because it has to side affect variable construction...
            return self.grid_2.shape
        if(var_name == "grid_2_spacing"):
            return self.grid_2.spacing
        if(var_name == "grid_2_origin"):
            return self.grid_2.origin
        if(var_name == "grid_2_units"):
            return self.grid_2.units

        if var_name not in self._values.keys():
            raise(UnknownBMIVariable(f"No known variable in BMI model: {var_name}"))

        #if var_name != 'ids':
        shape = self._values[var_name].shape
        try:
            #see if raveling is possible without a copy
            self._values[var_name].shape = (-1,)
            #reset original shape
            self._values[var_name].shape = shape
        except ValueError as e:
            raise RuntimeError("Cannot flatten array without copying -- "+str(e).split(": ")[-1])

        """
        if var_name == "ids":
            self._values['ids'] = self._values['cat_id']
            #print("self._values['ids']: ", self._values['ids'])
            return np.atleast_1d(self._values[var_name])
        else:
            #print("var_name, self._values[var_name])[self.arr_index]: ", var_name, (self._values[var_name])[self.arr_index])
            #return np.atleast_1d((self._values[var_name])[self.arr_index])

            #if var_name == "APCP_surface":
            #    self._values[var_name] = np.nan_to_num(self._values[var_name], copy=False, nan=2.5, posinf=None, neginf=None)
            #else:
            #    self._values[var_name] = np.nan_to_num(self._values[var_name], copy=False, nan=10.5, posinf=None, neginf=None)
            #return self._values[var_name].ravel()#.reshape((-1,))

            #testing good values on the grids
            #if var_name == "PRES_surface":
            #    self._values[var_name] = self._values[var_name][~np.isnan(self._values[var_name])]
            #    print("self._values[var_name] = ", var_name,  self._values[var_name])
            return self._values[var_name].ravel()#.reshape((-1,))
        """

        return self._values[var_name].ravel()#.reshape((-1,))

    #-------------------------------------------------------------------
    #-------------------------------------------------------------------
    # BMI: Variable Information Functions
    #-------------------------------------------------------------------
    #-------------------------------------------------------------------
    def get_var_name(self, long_var_name):
                              
        return self._var_name_map_long_first[ long_var_name ]

    #-------------------------------------------------------------------
    def get_var_units(self, long_var_name):

        return self._var_units_map[ long_var_name ]
                                                             
    #-------------------------------------------------------------------
    def get_var_type(self, var_name: str) -> str:
        """Data type of variable.

        Parameters
        ----------
        var_name : str
            Name of variable as CSDMS Standard Name.

        Returns
        -------
        str
            Data type.
        """
        #if var_name != "ids":
        return str(self.get_value_ptr(var_name).dtype)
    
    #------------------------------------------------------------ 
    def get_var_grid(self, name):
        
        # all vars have grid 0 but check if its in names list first
        if name in (self._output_var_names + self._input_var_names):
            return self._var_grid_id
        else:
            raise(UnknownBMIVariable(f"No known variable in BMI model: {name}"))

    #------------------------------------------------------------ 
    def get_var_itemsize(self, name):
        #if name != "ids":
        return self.get_value_ptr(name).itemsize

    #------------------------------------------------------------ 
    def get_var_location(self, name):
        #FIXME what about grid vars?
        if name in (self._output_var_names + self._input_var_names):
            return self._var_loc

    #-------------------------------------------------------------------
    def get_var_rank(self, long_var_name):
        return np.int16(2) 

    #-------------------------------------------------------------------
    def get_start_time( self ):
    
        return self._start_time 

    #-------------------------------------------------------------------
    def get_end_time( self ) -> float:

        return self._end_time 


    #-------------------------------------------------------------------
    def get_current_time( self ):

        return self._values['current_model_time']

    #-------------------------------------------------------------------
    def get_time_step( self ):

        return self._values['time_step_size']

    #-------------------------------------------------------------------
    def get_time_units( self ):

        return self.get_attribute( 'time_units' ) 
       
    #-------------------------------------------------------------------
    def set_value(self, var_name, values: np.ndarray):
        """
        Set model values for the provided BMI variable.

        Parameters
        ----------
        var_name : str
            Name of model variable for which to set values.
        values : np.ndarray
              Array of new values.
        """ 
        if( var_name == 'grid_1_shape' ):
            self.grid_1.shape = values
            for var, grid in self._grid_map.items():
                if grid.id == 1:
                    #shape is set externally, need to reshape/allocate all vars associated with the grid
                    self._values[var] = np.resize(self._values[var], self._grid_map[var].shape)
        elif( var_name == 'grid_1_spacing' ):
            self.grid_1.spacing = values
        elif( var_name == 'grid_1_origin' ):
            self.grid_1.origin = values
        elif( var_name == 'grid_1_units' ):
            self.grid_1.units = values
        #grid 2 meta
        elif( var_name == 'grid_2_shape' ):
            self.grid_2.shape = values
            for var, grid in self._grid_map.items():
                if grid.id == 2:
                    #shape is set externally, need to reshape/allocate all vars associated with the grid
                    self._values[var] = np.resize(self._values[var], self._grid_map[var].shape)
        elif( var_name == 'grid_2_spacing' ):
            self.grid_2.spacing = values
        elif( var_name == 'grid_2_origin' ):
            self.grid_2.origin = values
        elif( var_name == 'grid_2_units' ):
            self.grid_2.units = values
    
        else:
            #values is a FLATTENED array, need to reshape it...
            self._values[var_name][...] = np.ndarray( self._values[var_name].shape, self._values[var_name].dtype, values )

    #------------------------------------------------------------ 
    def set_value_at_indices(self, var_name: str, indices: np.ndarray, src: np.ndarray):
        """
        Set model values for the provided BMI variable at particular indices.

        Parameters
        ----------
        var_name : str
            Name of model variable for which to set values.
        indices : array_like
            Array of indices of the variable into which analogous provided values should be set.
        src : array_like
            Array of new values.
        """
        # This is not particularly efficient, but it is functionally correct.
        #if var_name != "ids":
        for i in range(indices.shape[0]):
            bmi_var_value_index = indices[i]
            self.get_value_ptr(var_name)[bmi_var_value_index] = src[i]

    #------------------------------------------------------------ 
    def get_var_nbytes(self, var_name) -> int:
        """
        Get the number of bytes required for a variable.
        Parameters
        ----------
        var_name : str
            Name of variable.
        Returns
        -------
        int
            Size of data array in bytes.
        """
        #if var_name != "ids":
        return self.get_value_ptr(var_name).nbytes

    #------------------------------------------------------------ 
    def get_value_at_indices(self, var_name: str, dest: np.ndarray, indices: np.ndarray) -> np.ndarray:
        """Get values at particular indices.
        Parameters
        ----------
        var_name : str
            Name of variable as CSDMS Standard Name.
        dest : np.ndarray
            A numpy array into which to place the values.
        indices : np.ndarray
            Array of indices.
        Returns
        -------
        np.ndarray
            Values at indices.
        """
        #if var_name != "ids":
        original: np.ndarray = self.get_value_ptr(var_name)
        for i in range(indices.shape[0]):
            value_index = indices[i]
            dest[i] = original[value_index]
        return dest

    # JG Note: remaining grid funcs do not apply for type 'scalar'
    #   Yet all functions in the BMI must be implemented 
    #   See https://bmi.readthedocs.io/en/latest/bmi.best_practices.html          
    #------------------------------------------------------------ 
    def get_grid_edge_count(self, grid):
        raise NotImplementedError("get_grid_edge_count")

    #------------------------------------------------------------ 
    def get_grid_edge_nodes(self, grid, edge_nodes):
        raise NotImplementedError("get_grid_edge_nodes")

    #------------------------------------------------------------ 
    def get_grid_face_count(self, grid):
        raise NotImplementedError("get_grid_face_count")
    
    #------------------------------------------------------------ 
    def get_grid_face_edges(self, grid, face_edges):
        raise NotImplementedError("get_grid_face_edges")

    #------------------------------------------------------------ 
    def get_grid_face_nodes(self, grid, face_nodes):
        raise NotImplementedError("get_grid_face_nodes")
    
    #------------------------------------------------------------ /get_value
    def get_grid_node_count(self, grid):
        raise NotImplementedError("get_grid_node_count")

    #------------------------------------------------------------ 
    def get_grid_nodes_per_face(self, grid, nodes_per_face):
        raise NotImplementedError("get_grid_nodes_per_face") 
    
    #------------------------------------------------------------ 
    def get_grid_origin(self, grid_id, origin):

        for grid in self._grids:
            if grid_id == grid.id: 
                origin[:] = grid.origin
                return
        raise ValueError(f"get_grid_origin: grid_id {grid_id} unknown")


    #------------------------------------------------------------ 
    def get_grid_rank(self, grid_id):
 
        for grid in self._grids:
            if grid_id == grid.id: 
                return grid.rank
        raise ValueError(f"get_grid_rank: grid_id {grid_id} unknown")

    #------------------------------------------------------------ 
    def get_grid_shape(self, grid_id, shape):

        for grid in self._grids:
            if grid_id == grid.id:
                shape[:] = grid.shape
                return
        raise ValueError(f"get_grid_shape: grid_id {grid_id} unknown")

    #------------------------------------------------------------ 
    def get_grid_size(self, grid_id):
       
        for grid in self._grids:
            if grid_id == grid.id: 
                return grid.size
        raise ValueError(f"get_grid_size: grid_id {grid_id} unknown")

    #------------------------------------------------------------ 
    def get_grid_spacing(self, grid_id, spacing):

        for grid in self._grids:
            if grid_id == grid.id: 
                spacing[:] = grid.spacing
                return
        raise ValueError(f"get_grid_spacing: grid_id {grid_id} unknown") 

    #------------------------------------------------------------ 
    def get_grid_type(self, grid_id):

        for grid in self._grids:
            if grid_id == grid.id: 
                return grid.type
        raise ValueError(f"get_grid_type: grid_id {grid_id} unknown")

    #------------------------------------------------------------ 
    def get_grid_x(self, grid_id: int, dest: np.ndarray):
        for grid in self._grids:
            if grid_id == grid.id: 
                dest[:] = grid.grid_x
                return
        raise ValueError(f"get_grid_x: grid_id {grid_id} unknown")

    #------------------------------------------------------------ 
    def get_grid_y(self, grid_id: int, dest: np.ndarray):
        for grid in self._grids:
            if grid_id == grid.id: 
                dest[:] = grid.grid_y
                return
        raise ValueError(f"get_grid_y: grid_id {grid_id} unknown")

    #------------------------------------------------------------ 
    def get_grid_z(self, grid_id: int, dest: np.ndarray):
        for grid in self._grids:
            if grid_id == grid.id: 
                dest[:] = grid.grid_z
                return
        raise ValueError(f"get_grid_z: grid_id {grid_id} unknown")


    #------------------------------------------------------------ 
    #------------------------------------------------------------ 
    #-- Random utility functions
    #------------------------------------------------------------ 
    #------------------------------------------------------------ 

    def _parse_config(self, cfg):
        for key, val in cfg.items():
            # convert all path strings to PosixPath objects
            if any([key.endswith(x) for x in ['_dir', '_path', '_file', '_files']]):
                if (val is not None) and (val != "None"):
                    if isinstance(val, list):
                        temp_list = []
                        for element in val:
                            temp_list.append(Path(element))
                        cfg[key] = temp_list
                    else:
                        cfg[key] = Path(val)
                else:
                    cfg[key] = None

            # convert Dates to pandas Datetime indexs
            elif key.endswith('_date'):
                if isinstance(val, list):
                    temp_list = []
                    for elem in val:
                        temp_list.append(pd.to_datetime(elem, format='%d/%m/%Y'))
                    cfg[key] = temp_list
                else:
                    cfg[key] = pd.to_datetime(val, format='%d/%m/%Y')

            else:
                pass

        # Add more config parsing if necessary
        return cfg
