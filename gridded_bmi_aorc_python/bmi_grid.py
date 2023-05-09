"""bmi_grid.py
Module for supporting BMI grid meta data and functionality

@author Nels Frazier
@version 0.1
"""

from enum import Enum
from functools import reduce
from typing import TYPE_CHECKING
import numpy as np

if TYPE_CHECKING:
    from typing import Tuple
    from numpy.typing import NDArray

_error_on_grid_type: bool = False

def error_on_grid_type(flag: bool = False) -> None:
    """Set the behavior of the module to throw an error on grid functions that are not applicable.

    Args:
        flag (bool): 
            True will enable exceptions to be thrown for access to grid functions that are not applicable to the grid type.
            
            False will attempt to provide reasonable default semantics for returning information from those functions.

    Returns:
        None
    """
    global _error_on_grid_type
    _error_on_grid_type = flag

class GridTypeAccessError(TypeError):
    """Grid meta data not accessible for grid type"""

class GridType(str, Enum):
    """
        Enumeration of supported BMI grid types (https://bmi.readthedocs.io/en/stable/#get-grid-type)
    """
    scalar = "scalar", #0 dim
    points = "points", #1 dim
    vector = "vector", #1 dim
    unstructured = "unstructured", #1-N
    structured_quadrilateral = "structured_quadrilaterl", #2 dim
    rectilinear = "rectilinear", #2 dim dx != dy
    uniform_rectilinear = "uniform_rectilinear" #2 dim -- dx = dy

class GridUnits(Enum):
    """
        Enumeration of supported Grid Units to facilitate framework interactions
    """
    none = -1
    m = 0
    km = 1

class Grid():
    """
        Structure for holding required BMI meta data for any grid intended to be used via BMI
    """
    def __init__(self, id: int, rank: int , type: GridType, units: GridUnits = GridUnits.none) :
        """_summary_

        Args:
            id (int): User defined identifier for this grid
            rank (int): The number of dimensions of the grid
            type (GridType): The type of BMI grid this meta data represents
        """
        self._id: int = id
        self._rank: int = rank
        self._size: int = 0
        self._type: GridType = type #FIXME validate type/rank?
        self._shape: 'NDArray[np.int32]' = None #array of size rank
        self._spacing: 'NDArray[np.float64]' = None #array of size rank
        self._origin: 'NDArray[np.float64]' = None #array of size rank
        self._units: 'NDArray[np.int16]' = None #array of size rank

        if( rank == 0 ):
            # We have to use a 1 dim representation for a scalar cause numpy initialization is weird
            # np.zeros( [1] ) gives you an array([0.])
            # np.zeros( [0] ) gives you an emptty array([])
            # np.zeros( () ) gives a scalar wrapped in an array array(0.)
            # This latter is really what we want, but then it is hard to communicate the actual size
            # (as a numerical value...)
            self._shape = np.zeros( (), np.int32 ) #note, int32 is important here -- assumed by ngen
            self._spacing = np.zeros( (), np.float64 )
            self._origin = np.zeros( (), np.float64 )
            self._units = np.ones( (), dtype=np.int16)
            self._units[()] = units.value
            #self._shape[...] = 1
        else:
            self._shape = np.zeros( rank, np.int32) #set the shape rank, with 0 allocated values
            self._spacing = np.zeros( rank, np.float64 )
            self._origin = np.zeros( rank, np.float64 )
            self._units = np.array( [units.value]*rank, dtype=np.int16)
        #Make the array "immutable", can only modify via setting
        self._shape.flags.writeable = False
 
    # TODO consider restricting resetting of grid values after they have been initialized

    @property
    def units(self) -> 'NDArray[np.int16]':
        """The grid units, specifically the units of grid spacing in each rank

        Raises:
            GridTypeAccessError: When error_on_grid_type is toggled on, this exeption is raised when called on a scalar grid.

        Returns:
            NDArray[np.int16]: array of GridUnit enum VALUES denoting the grid unit for each rank.
        """
        if _error_on_grid_type and self.type == GridType.scalar:
            raise GridTypeAccessError("Scalar has no grid units")
        return self._units
    
    @units.setter
    def units(self, units: 'NDArray[np.int16]'):
        """Set the grid spacing units for each dimension.

        Args:
            units (NDArray[np.int16]): the GridUnit enum VALUES denoting the grid spacing units in each dimension.
        """

        if self.rank > 0:
            self._units = np.array(units, dtype=np.int16)
            self._units.flags.writeable = False
        #noop for scalar or grids with rank < 1

    @property
    def id(self) -> int:
        """The unique grid identifer.

        Returns:
            int: grid identifier
        """
        return self._id

    @property
    def rank(self) -> int:
        """The dimensionality of the grid.

        Returns:
            int: Number of dimensions of the grid.
        """
        return self._rank

    @property
    def size(self) -> int:
        """The total number of elements in the grid

        Returns:
            int: number of grid elements
        """
        if _error_on_grid_type and self.type == GridType.scalar:
            raise GridTypeAccessError("Scalar has no grid size")

        if self.shape is None or self.shape.ndim == 0: #it is None or np.array( () )
            return 0
        else:
            #multiply the shape of each dimension together
            return reduce( lambda x, y: x*y, self._shape)

    @property
    def type(self) -> GridType:
        """The type of BMI grid.

        Returns:
            GridType: bmi grid type
        """
        return self._type
    
    @property
    def shape(self) -> 'NDArray[np.int32]':
        """The shape of the grid (the size of each dimension)

        Returns:
            NDArray[np.int32]: size of each dimension
        """
        """
            From the BMI docs:
            The grid shape is the number of rows and columns of nodes,
            as opposed to other types of element (such as cells or faces). 
            It is possible for grid values to be associated with the nodes or with the cells.
        """
        if _error_on_grid_type and self.type == GridType.scalar:
            raise GridTypeAccessError("Scalar has no shape")
        return self._shape
    
    @shape.setter
    def shape(self, shape: 'NDArray[np.int32]') -> None:
        """Set the shape of the grid to the provided shape

        Args:
            shape (NDArray[np.int32]): the size of each dimension of the grid
        """
        #Create a new shape array and replace the old one, make it immutable
        if self.rank > 0:
            self._shape = np.array(shape, dtype=np.int32)
            self._shape.flags.writeable = False
        #noop for scalar or grids with rank < 1

    @property
    def spacing(self) -> 'NDArray[np.float64]':
        """The spacing of the grid

        Returns:
            NDArray[np.float64]: Tuple of size rank with the spacing in each of rank dimensions
        """
        if _error_on_grid_type and self.type == GridType.scalar:
            raise GridTypeAccessError("Scalar has no grid spacing")
        return self._spacing
    
    @spacing.setter
    def spacing(self, spacing: 'NDArray[np.float64]') -> None:
        """Set the spacing of each grid dimension.

        Args:
            spacing (NDArray[np.float64]): Tuple of size rank with the spacing for each dimension
        """
        if self.rank > 0:
            self._spacing = np.array(spacing, dtype=np.float64)
            self._spacing.flags.writeable = False
        #noop for scalar or grids with rank < 1

    @property
    def origin(self) -> 'NDArray[np.float64]':
        """The origin point of the grid

        Returns:
            NDArray[np.float64]: Tuple of size rank with the coordinates of the the grid origin
        """
        if _error_on_grid_type and self.type == GridType.scalar:
            raise GridTypeAccessError("Scalar has no grid origin")
        return self._origin
    
    @origin.setter
    def origin(self, origin: 'NDArray[np.float64]') -> None:
        """Set the grid origin location

        Args:
            origin (NDArray[np.float64]): Tuple of size rank with grid origin coordinates.
        """
        if self.rank > 0:
            self._origin = np.array(origin, dtype=np.float64)
            self._origin.flags.writeable = False
        #noop for scalar or grids with rank < 1

    @property
    def grid_x(self) -> 'NDArray[np.float64]':
        """Coordinates of the x components of the grid

        Returns:
            NDArray[np.float64]: array of cooridnate values in the x direction
        """
        if _error_on_grid_type and self.type == GridType.scalar:
            raise GridTypeAccessError("Scalar has no grid x value")
        #TODO refactor this -- not generic to grid, this works for structured/quads, not for unstructured
        if (self.type == GridType.rectilinear or self.type == GridType.uniform_rectilinear) and len(self.shape) > 0:
            # https://bmi.readthedocs.io/en/stable/model_grids.html#model-grids
            # bmi states dimension info in `ij` form (last dimension indexed first...) in the shape meta
            # so x would at index rank, y at rank-1, z at rank-2 ect...
            idx = self.rank-1 #index is 0 based, rank is 1 based, adjust...
            return np.array( [ self.origin[idx] + self.spacing[idx]*x for x in range(self.shape[idx]) ], dtype=np.float64 )
        else:    
            #TODO should this raise an error or return an empty array?
            #raise RuntimeError(f"Cannot get x coordinates of grid with shape {self.shape}")
            return np.array((), dtype=np.float64)
    
    @property
    def grid_y(self) -> 'NDArray[np.float64]':
        """Coordinates of the y components of the grid

        Returns:
            NDArray[np.float64]: array of coordinate values in the y direction
        """
        if _error_on_grid_type and self.type == GridType.scalar:
            raise GridTypeAccessError("Scalar has no grid y value")

        if (self.type == GridType.rectilinear or self.type == GridType.uniform_rectilinear) and len(self.shape) > 1:
            idx = self.rank-2 #index is 0 based, rank is 1 based, adjust...
            return np.array( [ self.origin[idx] + self.spacing[idx]*y for y in range(self.shape[idx]) ], dtype=np.float64 )
        else:    
            #TODO should this raise an error or return an empty array?
            #raise RuntimeError(f"Cannot get y coordinates of grid with shape {self.shape}")
            return np.array((), dtype=np.float64)
    
    @property
    def grid_z(self) -> 'NDArray[np.float64]':
        """Coordinates of the z components of the grid

        Returns:
            NDArray[np.float64]: array of coordinate values in the z direction
        """
        if _error_on_grid_type and self.type == GridType.scalar:
            raise GridTypeAccessError("Scalar has no grid z value")

        if (self.type == GridType.rectilinear or self.type == GridType.uniform_rectilinear) and len(self.shape) > 2:
            idx = self.rank-3 #index is 0 based, rank is 1 based, adjust...
            return np.array( [ self.origin[idx] + self.spacing[idx]*z for z in range(self.shape[idx]) ], dtype=np.float64 )
        else:    
            #TODO should this raise an error or return an empty array?
            #raise RuntimeError(f"Cannot get z coordinates of grid with shape {self.shape}")
            return np.array((), dtype=np.float64)
