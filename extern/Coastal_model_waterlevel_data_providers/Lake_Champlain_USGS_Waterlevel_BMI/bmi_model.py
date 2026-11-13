# Need these for BMI
# This is needed for get_var_bytes
from pathlib import Path
import os
# import data_tools
# Basic utilities
import numpy as np
import pandas as pd
# Configuration file functionality
import yaml
from bmipy import Bmi
import datetime
from bmi_grid import Grid, GridType
# Here is the model we want to run
from model import usgs_lake_champlain_model

class UnknownBMIVariable(RuntimeError):
    pass

class bmi_model(Bmi):

    def __init__(self):
        """Create a model that is ready for initialization."""
        super(bmi_model, self).__init__()
        self._values = {}
        self._start_time = 0.0
        self._end_time = np.finfo(float).max
        self._model = None
        self.var_array_lengths = 1

    #----------------------------------------------
    # Required, static attributes of the model
    #----------------------------------------------
    _att_map = {
        'model_name':         'Lake Champlain USGS Observation Water level Forcing BMI',
        'version':            '1.0',
        'author_name':        'Jason Ducker',
        'grid_type':          'points',
        'time_units':         'seconds',
               }

    #---------------------------------------------
    # Input variable names (CSDMS standard names)
    #---------------------------------------------
    _input_var_names = []
    _input_var_types = {}

    #---------------------------------------------
    # Output variable names (CSDMS standard names)
    #---------------------------------------------
    _output_var_names = ['ETA2_bnd']

    #------------------------------------------------------
    # Create a Python dictionary that maps CSDMS Standard
    # Names to the model's internal variable names.
    # This is going to get long, 
    #     since the input variable names could come from any forcing...
    #------------------------------------------------------
    #_var_name_map_long_first = {
    _var_name_units_map = {'ETA2_bnd':['ETA2_bnd','m']}

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

        # ----- Create some lookup tabels from the long variable names --------#
        self._var_name_map_long_first = {long_name:self._var_name_units_map[long_name][0] for long_name in self._var_name_units_map.keys()}
        self._var_name_map_short_first = {self._var_name_units_map[long_name][0]:long_name for long_name in self._var_name_units_map.keys()}
        self._var_units_map = {long_name:self._var_name_units_map[long_name][1] for long_name in self._var_name_units_map.keys()}
        
        # -------------- Read in the BMI configuration -------------------------#
        if not isinstance(bmi_cfg_file_name, str) or len(bmi_cfg_file_name) == 0:
            raise RuntimeError("No BMI initialize configuration provided, nothing to do...")

        bmi_cfg_file = Path(bmi_cfg_file_name).resolve()
        if not bmi_cfg_file.is_file():
            raise RuntimeError("No configuration provided, nothing to do...")

        with bmi_cfg_file.open('r') as fp:
            cfg = yaml.safe_load(fp)
        self.cfg_bmi = self._parse_config(cfg)

        self.grid_0: Grid = Grid(0, 1, GridType.points) #Grid 0 is a 2 dimension "grid" for points
        self.grid_0._grid_x = np.array([self.cfg_bmi['Obs_Lon']],dtype=float)
        self.grid_0._grid_y = np.array([self.cfg_bmi['Obs_Lat']],dtype=float)
        self.grid_0._shape = self.grid_0._grid_x.shape
        self.grid_0._size = 1
        self._grids = [self.grid_0]

        # Assign input and output variables to their respective grid class
        self._grid_map = {'ETA2_bnd': self.grid_0}

        # ------------- Initialize the parameters, inputs and outputs ----------#
        for parm in self._model_parameters_list:
            self._values[self._var_name_map_short_first[parm]] = self.cfg_bmi[parm]
        for model_input in self.get_input_var_names():
            #This won't actually allocate the size, just the rank...
            if self._grid_map[model_input].type == GridType.scalar:
                self._values[model_input] = np.zeros( (), dtype=float)
            else: 
                self._values[model_input] = np.zeros(self._grid_map[model_input].shape, dtype=float)        
        #for model_input in self._input_var_types:
        #    self._values[model_input] = np.zeros(self.var_array_lengths, dtype=self._input_var_types[model_input])
        for model_output in self.get_output_var_names():
            #TODO why is output var 3 an arange?  should this be a unique "grid"?
            if self._grid_map[model_output].type == GridType.scalar:
                self._values[model_output] = np.zeros( (), dtype=float)
            else:
                self._values[model_output] = np.zeros(self._grid_map[model_output].shape, dtype=float)

        # ------------- Set time to initial value -----------------------#
        self._values['current_model_time'] = self.cfg_bmi['initial_time']
        
        # ------------- Set time step size -----------------------#
        self._values['time_step_size'] = self.cfg_bmi['time_step_seconds']

        # ------------- Set forecast start time -----------------------#
        self._values['start_timestamp'] = self.cfg_bmi['start_timestamp']

        # ------------- Set SCHISM boundary coordinates -----------------------#

        # Extract observation data and interpolate down to hourly data using
        # pandas dataframe then assign to BMI model class
        obs_df = pd.read_csv(self.cfg_bmi['Obs_Data'])
        obs_df.index = pd.to_datetime(obs_df[obs_df.columns[0]])
        obs_df = obs_df.resample('H').interpolate()

        # Set datetime stamps from USGS observation dataset to the model
        # class for use during data provider model execution
        self._values['Obs_Data'] = obs_df[obs_df.columns[1]].values
        self._values['Obs_datetimes'] = obs_df.index.to_pydatetime()

        # ------------- Initialize a model ------------------------------#
        self._model = usgs_lake_champlain_model()

    #------------------------------------------------------------ 
    def update(self):
        """
        Update/advance the model by one time step.
        """
        self._values['current_model_time'] += self._values['time_step_size']
        self.update_until(self._values['current_model_time'])
    
    #------------------------------------------------------------ 
    def update_until(self, future_time: float):
        """
        Update the model to a particular time

        Parameters
        ----------
        future_time : float
            The future time to when the model should be advanced.
        """
        # Flag to see if update is just a single model time step
        # otherwise we must perform a time loop to iterate data until
        # requested time stamp
        if(future_time != self._values['current_model_time']):
            while(self._values['current_model_time'] < future_time):
                self._values['current_model_time'] += self._values['time_step_size']
                self._model.run(self._values, self._values['current_model_time'])
        # This is just a single model time step (1 hour) update
        else:
            self._model.run(self._values, future_time)

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
            dest[:] = [self.grid_0.id]
        elif var_name == "grid:ranks":
            dest[:] = [self.grid_0.rank]
        else:
            dest[:] = self.get_value_ptr(var_name)
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
        if(var_name == "grid_0_shape"): # FIXME cannot expose shape as ptr, because it has to side affect variable construction...
            return self.grid_0.shape
        if(var_name == "grid_0_spacing"):
            return self.grid_0.spacing
        if(var_name == "grid_0_origin"):
            return self.grid_0.origin
        if(var_name == "grid_0_units"):
            return self.grid_0.units

        if var_name not in self._values.keys():
            raise(UnknownBMIVariable(f"No known variable in BMI model: {var_name}"))

        shape = self._values[var_name].shape

        try:
            #see if raveling is possible without a copy
            self._values[var_name].shape = (-1,)
            #reset original shape
            self._values[var_name].shape = shape
        except ValueError as e:
            raise RuntimeError("Cannot flatten array without copying -- "+str(e).split(": ")[-1])

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
        return str(self.get_value_ptr(var_name).dtype)
    
    #------------------------------------------------------------ 
    def get_var_grid(self, name):
        
        # all vars have grid 0 but check if its in names list first
        if name in (self._output_var_names + self._input_var_names):
            if(name== "ETA2_bnd"):
                return 0 
            else:
                return self._var_grid_id
        raise(UnknownBMIVariable(f"No known variable in BMI model: {name}"))

    #------------------------------------------------------------ 
    def get_var_itemsize(self, name):
        return self.get_value_ptr(name).itemsize

    #------------------------------------------------------------ 
    def get_var_location(self, name):
        #FIXME what about grid vars?
        #if name in (self._output_var_names + self._input_var_names):
        #    return self._var_loc
        if(name == 'ETA2_bnd'):
            return "node"
        else:
            return None

    #-------------------------------------------------------------------
    def get_var_rank(self, long_var_name):
        if(long_var_name == "ETA2_bnd"):
            return np.int16(0)

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
        self._values[var_name][:] = values

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
    def get_grid_shape(self, grid_id):

        for grid in self._grids:
            if grid_id == grid.id:
                return grid.shape
        raise ValueError(f"get_grid_shape: grid_id {grid_id} unknown")

    #------------------------------------------------------------ 
    def get_grid_size(self, grid_id):
       
        for grid in self._grids:
            if grid_id == grid.id: 
                return grid.size
        raise ValueError(f"get_grid_size: grid_id {grid_id} unknown")

    #------------------------------------------------------------ 
    def get_grid_spacing(self, grid_id):

        for grid in self._grids:
            if grid_id == grid.id: 
                return grid.spacing
        raise ValueError(f"get_grid_spacing: grid_id {grid_id} unknown") 

    #------------------------------------------------------------ 
    def get_grid_type(self, grid_id):

        for grid in self._grids:
            if grid_id == grid.id: 
                return grid.type
        raise ValueError(f"get_grid_type: grid_id {grid_id} unknown")

    #------------------------------------------------------------ 
    def get_grid_x(self, grid_id: int):
        for grid in self._grids:
            if grid_id == grid.id: 
                return grid.grid_x
        raise ValueError(f"get_grid_x: grid_id {grid_id} unknown")

    #------------------------------------------------------------ 
    def get_grid_y(self, grid_id: int):
        for grid in self._grids:
            if grid_id == grid.id: 
                return grid.grid_y
        raise ValueError(f"get_grid_y: grid_id {grid_id} unknown")

    #------------------------------------------------------------ 
    def get_grid_z(self, grid_id: int):
        for grid in self._grids:
            if grid_id == grid.id: 
                return grid.grid_z
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
            #elif key.endswith('_timestamp'):
            #    cfg[key] = pd.to_datetime(val)
            else:
                pass

        # Add more config parsing if necessary
        return cfg
