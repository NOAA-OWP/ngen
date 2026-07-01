import numpy as np
import ESMF

import geopandas as gpd

import numpy
import netCDF4 as nc4
import os
import sys
from shapely.geometry import Point, Polygon

import ESMF.util.helpers as helpers
import ESMF.api.constants as constants

import glob
from pathlib import Path

from os import listdir
from os.path import isfile, join
import argparse
import pandas as pd
from  multiprocessing import Process, Lock
import multiprocessing
import time
import math

def get_cat_id(path):
    """
    Extract the id from the file path
    """
    path = Path(path)
    name = path.stem
    id = name.split('_')[0]
    return id

def get_date_time(path):
    """
    Extract the date-time from the file path
    """
    path = Path(path)
    name = path.stem
    date_time = name.split('.')[0]
    date_time = date_time.split('_')[1]  #this index may depend on the naming format of the forcing data
    return date_time

def get_cat_boundary(coord_list, cat_id):
    """
    Get the rectangle boundary that encompasses a catchment
    """
    # Read in polygon coordinates
    lons = []
    lats = []
    for coord in coord_list:
        lons.append(float(coord[0]))
        lats.append(float(coord[1]))

    #find max and min of the lons and lats of the polygon
    lons_array = np.array(lons)
    lats_array = np.array(lats)
    lons_max = np.max(lons_array)
    lons_min = np.min(lons_array)
    lats_max = np.max(lats_array)
    lats_min = np.min(lats_array)

    return lons_min, lons_max, lats_min, lats_max


def get_basin_geometry(aorcfile, lons_min, lons_max, lats_min, lats_max):
    """
    This function takes input from aorc netcdf file and the polygon boundary to set the limit
    for the rectangle that encompasses the polygon
    """

    ds = nc4.Dataset(aorcfile)

    lons_first = ds['longitude'][0]
    lons_2nd = ds['longitude'][1]
    lons_delta = lons_2nd - lons_first
    lats_first = ds['latitude'][0]
    lats_2nd = ds['latitude'][1]
    lats_delta = lats_2nd - lats_first

    #calculate local grid
    lons_min_grid = (lons_min - lons_first)/lons_delta
    #to fully encompass the polygon, we need to do the round down operation at lower boundary
    lons_min_grid = int(lons_min_grid)
    lons_max_grid = (lons_max - lons_first)/lons_delta
    #to fully encompass the polygon, we need to do the round up operation at the upper boundary
    lons_max_grid = int(lons_max_grid) + 2    # lons_max_grid = int(lons_max_grid) + 1 causes an out of range error
    lats_min_grid = (lats_min - lats_first)/lats_delta
    #to fully encompass the polygon, we need to do the round down operation at lower boundary
    lats_min_grid = int(lats_min_grid)
    lats_max_grid = (lats_max - lats_first)/lats_delta
    #to fully encompass the polygon, we need to do the round up operation at upper boundary
    lats_max_grid = int(lats_max_grid) + 2    # lats_max_grid = int(lats_max_grid) + 1 causes an out of range error

    ds.close()

    return lats_min_grid, lats_max_grid, lats_delta, lats_first, lons_min_grid, lons_max_grid, lons_delta, lons_first

def read_sub_netcdf(datafile, var_name_list, var_value_list, lons_min_grid, lons_max_grid, lats_min_grid, lats_max_grid, lons_delta, lats_delta):
    """
    Extract a sebset netcdf from the large original netcdf file
    """
    ds = nc4.Dataset(datafile)

    i = 0
    for var_name in var_name_list:
        if var_name == 'time':
            var_value = ds.variables['time'][0]
            var_value_list[0].append(var_value)
        elif var_name == 'latitude':
            var_value = ds.variables[var_name][lats_min_grid:lats_max_grid]
            var_value_list[1].append(var_value)
        elif var_name == 'longitude':
            var_value = ds.variables[var_name][lons_min_grid:lons_max_grid]
            var_value_list[2].append(var_value)
        else:
            var_value = ds.variables[var_name][0, lats_min_grid:lats_max_grid, lons_min_grid:lons_max_grid]
            var_value_list[i].append(var_value)
        i += 1

    lons_first = ds['longitude'][0]
    lats_first = ds['latitude'][0]
    nlons = lons_max_grid - lons_min_grid
    nlats = lats_max_grid - lats_min_grid

    Var_array = []
    for var_name in var_name_list:
        Var_array.append([])

    for i in range(len(var_name_list)):
        Var_array[i] = numpy.array(var_value_list[i])

    return var_value_list, Var_array, nlats, nlons, ds


def write_sub_netcdf(filename_out, var_name_list, var_value_list, Var_array, nlats, nlons, ds):
    """
    Write the extracted data to a netcdf file per catchment
    """
    ntime = Var_array[0].size

    ncfile_out = nc4.Dataset(filename_out, 'w', format='NETCDF4')
    ncfile_out.createDimension('time', None)
    ncfile_out.createDimension('lat', nlats)
    ncfile_out.createDimension('lon', nlons)

    time = Var_array[0]
    lat = Var_array[1][0]
    lon = Var_array[2][0]
    APCP_surface = Var_array[3]
    DLWRF_surface = Var_array[4]
    DSWRF_surface = Var_array[5]
    PRES_surface = Var_array[6]
    SPFH_2maboveground = Var_array[7]
    TMP_2maboveground = Var_array[8]
    UGRD_10maboveground = Var_array[9]
    VGRD_10maboveground = Var_array[10]

    time_out = ncfile_out.createVariable('time', 'double', ('time',), fill_value=-32767.)
    lat_out = ncfile_out.createVariable('latitude', 'double', ('lat',), fill_value=-32767.)
    lon_out = ncfile_out.createVariable('longitude', 'double', ('lon',), fill_value=-32767.)
    APCP_surface_out = ncfile_out.createVariable('APCP_surface', 'f4', ('time', 'lat', 'lon',), fill_value=-32767)
    DLWRF_surface_out = ncfile_out.createVariable('DLWRF_surface', 'f4', ('time', 'lat', 'lon',), fill_value=-32767)
    DSWRF_surface_out = ncfile_out.createVariable('DSWRF_surface', 'f4', ('time', 'lat', 'lon',), fill_value=-32767)
    PRES_surface_out = ncfile_out.createVariable('PRES_surface', 'f4', ('time', 'lat', 'lon',), fill_value=-32767)
    SPFH_2maboveground_out = ncfile_out.createVariable('SPFH_2maboveground', 'f4', ('time', 'lat', 'lon',), fill_value=-32767)
    TMP_2maboveground_out = ncfile_out.createVariable('TMP_2maboveground', 'f4', ('time', 'lat', 'lon',), fill_value=-32767)
    UGRD_10maboveground_out = ncfile_out.createVariable('UGRD_10maboveground', 'f4', ('time', 'lat', 'lon',), fill_value=-32767)
    VGRD_10maboveground_out = ncfile_out.createVariable('VGRD_10maboveground', 'f4', ('time', 'lat', 'lon',), fill_value=-32767)
    #APCP_surface_out[:,:,:] = Var_array[3]

    varout_dict = {'time':time_out, 'latitude':lat_out, 'longitude':lon_out,
                   'APCP_surface':APCP_surface_out, 'DLWRF_surface':DLWRF_surface_out, 'DSWRF_surface':DSWRF_surface_out,
                   'PRES_surface':PRES_surface_out, 'SPFH_2maboveground':SPFH_2maboveground_out, 'TMP_2maboveground':TMP_2maboveground_out,
                   'UGRD_10maboveground':UGRD_10maboveground_out, 'VGRD_10maboveground':VGRD_10maboveground_out}

    for name, variable in ds.variables.items():
        varout_name = varout_dict[name]
        for attrname in variable.ncattrs():
            if attrname != '_FillValue':
                setattr(varout_name, attrname, getattr(variable, attrname))

    time_out[:] = time
    lat_out[:] = lat
    lon_out[:] = lon
    APCP_surface_out[:,:,:] = APCP_surface[:,:,:]
    DLWRF_surface_out[:,:,:] = DLWRF_surface[:,:,:]
    DSWRF_surface_out[:,:,:] = DSWRF_surface[:,:,:]
    PRES_surface_out[:,:,:] = PRES_surface[:,:,:]
    SPFH_2maboveground_out[:,:,:] = SPFH_2maboveground[:,:,:]
    TMP_2maboveground_out[:,:,:] = TMP_2maboveground[:,:,:]
    UGRD_10maboveground_out[:,:,:] = UGRD_10maboveground[:,:,:]
    VGRD_10maboveground_out[:,:,:] = VGRD_10maboveground[:,:,:]
    ncfile_out.close()

def process_sublist(data : dict, lock: Lock, num: int, return_dict: dict):
    """
    The sub-process processes a sub-set of catchments and return the min and max on this domain as a dictionary
    """
    num_inputs = len(data["g_sublist"])

    #initialize the local min and max, note that logitude is negative in west hemisphere
    #latitude is posive in northern hemisphere
    lons_min_loc = 0.0
    lons_max_loc = -180.0
    lats_min_loc = 90.0
    lats_max_loc = 0.0

    for i in range(num_inputs):
        #extract data
        pos = data["offsets"][i]
        g_sublist = data["g_sublist"][i]

        #extract catchment geometry
        x,y = g_sublist.exterior.coords.xy
        all_coords = np.dstack((x,y))
        coord_list = all_coords[0]
        cat_id = data["cat_ids"][i]

        #extract catchment boundary values and calculate limits for the sub-set of catchments on this process
        (lons_min, lons_max, lats_min, lats_max) = get_cat_boundary(coord_list, cat_id)
        lons_min_loc = min(lons_min_loc, lons_min)
        lons_max_loc = max(lons_max_loc, lons_max)
        lats_min_loc = min(lats_min_loc, lats_min)
        lats_max_loc = max(lats_max_loc, lats_max)

    return_dict[num] = {'lons_min_loc': lons_min_loc, 'lons_max_loc': lons_max_loc, 'lats_min_loc': lats_min_loc, 'lats_max_loc': lats_max_loc}

if __name__ == '__main__':
    #parse the input and output root directory
    #example for huc01: run code with "python code_name -i /local/esmpy -o /local/esmpy
    parser = argparse.ArgumentParser()
    parser.add_argument("-i", dest="input_root", type=str, required=True, help="The input directory with csv files")
    parser.add_argument("-o", dest="output_root", type=str, required=True, help="The output file path")
    args = parser.parse_args()

    #retrieve parsed values
    input_root = args.input_root
    output_root = args.output_root

    #multiprocessing
    manager = multiprocessing.Manager()
    return_dict = manager.dict()

    #generate catchment geometry from hydrofabric 
    hyfabfile = "/local/ngen/data/huc01/huc_01/hydrofabric/spatial/catchment_data.geojson"
    cat_df_full = gpd.read_file(hyfabfile)
    g = [i for i in cat_df_full.geometry]
    h = [i for i in cat_df_full.id]
    n_cats = len(g)
    print("number of catchments = {}".format(n_cats))

    #find out the indices of the two problematic catchments, not related to any calculation
    cat_dict = {}
    for i in range(n_cats):
        if h[i] == "cat-39990":
            cat_dict.update({"cat-39990":i})
            print("for cat-39990, list index = {}".format(i))
        if h[i] == "cat-39965":
            cat_dict.update({"cat-39965":i})
            print("for cat-39965, list index = {}".format(i))


    #prepare for processing
    num_catchments = len(g)
    num_processes = 50

    #generate the data objects for child processes
    g_groups = np.array_split(np.array(g), num_processes)
    cat_groups = np.array_split(np.array(h), num_processes)
    pos_groups = np.array_split(np.array(range(n_cats)), num_processes)

    process_data = []
    process_list = []
    lock = Lock()

    for i in range(num_processes):
        # fill the dictionary with needed data
        data = {}
        data["g_sublist"] = g_groups[i].tolist()
        data["cat_ids"] = cat_groups[i]
        data["offsets"] = pos_groups[i]

        #append to the list
        process_data.append(data)

        p = Process(target=process_sublist, args=(data, lock, i, return_dict))

        process_list.append(p)

    #start all processes
    for p in process_list:
        p.start()

    #wait for termination
    for p in process_list:
        p.join()
    #end multiprocessing

    #find the global min and max for the whole huc01 region
    lons_min_global = 0.0
    lons_max_global = -180.0
    lats_min_global = 90.0
    lats_max_global = 0.0
    for i in range(num_processes):
        lons_min_loc = return_dict.values()[i]['lons_min_loc']
        lons_min_global = min(lons_min_global, lons_min_loc)
        lons_max_loc = return_dict.values()[i]['lons_max_loc']
        lons_max_global = max(lons_max_global, lons_max_loc)
        lats_min_loc = return_dict.values()[i]['lats_min_loc']
        lats_min_global = min(lats_min_global, lats_min_loc)
        lats_max_loc = return_dict.values()[i]['lats_max_loc']
        lats_max_global = max(lats_max_global, lats_max_loc)
    print("lons_min_global = {}".format(lons_min_global))
    print("lons_max_global = {}".format(lons_max_global))
    print("lats_min_global = {}".format(lats_min_global))
    print("lats_max_global = {}".format(lats_max_global))

    datafile_path = join(input_root, "huc01/aorc_netcdf/", "AORC-OWP_*.nc4")    # run the whole data set
    #datafile_path = join(input_root, "huc01/aorc_netcdf_test/", "AORC-OWP_*.nc4")    # run a few input forcing file
    #datafile_path = join(input_root, "huc01/aorc_netcdf_test/", "AORC-OWP_2012062018z.nc4")    # run a single file for fast testing
    datafiles = glob.glob(datafile_path)
    print("number of forcing files = {}".format(len(datafiles)))
    #process data with time ordered
    datafiles.sort()

    # some function only executed once at the beginning (k = 0)
    k = 0
    for datafile in datafiles:
        if k == 0:
            aorcfile = datafile    #save a file to get attributes

            #define the boundary for extract sub-netcdf data
            lats_min_grid, lats_max_grid, lats_delta, lats_first, lons_min_grid, lons_max_grid, lons_delta, lons_first = get_basin_geometry(
            aorcfile, lons_min_global, lons_max_global, lats_min_global, lats_max_global)

        var_name_list = ['time', 'latitude', 'longitude', 'APCP_surface', 'DLWRF_surface', 'DSWRF_surface', 'PRES_surface', 'SPFH_2maboveground',
                         'TMP_2maboveground', 'UGRD_10maboveground', 'VGRD_10maboveground']

        var_value_list = []
        for var_name in var_name_list:
            var_value_list.append([])

        #extract sub-netcdf data
        var_value_list, Var_array, nlats, nlons, ds = read_sub_netcdf(datafile, var_name_list, var_value_list, lons_min_grid, lons_max_grid,
                                                                      lats_min_grid, lats_max_grid, lons_delta, lats_delta)

        #write to netcdf file for the region encompassing huc01
        date_time = get_date_time(datafile)
        filename_out = join(output_root, "huc01/aorc_netcdf_sub/", "AORC-OWP_"+date_time+".nc4")
        write_sub_netcdf(filename_out, var_name_list, var_value_list, Var_array, nlats, nlons, ds)

        if k%10 == 0:
            print("processing {}-th datafile".format(k))

        #increment counter
        k += 1

