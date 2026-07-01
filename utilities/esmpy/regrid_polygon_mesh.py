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

def mesh_create_polygon(coord_list, cat_id, cleanup_geometry=False):
    """
    Create a ESMF mesh from the infile that contains the coordinates of the polygon
    corresponding to a catchment
    """
    # Read in polygon coordinates
    node_id = []
    lons = []
    lats = []
    poly_coords = []
    node_id1 = range(len(coord_list))
    i = 0
    for coord in coord_list:
        node_id.append(i)
        lons.append(float(coord[0]))
        lats.append(float(coord[1]))
        thistuple = (float(coord[0]), float(coord[1]))
        poly_coords.append(thistuple)
        i += 1

    #FIXME The following code is specific for the current huc01 hydrofabric.
    # Note also that this part of code does not throw an exception, so no exception to catch here. Rather,
    # it generates a ValueError when the function mesh.add_elements() is executed.
    # This part of the codes correct the error present in the hydrofabric data for the
    # two listed catchments that contains zero angle and clockwise ordering that cause meshing error
    # This can be removed for hydrofabric without error or can be used as a validation check
    # for a new hydrofabric
    if cleanup_geometry:
        #only incorrect catchments need clean up
        if cat_id == 'cat-39990' or cat_id == 'cat-39965':
            # check that the angle is not zero
            index = []
            for i in range(len(lons)-2):
                x0 = lons[i]
                y0 = lats[i]
                x1 = lons[i+1]
                y1 = lats[i+1]
                x2 = lons[i+2]
                y2 = lats[i+2]
                vec1x = x0 - x1
                vec1y = y0 - y1
                vec2x = x2 - x1
                vec2y = y2 - y1
                dot_prod = vec1x*vec2x + vec1y*vec2y
                abs_vec1 = math.sqrt(vec1x**2 + vec1y**2)
                abs_vec2 = math.sqrt(vec2x**2 + vec2y**2)
                try:
                    norm_dot_prod = dot_prod / (abs_vec1 * abs_vec2)
                    angle = math.acos(norm_dot_prod)
                    if (abs(norm_dot_prod-1.0) < 10e-6):
                        index.append(i+1)    # (i+1)th element is the apex of the angle
                except ZeroDivisionError:
                    raise(ZeroDivisionError("ZeroDivisionError, abs_vec1, abs_vec2 = {}, {}".format(abs_vec1, abs_vec2)))

            #remove the vertices associated with zero angle
            idx = index[0]
            del node_id[idx]
            # keep the node_id contiguous
            for i in range(len(node_id)):
                if i >= idx:
                    node_id[i] = node_id[i] - 1
            del lons[idx]
            del lats[idx]
            del poly_coords[idx]
            # Handle the clockwise ordering of polygon vertices
            node_id.reverse()
            lons.reverse()
            lats.reverse()
            poly_coords.reverse()

    #add the center of the polygon to the nodes
    elem_id = []
    elem_id.append(int(0))
    elemId = np.array(elem_id)

    #two parametric dimensions, and two spatial dimensions
    mesh = ESMF.Mesh(parametric_dim=2, spatial_dim=2, coord_sys=ESMF.CoordSys.SPH_DEG)
    
    #number of nodes equal to the number of vortices of the polygon
    num_node = len(node_id)
    num_elem = 1
    nodeId = np.array(node_id)

    #find max and min of the lons and lats of the polygon
    lons_array = np.array(lons)
    lats_array = np.array(lats)
    lons_max = np.max(lons_array)
    lons_min = np.min(lons_array)
    lats_max = np.max(lats_array)
    lats_min = np.min(lats_array)

    #lons_lats_array = np.empty((lons_array.size + lats_array.size), dtype=int)
    lons_lats_array = numpy.zeros([lons_array.size + lats_array.size], dtype='float64')
    if cleanup_geometry:
        if cat_id == 'cat-39990' or cat_id == 'cat-39965':
            lons_lats_array[0::2] = lats_array    #lons_array -> lats_array for cat-39990, cat-39965
            lons_lats_array[1::2] = lons_array    #lats_array -> lons_array for cat-39990, cat-39965
        else:
            lons_lats_array[0::2] = lons_array
            lons_lats_array[1::2] = lats_array

    nodeCoord = lons_lats_array

    polygon = Polygon(poly_coords)
    nodeOwner = np.zeros(num_node)

    #build up the polygon by forming connection between successive vertices
    #no need to connect the last vertex with the first, otherwise it will give an error
    elem_conn = []
    elem_coord = []
    i = 0
    for nid in node_id:
        elem_conn1 = node_id[i]
        elem_conn.append(elem_conn1)
        i += 1

    elem_coordx = np.sum(lons_array)/num_node
    elem_coordy = np.sum(lats_array)/num_node
    elem_coord.append(elem_coordx)
    elem_coord.append(elem_coordy)

    elemConn = np.array(elem_conn)
    elemCoord = np.array(elem_coord)

    elem_type = []
    elem_type.append(num_node)

    #for polygon with vertices greater than 4, elemType is a numerical value
    elemType=np.array(elem_type)

    mesh.add_nodes(num_node,nodeId,nodeCoord,nodeOwner)

    mesh.add_elements(num_elem,elemId,elemType,elemConn, element_coords=elemCoord)

    return mesh, lons_min, lons_max, lats_min, lats_max, polygon


def get_cat_geometry(aorcfile, lons_min, lons_max, lats_min, lats_max):
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

def read_sub_netcdf(cat_id, datafile, var_name_list, var_value_list, lons_min_grid, lons_max_grid, lats_min_grid, lats_max_grid, lons_delta, lats_delta, polygon):
    """
    Extract a sebset netcdf from the large original netcdf file
    """
    # the reading process. read_sub_netcdf is currently the slowest process as the input netcdf file is too large
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
    # Populate a 3D landmask array, where 1=has value, 0=no value
    # the first dimension size is 1, corresponding to current time, to conform to the variables' dimension in write_sub_netcdf function
    # x_coord_local = x * lons_delta                          # x is indexed for this local sub_area, x_coord_local is relative to lons_min
    # x_coord = x_coord_local + lons_min_grid * lons_delta    # x_coord is relative to the first grid position in the input AORC netcdf forcing file
                                                              # where the first grid point corresponds to longitude = -130 (for huc01)
    # x_coord_acual = x_coord + lons_first                    # this is the actual global longtitudinal coordinate that x corresponds to in catchment_data.geojson
    # similarly for y: latitude
    landmask = numpy.zeros([1, nlats, nlons], dtype='int')
    for y in range(nlats):
        for x in range(nlons):
            x_coord_local = x * lons_delta
            x_coord = x_coord_local + lons_min_grid * lons_delta
            x_coord_global = x_coord + lons_first
            y_coord_local = y * lats_delta
            y_coord = y_coord_local + lats_min_grid * lats_delta
            y_coord_global = y_coord + lats_first
            this_point = Point(x_coord_global, y_coord_global)
            if this_point.within(polygon):
                landmask[0,y,x] = 1 # Populate a 2D landmask array, where 1=land and 0=ocean

    Var_array = []
    for var_name in var_name_list:
        Var_array.append([])

    for i in range(len(var_name_list)):
        Var_array[i] = numpy.array(var_value_list[i])

    #using mask to calculate the average value of state variables
    avg_var_value = []
    for var_name in var_name_list:
        avg_var_value.append([])

    for i in range(len(var_name_list)):
        avg_var_value[i] = 0.0
        num_ones = 0
        for y in range(nlats):
            for x in range(nlons):
                if i > 2:
                    if landmask[0,y,x] == 1:
                        num_ones += 1
                        avg_var_value[i] += Var_array[i][0][y,x] * landmask[0,y,x]
        if i > 2:
            if num_ones != 0:
                avg_var_value[i] /= num_ones
            else:
                print("no grid point inside the polygon, the catchment is too small")
                print("landmask =\n {}".format(landmask))
                sys.exit()

    date_time = get_date_time(datafile)
    with open(avg_outfile, 'a') as wfile:
        out_data = "{}".format(cat_id)
        for i in range(len(var_name_list)):
            if i == 0:
                out_data += ",{}".format(Var_array[i][0])
            if i > 2:
                out_data += ",{}".format(avg_var_value[i])
        wfile.write(out_data+'\n')
    wfile.close()

    return var_value_list, Var_array, landmask, nlats, nlons, ds


def write_sub_netcdf(filename_out, var_name_list, var_value_list, Var_array, landmask, nlats, nlons, ds):
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

    time_out = ncfile_out.createVariable('time', 'double', ('time',), fill_value=-99999)
    lat_out = ncfile_out.createVariable('lat', 'double', ('lat',), fill_value=-99999)
    lon_out = ncfile_out.createVariable('lon', 'double', ('lon',), fill_value=-99999)
    #landmask_out = ncfile_out.createVariable('landmask', 'i', ('time', 'lat', 'lon',), fill_value=-99999)
    APCP_surface_out = ncfile_out.createVariable('APCP_surface', 'f4', ('time', 'lat', 'lon',), fill_value=-99999)
    DLWRF_surface_out = ncfile_out.createVariable('DLWRF_surface', 'f4', ('time', 'lat', 'lon',), fill_value=-99999)
    DSWRF_surface_out = ncfile_out.createVariable('DSWRF_surface', 'f4', ('time', 'lat', 'lon',), fill_value=-99999)
    PRES_surface_out = ncfile_out.createVariable('PRES_surface', 'f4', ('time', 'lat', 'lon',), fill_value=-99999)
    SPFH_2maboveground_out = ncfile_out.createVariable('SPFH_2maboveground', 'f4', ('time', 'lat', 'lon',), fill_value=-99999)
    TMP_2maboveground_out = ncfile_out.createVariable('TMP_2maboveground', 'f4', ('time', 'lat', 'lon',), fill_value=-99999)
    UGRD_10maboveground_out = ncfile_out.createVariable('UGRD_10maboveground', 'f4', ('time', 'lat', 'lon',), fill_value=-99999)
    VGRD_10maboveground_out = ncfile_out.createVariable('VGRD_10maboveground', 'f4', ('time', 'lat', 'lon',), fill_value=-99999)
    landmask_out = ncfile_out.createVariable('landmask', 'i', ('time', 'lat', 'lon',), fill_value=-99999)
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

    setattr(landmask_out, 'description', '1=in polygon 0=outside polygon')

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
    landmask_out[:,:,:] = landmask[:,:,:]
    ncfile_out.close()

def grid_to_mesh_regrid(cat_id, lons_start, lats_start, lons_min, lons_max, lats_min, lats_max, polygon, esmf_outfile, mesh,
                        lats_min_grid, lats_max_grid, lats_delta, lons_min_grid, lons_max_grid, lons_delta, iter_k):
    """
    Regrid a rectangular area to a polygon generated in ESMF.Mesh
    See read_write_weight_file.py in the examples of ESMPY
    """
    datafile_in = join(input_root, netcdf_files, "AORC_"+cat_id+".nc")

    # Create the source grid from memory with no periodic dimension.
    [lat,lon] = [1,0]

    # These values are obtained from the original input AORC file: AORC-OWP_2012063023z.nc4
    # they have now been generalized
    #for huc01
    #lats_start = 20.0
    #lons_start = -130.0
    #for sugar_creek
    #lats_start = 34.49942
    #lons_start = -81.5102730000392

    lats_upper = lats_start + lats_max_grid * lats_delta
    lats_lower = lats_start + lats_min_grid * lats_delta
    lons_upper = lons_start + lons_max_grid * lons_delta
    lons_lower = lons_start + lons_min_grid * lons_delta

    lats  = numpy.arange(lats_lower, lats_upper, lats_delta)
    lons = numpy.arange(lons_lower, lons_upper, lons_delta)
    max_index = numpy.array([lons.size, lats.size])

    srcgrid = ESMF.Grid(max_index, 
                        coord_sys=ESMF.CoordSys.SPH_DEG,
                        staggerloc=ESMF.StaggerLoc.CENTER,
                        coord_typekind=ESMF.TypeKind.R4,
                        num_peri_dims=0, periodic_dim=0, pole_dim=0)

    gridCoordLat = srcgrid.get_coords(lat)
    gridCoordLon = srcgrid.get_coords(lon)

    lons_par = lons[srcgrid.lower_bounds[ESMF.StaggerLoc.CENTER][0]:srcgrid.upper_bounds[ESMF.StaggerLoc.CENTER][0]]
    lats_par = lats[srcgrid.lower_bounds[ESMF.StaggerLoc.CENTER][1]:srcgrid.upper_bounds[ESMF.StaggerLoc.CENTER][1]]

    # make sure to use indexing='ij' as ESMPy backend uses matrix indexing (not Cartesian)
    lonm, latm = numpy.meshgrid(lons_par, lats_par, indexing='ij')

    gridCoordLon[:] = lonm
    gridCoordLat[:] = latm

    # Create a field on the centers of the source grid with the mask applied.
    srcfield = ESMF.Field(srcgrid, name="srcfield", staggerloc=ESMF.StaggerLoc.CENTER)

    gridLon = srcfield.grid.get_coords(lon, ESMF.StaggerLoc.CENTER)
    gridLat = srcfield.grid.get_coords(lat, ESMF.StaggerLoc.CENTER)

    #srcfield_tmp = srcfield.data[:,:]
    #print("type of srcfield.data = {}".format(type(srcfield_tmp)))    #<class 'numpy.ndarray'>

    dstfield = ESMF.Field(mesh, name='dstfield', meshloc=ESMF.MeshLoc.ELEMENT)
    xctfield = ESMF.Field(mesh, name='xctfield', meshloc=ESMF.MeshLoc.ELEMENT)
    dstfield.data[:] = 2.0

    weight_file = join(input_root, weight_files, "weight_file_"+cat_id+".nc")

    if iter_k == 0:
        if os.path.exists(weight_file):
            os.remove(weight_file)

        regrid = ESMF.Regrid(srcfield, dstfield, filename=weight_file, 
                             #regrid_method=ESMF.RegridMethod.BILINEAR, 
                             regrid_method=ESMF.RegridMethod.PATCH,
                             unmapped_action=ESMF.UnmappedAction.IGNORE)
        regrid.destroy()

    # process the srcfield data
    srcfield_vars = []
    srcfield_vars.append('APCP_surface')
    srcfield_vars.append('DLWRF_surface')
    srcfield_vars.append('DSWRF_surface')
    srcfield_vars.append('PRES_surface')
    srcfield_vars.append('SPFH_2maboveground')
    srcfield_vars.append('TMP_2maboveground')
    srcfield_vars.append('UGRD_10maboveground')
    srcfield_vars.append('VGRD_10maboveground')

    #get input datafile_in atributes: scale_factor, offset
    ds1 = nc4.Dataset(datafile_in)
   
    add_offset = numpy.zeros([len(srcfield_vars)])
    scale_factor = numpy.zeros([len(srcfield_vars)])
    i = 0
    for key in srcfield_vars:
        scale_factor[i] = ds1.variables[key].scale_factor
        try:
            add_offset[i] = ds1.variables[key].add_offset
        except AttributeError as e:
            add_offset[i] = 0.0
        i += 1

    # read in the weight file
    ds = nc4.Dataset(weight_file)

    weight = ds['S']
    col = ds['col']
    row = ds['row']

    time = ds1.variables['time'][0]
    with open(esmf_outfile, 'a') as wfile:
        for m in range(1):
            time = ds1.variables['time'][m]
            out_data = "{},{}".format(cat_id, time)
            for i in range(len(srcfield_vars)):
                srcfield.read(filename=datafile_in, variable=srcfield_vars[i], timeslice=m+1)
                srcfield_tmp = srcfield.data[:,:]
                #ESMF matrix uses Fortran convention
                flat_array = srcfield_tmp.flatten(order='F')

                # calculate the weighted average
                sum = 0.0
                for k in range(len(col)):
                    l = col[k] - 1
                    sum += weight[k] * flat_array[l]
                #take into account the scale_factor and offset in the input netcdf datafile
                average = sum * scale_factor[i] + add_offset[i]
                if i == 0 and average < 0:    # i == 0 corresponds srcfield_vars for APCP_surface
                    average = 0.0
                out_data += ",{}".format(average)
            wfile.write(out_data+'\n')
    wfile.close()

    return srcfield.data, lons_par, lats_par

def csv_to_netcdf(num_catchments, num_time, date_time, aorc_ncfile, regrid_weight_forcing):
    """
    Convert from csv to netcdf format
    """
    #get the input csv file and create output netcdf file name
    if regrid_weight_forcing:
        csv_infile = join(input_root, "esmf_output.csv")
        output_path = join(output_root, forcing, "huc01_forcing_"+date_time+".nc")
    else:
        csv_infile = join(input_root, mask_avg, "avg_output_"+date_time+".csv")
        output_path = join(output_root, mask_avg, "huc01_forcing_"+date_time+".nc")

    df = pd.read_csv(csv_infile)
    df = df.sort_values(['time', 'id'])
    variable_names = df.columns

    #make the data set
    filename = output_path
    filename_out = output_path

    print("num_catchments = {}".format(num_catchments))
    cat_id = numpy.zeros(num_catchments, dtype=object)
    print("cat_id.size = {}".format(cat_id.size))
    time = []

    apcp_surface = []
    dlwrf_surface = []
    dswrf_surface = []
    pres_surface = []
    spfh_2maboveground = []
    tmp_2maboveground = []
    ugrd_10maboveground = []
    vgrd_10maboveground = []

    #initialize temporary list
    apcp_tmp = []
    dlwrf_tmp = []
    dswrf_tmp = []
    pres_tmp = []
    spfh_tmp = []
    tmp_tmp = []
    ugrd_tmp = []
    vgrd_tmp = []

    #save data into list of list
    df = df.reset_index()  # make sure indexes pair with number of rows
    i = 0
    for index, row in df.iterrows():
        cat_id[i] = row['id']
        if index < num_time:
            time.append(row['time'])
        i += 1
        apcp_tmp.append(row['APCP_surface'])
        dlwrf_tmp.append(row['DLWRF_surface'])
        dswrf_tmp.append(row['DSWRF_surface'])
        pres_tmp.append(row['PRES_surface'])
        spfh_tmp.append(row['SPFH_2maboveground'])
        tmp_tmp.append(row['TMP_2maboveground'])
        ugrd_tmp.append(row['UGRD_10maboveground'])
        vgrd_tmp.append(row['VGRD_10maboveground'])
        #
        apcp_surface.append(apcp_tmp)
        dlwrf_surface.append(dlwrf_tmp)
        dswrf_surface.append(dswrf_tmp)
        pres_surface.append(pres_tmp)
        spfh_2maboveground.append(spfh_tmp)
        tmp_2maboveground.append(tmp_tmp)
        ugrd_10maboveground.append(ugrd_tmp)
        vgrd_10maboveground.append(vgrd_tmp)
        apcp_tmp = []
        dlwrf_tmp = []
        dswrf_tmp = []
        pres_tmp = []
        spfh_tmp = []
        tmp_tmp = []
        ugrd_tmp = []
        vgrd_tmp = []

    # Convert to numpy array
    time = numpy.array(time)
    APCP_surface = numpy.array(apcp_surface)
    DLWRF_surface = numpy.array(dlwrf_surface)
    DSWRF_surface = numpy.array(dswrf_surface)
    PRES_surface = numpy.array(pres_surface)
    SPFH_2maboveground = numpy.array(spfh_2maboveground)
    TMP_2maboveground = numpy.array(tmp_2maboveground)
    UGRD_10maboveground = numpy.array(ugrd_10maboveground)
    VGRD_10maboveground = numpy.array(vgrd_10maboveground)

    # write data to netcdf files
    filename_out = output_path
    ncfile_out = nc4.Dataset(filename_out, 'w', format='NETCDF4')

    #add the dimensions
    time_dim = ncfile_out.createDimension('time', None)
    catchment_id_dim = ncfile_out.createDimension('catchment-id', num_catchments)
    string_dim =ncfile_out.createDimension('str_dim', 1)
  
    # create variables
    cat_id_out = ncfile_out.createVariable('ids', 'str', ('catchment-id'), fill_value="None")
    time_out = ncfile_out.createVariable('Time', 'double', ('time',), fill_value=-99999)
    APCP_surface_out = ncfile_out.createVariable('RAINRATE', 'f4', ('catchment-id', 'time',), fill_value=-99999)
    TMP_2maboveground_out = ncfile_out.createVariable('T2D', 'f4', ('catchment-id', 'time',), fill_value=-99999)
    SPFH_2maboveground_out = ncfile_out.createVariable('Q2D', 'f4', ('catchment-id', 'time',), fill_value=-99999)
    UGRD_10maboveground_out = ncfile_out.createVariable('U2D', 'f4', ('catchment-id', 'time',), fill_value=-99999)
    VGRD_10maboveground_out = ncfile_out.createVariable('V2D', 'f4', ('catchment-id', 'time',), fill_value=-99999)
    PRES_surface_out = ncfile_out.createVariable('PSFC', 'f4', ('catchment-id', 'time',), fill_value=-99999)
    DSWRF_surface_out = ncfile_out.createVariable('SWDOWN', 'f4', ('catchment-id', 'time',), fill_value=-99999)
    DLWRF_surface_out = ncfile_out.createVariable('LWDOWN', 'f4', ('catchment-id', 'time',), fill_value=-99999)

    #set output netcdf file atributes
    ds = nc4.Dataset(aorc_ncfile)
    varout_dict = {'time':time_out,
                   'APCP_surface':APCP_surface_out, 'DLWRF_surface':DLWRF_surface_out, 'DSWRF_surface':DSWRF_surface_out,
                   'PRES_surface':PRES_surface_out, 'SPFH_2maboveground':SPFH_2maboveground_out, 'TMP_2maboveground':TMP_2maboveground_out,
                   'UGRD_10maboveground':UGRD_10maboveground_out, 'VGRD_10maboveground':VGRD_10maboveground_out}

    #copy all attributes from input netcdf file
    for name, variable in ds.variables.items():
        if name == 'latitude' or name == 'longitude':
            pass
        else:
            varout_name = varout_dict[name]
            for attrname in variable.ncattrs():
                if attrname != '_FillValue':
                    setattr(varout_name, attrname, getattr(variable, attrname))

    #drop the scale_factor from the output netcdf forcing file attributes
    for key, varout_name in varout_dict.items():
        if key != 'time':
            del varout_name.scale_factor

    #set attributes for additional variables
    setattr(cat_id_out, 'description', 'catchment_id')

    cat_id_out[:] = cat_id[:]
    time_out[:] = time[:]
    APCP_surface_out[:,:] = APCP_surface[:,:]
    DLWRF_surface_out[:,:] = DLWRF_surface[:,:]
    DSWRF_surface_out[:,:] = DSWRF_surface[:,:]
    PRES_surface_out[:,:] = PRES_surface[:,:]
    SPFH_2maboveground_out[:,:] = SPFH_2maboveground[:,:]
    TMP_2maboveground_out[:,:] = TMP_2maboveground[:,:]
    UGRD_10maboveground_out[:,:] = UGRD_10maboveground[:,:]
    VGRD_10maboveground_out[:,:] = VGRD_10maboveground[:,:]
     
    ncfile_out.close()

def process_sublist(data : dict, lock: Lock, num: int):
    num_inputs = len(data["g_sublist"])
    datafile = data["datafile"]
    aorcfile = datafile
    iter_k = data["iter"]

    for i in range(num_inputs):
        #extract data
        pos = data["offsets"][i]
        g_sublist = data["g_sublist"][i]

        #extract catchment geometry
        x,y = g_sublist.exterior.coords.xy
        all_coords = np.dstack((x,y))
        coord_list = all_coords[0]
        cat_id = data["cat_ids"][i]

        #create polygon mesh
        (mesh, lons_min, lons_max, lats_min, lats_max, polygon) = mesh_create_polygon(coord_list, cat_id, cleanup_geometry)

        #define the boundary for extract sub-netcdf data
        lats_min_grid, lats_max_grid, lats_delta, lats_first, lons_min_grid, lons_max_grid, lons_delta, lons_first = get_cat_geometry(aorcfile,
        lons_min, lons_max, lats_min, lats_max)

        var_name_list = ['time', 'latitude', 'longitude', 'APCP_surface', 'DLWRF_surface', 'DSWRF_surface', 'PRES_surface', 'SPFH_2maboveground',
                         'TMP_2maboveground', 'UGRD_10maboveground', 'VGRD_10maboveground']

        var_value_list = []
        for var_name in var_name_list:
            var_value_list.append([])

        #extract sub-netcdf data
        var_value_list, Var_array, landmask, nlats, nlons, ds = read_sub_netcdf(cat_id, datafile, var_name_list, var_value_list,
                                                                  lons_min_grid, lons_max_grid, lats_min_grid, lats_max_grid,
                                                                  lons_delta, lats_delta, polygon)

        #write to netcdf file for the cat_id
        filename_out = join(output_root, netcdf_files, "AORC_"+cat_id+".nc")

        write_sub_netcdf(filename_out, var_name_list, var_value_list, Var_array, landmask, nlats, nlons, ds)

        #perform regridding, generate weight file, and calculate average for cat_id
        (srcfield_data, lons_par, lats_par) = grid_to_mesh_regrid(cat_id, lons_first, lats_first, lons_min, lons_max, lats_min, lats_max, polygon, esmf_outfile, mesh,
                                                                  lats_min_grid, lats_max_grid, lats_delta, lons_min_grid, lons_max_grid, lons_delta, iter_k)


def plot_srcfield(srcfield_data, lons_par, lats_par):
    """
    plot the property in colored spectrum for a sub-area
    """
    if ESMF.local_pet() == 0:
        print(type(srcfield_data))
        print(srcfield_data)
        print(srcfield_data.T)
        import matplotlib.pyplot as plt
        fig = plt.figure(1, (15, 6))
        fig.suptitle('ESMPy Grids', fontsize=14, fontweight='bold')

        ax = fig.add_subplot(1, 2, 1)
        im = ax.imshow(srcfield_data.T, vmin=0.012, vmax=0.0132, cmap='gist_ncar', aspect='auto',
                       extent=[min(lons_par), max(lons_par), min(lats_par), max(lats_par)])
        ax.set_xbound(lower=min(lons_par), upper=max(lons_par))
        ax.set_ybound(lower=min(lats_par), upper=max(lats_par))
        ax.set_xlabel("Longitude")
        ax.set_ylabel("Latitude")
        ax.set_title("SPFH_2maboveground")

        #ax = fig.add_subplot(1, 2, 2)
        #im = ax.imshow(dstfield.data, vmin=1, vmax=3, cmap='gist_ncar', aspect='auto',
        #extent=[min(lons_par), max(lons_par), min(lats_par), max(lats_par)])
        #ax.set_xlabel("Longitude")
        #ax.set_ylabel("Latitude")
        #ax.set_title("Regrid Solution")

        fig.subplots_adjust(right=0.8)
        cbar_ax = fig.add_axes([0.9, 0.1, 0.01, 0.8])
        fig.colorbar(im, cax=cbar_ax)

        #plt.show()
        plt.savefig("srcfield_SPFH.png")
        print("Finished plotting figure")


if __name__ == '__main__':
    #parse the input and output root directory
    #example: python code_name -i /local/esmpy/huc01 -o /local/esmpy/huc01 -a aorc_netcdf_sub/ -w weight_files/ -f forcing/ -m mask_avg/ -n netcdf_files/ -c 1
    #for huc01: option "-c 1" for current hydrofabric where some defect exist
    #for completely correct hydrofabric, use option "-c 0"
    parser = argparse.ArgumentParser()
    parser.add_argument("-i", dest="input_root", type=str, required=True, help="The input directory with csv files")
    parser.add_argument("-o", dest="output_root", type=str, required=True, help="The output file path")
    parser.add_argument("-a", dest="aorc_netcdf", type=str, required=True, help="The input aorc netcdf files directory")
    parser.add_argument("-w", dest="weight_files", type=str, required=True, help="The output weight files sub_dir")
    parser.add_argument("-f", dest="forcing", type=str, required=True, help="The output forcing files sub_dir")
    parser.add_argument("-m", dest="mask_avg", type=str, required=True, help="The output forcing calculated using mask sub_dir")
    parser.add_argument("-n", dest="netcdf_files", type=str, required=True, help="The netcdf files directory")
    parser.add_argument("-c", dest="cleanup_geometry", type=int, required=True, help="The logic variable for clean up abnormal catchment geometry")
    args = parser.parse_args()

    #retrieve parsed values
    input_root = args.input_root
    output_root = args.output_root
    aorc_netcdf = args.aorc_netcdf
    weight_files = args.weight_files
    forcing = args.forcing
    mask_avg = args.mask_avg
    netcdf_files = args.netcdf_files
    cleanup_geometry = args.cleanup_geometry

    if cleanup_geometry == 0:
        cleanup_geometry = False
    else:
        cleanup_geometry = True

    #generate catchment geometry from hydrofabric 
    hyfabfile = "/local/ngen/data/huc01/huc_01/hydrofabric/spatial/catchment_data.geojson"
    cat_df_full = gpd.read_file(hyfabfile)
    g = [i for i in cat_df_full.geometry]
    h = [i for i in cat_df_full.id]
    n_cats = len(g)
    print("number of catchments = {}".format(n_cats))

    cat_dict = {}
    for i in range(n_cats):
        if h[i] == "cat-39990":
            cat_dict.update({"cat-39990":i})
            print("for cat-39990, list index = {}".format(i))
        if h[i] == "cat-39965":
            cat_dict.update({"cat-39965":i})
            print("for cat-39965, list index = {}".format(i))

    datafile_path = join(input_root, aorc_netcdf, "AORC-OWP_*.nc4")
    #datafile_path = join(input_root, aorc_netcdf, "AORC-OWP_2012063005z.nc4")
    datafiles = glob.glob(datafile_path)
    print("number of forcing files = {}".format(len(datafiles)))
    #process data with time ordered
    datafiles.sort()

    # some function only executed once at the beginning (k = 0)
    k = 0
    for datafile in datafiles:
        if k == 0:
            aorc_ncfile = datafile    #save a file to get attributes
        esmf_outfile = join(output_root, "esmf_output.csv")
        if os.path.exists(esmf_outfile):
            os.remove(esmf_outfile)
        else:
            print("The esmf_outfile does not exist")
        with open(esmf_outfile, 'w') as wfile:
            out_header = "id,time,APCP_surface,DLWRF_surface,DSWRF_surface,PRES_surface,SPFH_2maboveground,TMP_2maboveground,UGRD_10maboveground,VGRD_10maboveground"
            wfile.write(out_header+'\n')
        wfile.close()

        #create output file for data calculated from averaging using landmask
        date_time = get_date_time(datafile)
        print("processing forcing file for date_time = {}".format(date_time))
        avg_outfile = join(output_root, mask_avg, "avg_output_"+date_time+".csv")
        if os.path.exists(avg_outfile):
            os.remove(avg_outfile)
        else:
            print("The avg_outfile does not exist")
        with open(avg_outfile, 'w') as wfile:
            out_header = "id,time,APCP_surface,DLWRF_surface,DSWRF_surface,PRES_surface,SPFH_2maboveground,TMP_2maboveground,UGRD_10maboveground,VGRD_10maboveground"
            wfile.write(out_header+'\n')
        wfile.close()

        #prepare for processing
        #num_csv_inputs = len(csv_files)
        num_csv_inputs = len(g)
        num_processes = 25

        #generate the data objects for child processes
        g_groups = np.array_split(np.array(g), num_processes)
        cat_groups = np.array_split(np.array(h), num_processes)
        pos_groups = np.array_split(np.array(range(n_cats)), num_processes)

        process_data = []
        process_list = []
        lock = Lock()

        for i in range(num_processes):
            # fill the dictionary with needed at
            data = {}
            data["g_sublist"] = g_groups[i].tolist()
            data["cat_ids"] = cat_groups[i]
            data["offsets"] = pos_groups[i]
            data["datafile"] = datafile
            data["iter"] = k

            #append to the list
            process_data.append(data)

            p = Process(target=process_sublist, args=(data, lock, i))

            process_list.append(p)

        #start all processes
        for p in process_list:
            p.start()

        #wait for termination
        for p in process_list:
            p.join()

        num_catchments = n_cats

        #write a netcdf file every time step the same as the input file
        ntime = 1
        date_time = get_date_time(datafile)

        #generate netcdf file from rigrid generated weight file
        regrid_weight_forcing = True
        csv_to_netcdf(num_catchments, ntime, date_time, aorc_ncfile, regrid_weight_forcing)

        #generate netcdf file using the calculated weight
        regrid_weight_forcing = False
        csv_to_netcdf(num_catchments, ntime, date_time, aorc_ncfile, regrid_weight_forcing)
        k += 1

