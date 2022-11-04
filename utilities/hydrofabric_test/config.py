#Typing, datamodel
from pydantic import BaseModel, FilePath, DirectoryPath, validator
from typing import Optional, Union, List, Mapping
from pathlib import Path
from cloudpathlib import CloudPath
from os.path import expanduser
from collections.abc import Iterable 


from ngen.config.configurations import Forcing, Time

class RoutingOptions(BaseModel):
    """
        Extension of Routing providing options for running routing embedded in ngen or stand alone
    """
    embedded: bool = True

class General(BaseModel):
    """
        General ngen-cal configuration requirements
    """
    #required fields
    ngen: str
    hydrofabric: Union[CloudPath, FilePath, DirectoryPath, List[Union[CloudPath, FilePath]]]
    forcing: Forcing
    time: Time
    routing: RoutingOptions
    formulation: Mapping[ str, Union[str, Mapping[str, str]] ]
    #Fields with reasonable defaults
    #restart: bool = False
    #Optional fields
    workdir: Optional[Path]
    #log_file: Optional[Path]
    @validator("hydrofabric", pre=True)
    def expand_home(cls, v):
        if v.startswith("~"):
            return expanduser(v)
        return v
    
    @validator("hydrofabric", pre=True)
    def make_list(cls, v):
        if isinstance(v, Iterable) and not isinstance(v, str):
            return v
        return [v]