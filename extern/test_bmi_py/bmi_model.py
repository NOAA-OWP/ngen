# Need these for BMI
# This is needed for get_var_bytes
from pathlib import Path

# import data_tools
# Basic utilities
import numpy as np
import pandas as pd
# Configuration file functionality
import yaml
from bmipy import Bmi

from .bmi_grid import Grid, GridType
# Here is the model we want to run
from .model import ngen_model

class UnknownBMIVariable(RuntimeError):
    pass

class bmi_model(Bmi):

    def __init__(self):
        """Create a model that is ready for initialization."""
        super(bmi_model, self).__init__()
        self._values = {}
        self._var_loc = "node"      # JG Edit
        self._var_grid_id = 0       # JG Edit
        self._start_time = 0.0
        self._end_time = np.finfo(float).max
        self._model = None
        self.var_array_lengths = 1

        self.grid_0: Grid = Grid(0, 0, GridType.scalar) #Grid 0 is a 0 dimension "grid" for scalars
        self.grid_1: Grid = Grid(1, 2, GridType.uniform_rectilinear) #Grid 1 is a 2 dimensional grid
        self.grid_2: Grid = Grid(2, 3, GridType.rectilinear) #Grid 2 is a 3 dimensional grid

        self._grids = [self.grid_0, self.grid_1, self.grid_2]

        #TODO this can be done more elegantly using a more coherent data structure representing a BMI variable
        self._grid_map = {'INPUT_VAR_1': self.grid_0, 'INPUT_VAR_2': self.grid_0, 'GRID_VAR_1': self.grid_1,
                          'OUTPUT_VAR_1': self.grid_0, 'OUTPUT_VAR_2': self.grid_0, 'OUTPUT_VAR_3': self.grid_0,
                          'GRID_VAR_2': self.grid_1, 'GRID_VAR_3': self.grid_0}
    #----------------------------------------------
    # Required, static attributes of the model
    #----------------------------------------------
    _att_map = {
        'model_name':         'Test Python model for Next Generation NWM',
        'version':            '1.0',
        'author_name':        'Jonathan Martin Frame',
        'grid_type':          'scalar&uniform_rectilinear',
        'time_units':         'seconds',
               }

    #---------------------------------------------
    # Input variable names (CSDMS standard names)
    #---------------------------------------------
    #will use these implicitly for grid meta data, could in theory advertise them???
    #grid_1_shape, grid_1_size, grid_1_origin
    _input_var_names = ['INPUT_VAR_1', 'INPUT_VAR_2', 'GRID_VAR_1']
    _input_var_types = {'INPUT_VAR_1': float, 'INPUT_VAR_2': np.int32}

    #---------------------------------------------
    # Output variable names (CSDMS standard names)
    #---------------------------------------------
    #will use these implicitly for grid meta data, could in theory advertise them?
    # grid_1_rank -- could probably establish a pattern grid_{id}_rank for managing different ones?
    _output_var_names = ['OUTPUT_VAR_1', 'OUTPUT_VAR_2', 'OUTPUT_VAR_3', 'GRID_VAR_2', 'GRID_VAR_3']

    #------------------------------------------------------
    # Create a Python dictionary that maps CSDMS Standard
    # Names to the model's internal variable names.
    # This is going to get long, 
    #     since the input variable names could come from any forcing...
    #------------------------------------------------------
    #_var_name_map_long_first = {
    _var_name_units_map = {'INPUT_VAR_1':['INPUT_VAR_1','-'],
                           'INPUT_VAR_2':['INPUT_VAR_2','-'],
                           'OUTPUT_VAR_1':['OUTPUT_VAR_1','-'],
                           'OUTPUT_VAR_2':['OUTPUT_VAR_2','-'],
                           'OUTPUT_VAR_3':['OUTPUT_VAR_3','-'],
                           'GRID_VAR_1':['OUTPUT_VAR_1','-'],
                           'GRID_VAR_2':['GRID_VAR_2','-'],
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
        self.var_array_lengths = 1

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
            if model_output == "OUTPUT_VAR_3":
                self._values[model_output] = np.arange(3, dtype=float)
            else:
                #TODO why is output var 3 an arange?  should this be a unique "grid"?
                if self._grid_map[model_output].type == GridType.scalar:
                    self._values[model_output] = np.zeros( (), dtype=float)
                else: 
                    self._values[model_output] = np.zeros(self._grid_map[model_output].shape, dtype=float)
        #print(self._values)

        # ------------- Set time to initial value -----------------------#
        self._values['current_model_time'] = self.cfg_bmi['initial_time']
        
        # ------------- Set time step size -----------------------#
        self._values['time_step_size'] = self.cfg_bmi['time_step_seconds']

        # ------------- Initialize a model ------------------------------#
        #self._model = ngen_model(self._values.keys())
        self._model = ngen_model()

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
        self._model.run(self._values, update_delta_t)
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
            if(name == "GRID_VAR_1" or name == "GRID_VAR_2"):
                return 1 #FIXME remove "magic number"
            if(name == "GRID_VAR_3"):
                return 2
            else:
                return self._var_grid_id
        raise(UnknownBMIVariable(f"No known variable in BMI model: {name}"))

    #------------------------------------------------------------ 
    def get_var_itemsize(self, name):
        return self.get_value_ptr(name).itemsize

    #------------------------------------------------------------ 
    def get_var_location(self, name):
        #FIXME what about grid vars?
        if name in (self._output_var_names + self._input_var_names):
            return self._var_loc

    #-------------------------------------------------------------------
    def get_var_rank(self, long_var_name):
        if(long_var_name == "GRID_VAR_1" or long_var_name == "GRID_VAR_2"):
            return np.int16(2) #FIXME magic number
        else:
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