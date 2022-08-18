import numpy as np
import geopandas as gpd
import netCDF4 as nc4
import os
import glob
from pathlib import Path
from os.path import join
import argparse
import pandas as pd
from  multiprocessing import Process, Lock
import multiprocessing
import time
import datetime
import re

def get_date_time_csv(path):
    """
    Extract the date-time from the file path
    """
    path = Path(path)
    name = path.stem
    date_time = name.split('.')[0]
    date_time = date_time.split('_')[2]  #this index may depend on the naming format of the forcing data
    date_time = re.sub('\D','',date_time)
    return date_time

def get_date_time(path):
    """
    Extract the date-time from the file path
    """
    path = Path(path)
    name = path.stem
    date_time = name.split('.')[0]
    date_time = date_time.split('_')[1]  #this index may depend on the naming format of the forcing data
    date_time = re.sub('\D','',date_time)
    return date_time

def csv_to_netcdf(num_catchments, weighted_csv_files, aorc_ncfile):
    """
    Convert from csv to netcdf format
    """

    # first read AORC metadata in and extract scale factors and offsets if needed
    ds = nc4.Dataset(aorc_ncfile)

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

    # get scale_factor and offset keys if available
    # (AORC-OWP files for HUC01 scenario has this metadata)
    add_offset = np.zeros([len(srcfield_vars)])
    scale_factor = np.zeros([len(srcfield_vars)])
    i = 0
    for key in srcfield_vars:
        try:
            scale_factor[i] = ds.variables[key].scale_factor
        except AttributeError as e:
            scale_factor[i] = 1.0
        try:
            add_offset[i] = ds.variables[key].add_offset
        except AttributeError as e:
            add_offset[i] = 0.0
        i += 1


    # pre-allocate arrays for netcdf files

    num_files = len(weighted_csv_files)


    print("num_catchments = {}".format(num_catchments))
    cat_id = np.zeros(num_catchments, dtype=str)
    print("num_time_steps = {}".format(num_files))


    APCP_surface = np.zeros((num_catchments,num_files), dtype=float)
    DLWRF_surface = np.zeros((num_catchments,num_files), dtype=float)
    DSWRF_surface = np.zeros((num_catchments,num_files), dtype=float)
    PRES_surface = np.zeros((num_catchments,num_files), dtype=float)
    SPFH_2maboveground = np.zeros((num_catchments,num_files), dtype=float)
    TMP_2maboveground = np.zeros((num_catchments,num_files), dtype=float)
    UGRD_10maboveground = np.zeros((num_catchments,num_files), dtype=float)
    VGRD_10maboveground = np.zeros((num_catchments,num_files), dtype=float)



    # get catchment ids and reference start time for NextGen forcing file
    df = pd.read_csv(weighted_csv_files[0])
    cat_id[:] = df['id'].values
    start_time = get_date_time(weighted_csv_files[0])

    # AORC reference time to use
    ref_date = pd.Timestamp("1970-01-01 00:00:00")

    # Loop over ech ExactExtract csv file, read and append data to netcdf arrays
    for i in np.arange(num_files):
        file_date = get_date_time_csv(weighted_csv_files[i])
        time[i] = (pd.Timestamp(datetime.datetime.strptime(file_date,'%Y%m%d%H')) - ref_date).total_seconds()
        df = pd.read_csv(weighted_csv_files[i])
        #since ExactExtract module doesn't account for scale_factor and offset
        # keys for a given variable, we will have to finish calculating the lumped
        # sum for the ExactExtract csv files
        df['APCP_surface_weighted_mean'] = df['APCP_surface_weighted_mean']*scale_factor[0] + add_offset[0]
        df['DLWRF_surface_weighted_mean'] = df['DLWRF_surface_weighted_mean']*scale_factor[1] + add_offset[1]
        df['DSWRF_surface_weighted_mean'] = df['DSWRF_surface_weighted_mean']*scale_factor[2] + add_offset[2]
        df['PRES_surface_weighted_mean'] = df['PRES_surface_weighted_mean']*scale_factor[3] + add_offset[3]
        df['SPFH_2maboveground_weighted_mean'] = df['SPFH_2maboveground_weighted_mean']*scale_factor[4] + add_offset[4]
        df['TMP_2maboveground_weighted_mean'] = df['TMP_2maboveground_weighted_mean']*scale_factor[5] + add_offset[5]
        df['UGRD_10maboveground_weighted_mean'] = df['UGRD_10maboveground_weighted_mean']*scale_factor[6] + add_offset[6]
        df['VGRD_10maboveground_weighted_mean'] = df['VGRD_10maboveground_weighted_mean']*scale_factor[7] + add_offset[7]

        # Now add the ExactExtract csv data to netcdf variables
        APCP_surface[:,i] = df['APCP_surface_weighted_mean'].values
        DLWRF_surface[:,i] = df['DLWRF_surface_weighted_mean'].values
        DSWRF_surface[:,i] = df['DSWRF_surface_weighted_mean'].values
        PRES_surface[:,i] = df['PRES_surface_weighted_mean'].values
        SPFH_2maboveground[:,i] = df['SPFH_2maboveground_weighted_mean'].values
        TMP_2maboveground[:,i] = df['TMP_2maboveground_weighted_mean'].values
        UGRD_10maboveground[:,i] = df['UGRD_10maboveground_weighted_mean'].values
        VGRD_10maboveground[:,i] = df['VGRD_10maboveground_weighted_mean'].values

        # save csv file with updated data after accounting for
        # scale factor and offset within netcdf metadata
        df.to_csv(weighted_csv_files[i],header=True,index=False)

    #create output netcdf file name
    output_path = join(output_root, forcing, "NextGen_forcing_"+start_time+".nc")

    #make the data set
    filename = output_path
    filename_out = output_path


    # write data to netcdf files
    filename_out = output_path
    ncfile_out = nc4.Dataset(filename_out, 'w', format='NETCDF4')

    #add the dimensions
    time_dim = ncfile_out.createDimension('time', num_files)
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
            try:
                del varout_name.scale_factor
            except:
                print("No scale factor in forcing files. No keys to tweak for output netcdf")

    #####################################################################

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

    # Now close NextGen netcdf file
    # and AORC file
    ncfile_out.close()
    ds.close()


def process_sublist(data : dict, lock: Lock, num: int):
    num_files = len(data["forcing_files"])


    for i in range(num_files):
        # extract forcing file
        aorc_file = data["forcing_files"][i]
        # get datetime of forcing file to append
        # to ExactExtract csv output file
        date_time = get_date_time(aorc_file)
        NextGen_csv = join(exactextract_files,'NextGen_forcings_'+str(date_time)+'.csv')
        # create string for ExactExtract module output file
        exactextract_output_file = ' -o ' + NextGen_csv
        # Since we now have the pathway to the location file
        # We can go ahead and finish the ExactExtract command
        # string, which specifies which variables to call for
        # this particular AORC file
        file_vars = ''
        for i in np.arange(len(AORC_met_vars)):
           file_vars += ' -r "'+AORC_met_vars[i]+':NETCDF:'+aorc_file+':'+AORC_met_vars[i]+'"'

        ''''''''''''''''''''''''''''
    Example call for ExactExtract Module
    exactextract -r "VGRD_10maboveground:NETCDF:AORC_Charlotte_2015121309.nc4:VGRD_10maboveground" -p catchment_data.geojson -f id -s "weighted_mean(VGRD_10maboveground)" -o NextGen_VGRD_Lumped_Sum_Forcings.csv
    '''''''''''''''''''''''''''

        # Call ExactExtract Module to produce NextGen csv weighted lumped sum data
        os.system(exactextract_executable + file_vars + shapefile + shapefile_variable + avg_variable_technique + exactextract_output_file)


if __name__ == '__main__':

    #example: python code_name -i /home/jason.ducker/esmf_forcing_files_test/ExactExtract_sugar_creek -o /home/jason.ducker/esmf_forcing_files_test/ExactExtract_sugar_creek -a /apd_common/test/test_data/aorc_netcdf/AORC/2015 -w weight_files/ -f forcing_files/ -e_csv csv_files/ -e /home/jason.ducker/ExactExtract/build/exactextract

    #parse the input and output root directory
    parser = argparse.ArgumentParser()
    parser.add_argument("-i", dest="input_root", type=str, required=True, help="The input directory with csv files")
    parser.add_argument("-o", dest="output_root", type=str, required=True, help="The output file path")
    parser.add_argument("-a", dest="aorc_netcdf", type=str, required=True, help="The input aorc netcdf files directory")
    parser.add_argument("-w", dest="weight_files", type=str, required=True, help="The output weight files sub_dir")
    parser.add_argument("-f", dest="forcing", type=str, required=True, help="The output forcing files sub_dir")
    parser.add_argument("-e_csv", dest="ExactExtract_csv_files", type=str, required=True, help="The output sub_dir for ExactExtract csv files created from each AORC file")
    parser.add_argument("-e", dest="ExactExtract_executable", type=str, required=True, help="The pathway pointing to the ExactExtract executable")
    args = parser.parse_args()

    #retrieve parsed values
    input_root = args.input_root
    output_root = args.output_root
    aorc_netcdf = args.aorc_netcdf
    weight_files = join(output_root,args.weight_files)
    forcing = join(output_root,args.forcing)
    exactextract_files = join(output_root,args.ExactExtract_csv_files)
    exactextract_executable = args.ExactExtract_executable

    #generate catchment geometry from hydrofabric
    #hyfabfile = "/home/jason.ducker/sugar_creek/catchment_data.geojson"
    hyfabfile = "/home/jason.ducker/hydrofabric/huc01/catchment_data.geojson"

    cat_df_full = gpd.read_file(hyfabfile)
    g = [i for i in cat_df_full.geometry]
    h = [i for i in cat_df_full.id]
    n_cats = len(g)
    num_catchments = n_cats
    print("number of catchments = {}".format(n_cats))

    # Data paths for either sugar_creek (AOR_Charlotte) or
    # Huc01 (AORC-OWP) files on Linux clustet
    #datafile_path = join(aorc_netcdf, "AORC_Charlotte_*.nc4")
    datafile_path = join(aorc_netcdf, "AORC-OWP_*.nc4")
    #get list of files
    datafiles = glob.glob(datafile_path)
    print("number of forcing files = {}".format(len(datafiles)))
    #process data with time ordered
    datafiles.sort()

    # Extract variable names from AORC netcdf data
    nc_file = nc4.Dataset(datafiles[0])
    # Get variable list from AORC file
    nc_vars = list(nc_file.variables.keys())
    # Get indices corresponding to Meteorological data
    indices = [nc_vars.index(i) for i in nc_vars if '_' in i]
    # Make array with variable names to use for ExactExtract module
    AORC_met_vars = np.array(nc_vars)[indices]
    # Close netcdf file
    nc_file.close()


    # We want to now go ahead and quickly build part of the system call string
    # for python to call the ExactExtract Module for our given AORC
    # variables. This portion just simply calls what variables to calculate
    # a weighted mean and which shape file we will use
    shapefile = ' -p ' + hyfabfile
    shapefile_variable = ' -f id'

    avg_variable_technique = ''
    for i in np.arange(len(AORC_met_vars)):
        avg_variable_technique += ' -s "weighted_mean('+AORC_met_vars[i]+')"'




    #prepare for processing
    num_forcing_files = len(datafiles)
    num_processes = 25

    #generate the data objects for child processes
    file_groups = np.array_split(np.array(datafiles), num_processes)


    process_data = []
    process_list = []
    lock = Lock()

    for i in range(num_processes):
        # fill the dictionary with needed at
        data = {}
        data["forcing_files"] = file_groups[i]

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


    # Since we have finished creating the ExactExtract csv files,
    # we can now go ahead and call the ExactExtract module once
    # to just extract the weight file for AORC forcing dataset
    weight_csv = join(weight_files,'AORC_weight_file.csv')
    # create string for ExactExtract module output weight file
    exactextract_output_file = ' -o ' + weight_csv
    # create string to request coverage fraction (weight)
    # file from the ExactExtract module
    extract_weights = ' --strategy "coverage_fraction"'
    # Just take the first AORC file to get weights
    aorc_file = datafiles[0]
    # Since we now have the pathway to the location file
    # We can go ahead and finish the ExactExtract command
    # string, which specifies which variables to call for
    # this particular AORC file
    file_vars = ''
    for i in np.arange(len(AORC_met_vars)):
       file_vars += ' -r "'+AORC_met_vars[i]+':NETCDF:'+aorc_file+':'+AORC_met_vars[i]+'"'

    ''''''''''''''''''''''''''''
    Example call for ExactExtract Module requesting weight file
    exactextract -r "VGRD_10maboveground:NETCDF:AORC_Charlotte_2015121309.nc4:VGRD_10maboveground" -p catchment_data.geojson -f id -s "weighted_mean(VGRD_10maboveground)" --strategy "coverage_fraction" -o NextGen_AORC_weights.csv
    '''''''''''''''''''''''''''

    # Call ExactExtract Module to produce weighted csv file for AORC forcings file
    os.system(exactextract_executable + file_vars + shapefile + shapefile_variable + avg_variable_technique + extract_weights + exactextract_output_file)

    # Now get file paths for created ExactExtract csv files
    ExactExtract_path = join(exactextract_files, "*.csv")
    weighted_csv_files = glob.glob(ExactExtract_path)
    print("Number of ExactExtract csv files = {}".format(len(weighted_csv_files)))
    #process data with time ordered
    weighted_csv_files.sort()

    # set aorc netcdf file pathway to get variable attribute data
    aorc_ncfile = datafiles[0]

    #generate single NextGen netcdf file from generated ExactExtract weighted csv files
    csv_to_netcdf(num_catchments, weighted_csv_files, aorc_ncfile)

