#!/usr/bin/env python

from pathlib import Path
from os import chdir
import yaml
from tempfile import mkdtemp
import geopandas as gpd
import pandas as pd
from config import General
from typing import Union
import subprocess

from ngen.config.formulation import Formulation
from ngen.config.realization import Realization, NgenRealization
from ngen.config.configurations import Forcing, Time

#Path to directory containing this file, also contains the reouting_config.yaml template
__conf_dir: Path = Path(__file__).parent

class NgenRunException(Exception):
    pass

class RoutingRunException(Exception):
    pass

def stage_routing_config(hydrofabric, workdir: Path):
    """prepare a routing config for testing in the provided workdir

    Args:
        workdir Path: directory to write the config to
    """
    with open(__conf_dir/'routing_config.yaml', 'r') as template_file:
        template = yaml.safe_load(template_file)
    template['run_parameters']['nts'] = 1 #FIXME
    template['forcing_parameters']['nexus_input_folder'] = str(workdir)
    template['output_parameters']['csv_output']['csv_output_folder'] = str(workdir)
    template['supernetwork_parameters']['geo_file_path'] = hydrofabric
    with open(workdir/'test_routing_config.yaml', 'w') as test_file:
        yaml.dump(template, test_file)

def make_x_walk(hydrofabric):
    """

    Args:
        hydrofabric (_type_): _description_
    """
    attributes = gpd.read_file(hydrofabric, layer="flowpath_attributes").set_index('id')
    x_walk = pd.Series( attributes[ ~ attributes['rl_gages'].isna() ]['rl_gages'] )
    
    data = {}
    for wb, gage in x_walk.items():
        data[wb] = {'Gage_no':[gage]}
    import json
    with open('crosswalk.json', 'w') as fp:
        json.dump(data, fp, indent=2)

def make_geojson(hydrofabric: Path):
    """Create the various required geojson/json files from the geopkg

    Args:
        hydrofabric (Path): path to hydrofabric geopkg
    """
    try:
        catchments = gpd.read_file(hydrofabric, layer="divides")
        nexuses = gpd.read_file(hydrofabric, layer="nexus")
        flowpaths = gpd.read_file(hydrofabric, layer="flowpaths")
        edge_list = pd.DataFrame(gpd.read_file(hydrofabric, layer="flowpath_edge_list").drop(columns='geometry'))
        make_x_walk(hydrofabric)
        catchments.to_file("catchments.geojson")
        nexuses.to_file("nexus.geojson")
        flowpaths.to_file("flowpaths.geojson")
        edge_list.to_json("flowpath_edge_list.json", orient='records', indent=2)
    except Exception as e:
        print(f"Unable to use hydrofabric file {hydrofabric}")
        print(str(e))
        raise NgenRunException

def make_realization(workdir: Path, config: Formulation, forcing: Forcing, time: Time, include_routing: bool=False):
    """Generates a ngen realization file based on the user defined formulation.

    Args:
        workdir (Path): Location to stage the realization file in.
        config (Formulation): Formulation to build.
        forcing (Forcing): Forcing configuration to use.
        time (Time): Simulation time range.
        include_routing (bool, optional): Whether or not to add a routing block to the realization. Defaults to False.
                                          When true, assumes the routing config is in the workdir and called test_routing_config.yaml
    """

    f = Formulation(name=config['name'], params=config)
    r = Realization(formulations=[f], forcing=forcing)
    if(include_routing):
        routing = {'t_route_config_file_with_path': workdir/'test_routing_config.yaml'}
    else:
        routing = None
    g = NgenRealization(global_config=r, time=time, routing=routing)
    with open(workdir/"realization.json", 'w') as fp:
         fp.write( g.json(by_alias=True, exclude_none=True, indent=4))

#TODO on failue of check_call, output log to console???
def test_ngen(hydrofabric: Union[str, Path], config, tag=""):
    """_summary_

    Args:
        hydrofabric (Union[str, Path]): path to geopackage hydrofabric to test
        config (General): the user configuration
        tag (str, optional): A tag to prefix the temporary directory the model runs in. Defaults to "".

    Raises:
        NgenRunException: _description_
    """
    ngen = config.ngen
    workdir = config.workdir
    tmp_dir = Path(mkdtemp(prefix=tag, dir=workdir))
    chdir(tmp_dir)
    make_geojson(hydrofabric)
    #This is required if we want to test routing embedded in ngen, but right now it gets tested stand-alone
    stage_routing_config(hydrofabric, tmp_dir)
    make_realization(tmp_dir, config.formulation, config.forcing, config.time, config.routing.embedded)
    cmd = f'{ngen} catchments.geojson all nexus.geojson all realization.json'
    try:
        with open('ngen.log', 'w') as log_file:
            subprocess.check_call(cmd, stdout=log_file, stderr=log_file, shell=True, cwd=tmp_dir)
        print(f"ngen run successful on {hydrofabric}")
    except:
        print(f"Running ngen on hydrofabric {hydrofabric} failed, check {tmp_dir}/ngen.log")
        raise NgenRunException
    test_routing(tmp_dir, hydrofabric)
    chdir(workdir)

def test_routing(workdir, hydrofabric):
    """Test routing via ngen_main in an external call.

    Args:
        workdir (Path): working directory to run the test in
        hydrofabric (Path): which hydrofabric to test
    """

    if not (workdir/'test_routing_config.yaml').exists():
        stage_routing_config(hydrofabric, workdir)
    cmd = 'python -m ngen_routing.ngen_main -f test_routing_config.yaml'
    try:
        with open('troute.log', 'w') as log_file:
            subprocess.check_call(cmd, stdout=log_file, stderr=log_file, shell=True, cwd=workdir)
        print(f"troute run successful on {hydrofabric}")
    except:
        print(f"Running t-route on hydrofabric {hydrofabric} failed, check {workdir}/troute.log")
        raise RoutingRunException
        

#TODO do a little testing of required routing modules in current ENV???
#ngen_routing
#nwm_routing
#troute.network
#troute.routing
#TODO check t-route dependencies installed/packaged correctly
#TODO validate tables install???

def main(config: General):
    #in case relative paths were given, modify them here to be absolute before
    #changing working directories
    config.forcing.path = Path(config.forcing.path).absolute()
    if config.workdir is None:
        config.workdir = Path(mkdtemp(prefix="ngen_test_", dir=Path.cwd()))
    # change directory to workdir
    chdir(config.workdir)

    for resource in config.hydrofabric:
        if resource.is_file():
            try:
                test_ngen(str(resource), config, tag=resource.stem+"_")
            except (NgenRunException, RoutingRunException):
                continue
        elif resource.is_dir():
            geopkgs = resource.glob('*.gpkg')
            for fabric in geopkgs:
                try:
                    test_ngen(str(fabric), config, tag=resource.stem+"_")
                except (NgenRunException, RoutingRunException):
                    continue

if __name__ == "__main__":
    import argparse

    # get the command line parser
    parser = argparse.ArgumentParser(
        description='Test ngen and t-route with ngen hydrofabric')
    parser.add_argument('config_file', type=Path,
                        help='The configuration yaml file with ngen and hydrdofabric configuration information')

    args = parser.parse_args()
    
    with open(args.config_file) as file:
        conf = yaml.safe_load(file)
    
    general = General(**conf['general'])
    
    main(general)