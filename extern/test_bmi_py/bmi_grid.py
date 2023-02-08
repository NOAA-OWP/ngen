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
    from numpy import ndarray

class GridType(str, Enum):
    """
        Enumeration of supported BMI grid types (https://bmi.readthedocs.io/en/stable/#get-grid-type)
    """
    scalar = "scalar",
    points = "points",
    vector = "vector",
    unstructured = "unstructured",
    structured_quadrilateral = "structured_quadrilaterl",
    rectilinear = "rectilinear",
    uniform_rectilinear = "uniform_rectilinear"

class Grid():
    """
        Structure for holding required BMI meta data for any grid intended to be used via BMI
    """
    def __init__(self, id: int, rank: int , type: GridType):
        """_summary_

        Args:
            id (int): User defined identifier for this grid
            rank (int): The number of dimensions of the grid
            type (GridType): The type of BMI grid this meta data represents
        """
        self._id: int = id
        self._rank: int = rank
        self._size: int = 0 #FIXME make property
        self._type: GridType = type #FIXME validate type/rank?
        self._shape: Tuple[int] = None #tuple of size rank
        self._spacing: Tuple[float] = None #tuple of size rank
        self._origin: Tuple[float] = None #tuple of size rank
    
        self._shape = tuple((0 for i in range(rank))) #set the shape rank, with 0 allocated values

    # TODO consider restricting resetting of grid values after they have been initialized

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
        if not self.shape: #it is None or ()
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
    def shape(self) -> 'Tuple[int]':
        """The shape of the grid (the size of each dimension)

        Returns:
            Tuple[int]: size of each dimension
        """
        return self._shape
    
    @shape.setter
    def shape(self, shape: 'Tuple[int]') -> None:
        """Set the shape of the grid to the provided shape

        Args:
            shape (Tuple[int]): the size of each dimension of the grid
        """
        self._shape = shape
    
    @property
    def spacing(self) -> 'Tuple[float]':
        """The spacing of the grid

        Returns:
            Tuple[float]: Tuple of size rank with the spacing in each of rank dimensions
        """
        return self._spacing
    
    @spacing.setter
    def spacing(self, spacing: 'Tuple[float]') -> None:
        """Set the spacing of each grid dimension.

        Args:
            spacing (Tuple[float]): Tuple of size rank with the spacing for each dimension
        """
        self._spacing = spacing

    @property
    def origin(self) -> 'Tuple[float]':
        """The origin point of the grid

        Returns:
            Tuple[float]: Tuple of size rank with the coordinates of the the grid origin
        """
        return self._origin
    
    @origin.setter
    def origin(self, origin: 'Tuple[float]') -> None:
        """Set the grid origin location

        Args:
            origin (Tuple[float]): Tuple of size rank with grid origin coordinates.
        """
        self._origin = origin

    @property
    def grid_x(self) -> 'ndarray':
        """Coordinates of the x components of the grid

        Returns:
            ndarray: array of cooridnate values in the x direction
        """
        if len(self.shape) > 0:
            return np.array( [ self.origin[0] + self.spacing[0]*x for x in range(self.shape[0]) ] )
        else:    
            #TODO should this raise an error or return an empty array?
            #raise RuntimeError(f"Cannot get x coordinates of grid with shape {self.shape}")
            return np.array(())
    
    @property
    def grid_y(self) -> 'ndarray':
        """Coordinates of the y components of the grid

        Returns:
            ndarray: array of coordinate values in the y direction
        """
        if len(self.shape) > 1:
            return np.array( [ self.origin[1] + self.spacing[1]*y for y in range(self.shape[1]) ] )
        else:    
            #TODO should this raise an error or return an empty array?
            #raise RuntimeError(f"Cannot get y coordinates of grid with shape {self.shape}")
            return np.array(())
    
    @property
    def grid_z(self) -> 'ndarray':
        """Coordinates of the z components of the grid

        Returns:
            ndarray: array of coordinate values in the z direction
        """
        if len(self.shape) > 2:
            return np.array( [ self.origin[2] + self.spacing[2]*z for z in range(self.shape[2]) ] )
        else:    
            #TODO should this raise an error or return an empty array?
            #raise RuntimeError(f"Cannot get z coordinates of grid with shape {self.shape}")
            return np.array(())
