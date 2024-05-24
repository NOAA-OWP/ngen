from os import listdir
from os.path import isfile, join
import argparse
import netCDF4 as nc
import pandas as pd
from  multiprocessing import Process, Lock
import numpy as np
import re

def create_netcdf(filename : str, num_catchments : int, variable_names):
    #make the data set
    ds = nc.Dataset(filename, 'w', format='NETCDF4')

    #add the dimensions
    time_dim = ds.createDimension('time', None)
    catchment_id_dim = ds.createDimension('catchment-id', num_catchments)
    string_dim =ds.createDimension('str_dim', 1)

    #add the variables

    ds.createVariable("ids",str,('catchment-id'))

    #add variables from the csv
    for name in variable_names:
        if name.lower() == "time":
            ds.createVariable("Time",'f8',('catchment-id','time'))
            ds["Time"].units = 'ns'
        else:
            ds.createVariable(name,'f4',('catchment-id','time'), chunksizes=(num_catchments,1))

    return ds

def create_partial_netcdf(filename : str, num_catchments : int, variable_names):
    ds = create_netcdf(filename, num_catchments, variable_names)
    ds.createVariable("offset",'i4',('catchment-id'))
    return ds

def add_data(ds : nc.Dataset, pos : int, cat_name : str, df: pd.DataFrame):
    # set the id for this position
    ds['ids'][pos] = cat_name

    # set other variables from csv data
    for var in df.columns:
        var_name = var
        if var.lower() == "time": 
            var_name = "Time"
        ds[var_name][pos,:] = df[var]
    
    # release memory
    del df

def add_partial_data(ds : nc.Dataset, pos : int, offset : int, cat_name : str, df: pd.DataFrame):
    add_data(ds, pos, cat_name, df)
    ds['offset'][pos] = offset

def process_sublist(data : dict, lock: Lock, num: int):
    num_inputs = len(data["csv_files"])
    input_path = data["input_path"]

    #open the netcdf dataset
    ds = None
    first = True

    for i in range(num_inputs):
        #extract data
        csv_file = data["csv_files"][i]
        cat_name = re.sub('(cat-\d+)\D.*','\\1',csv_file)
        pos = data["offsets"][i]
        netcdf_path = data["netcdf_path"]

        #load the csv data
        print("Process ", num, " reading file", csv_file)
        
        df = pd.read_csv(join(input_path,csv_file), parse_dates=[0], na_values=["   nan"])

        if first:
            ds = create_partial_netcdf(netcdf_path + "." + str(num), num_inputs, df.columns)
            first = False

        #update the netcdf with data from this csv
        print("Process", num, " updating netcdf")
        add_partial_data(ds, i, pos, cat_name, df)

    #close the netcdf data
    ds.close()

def merge_partial_files(output_path : str, num_processes : int):
    #open the output path
    ds = nc.Dataset(output_path,"r+")

    #add the contents of each partial file
    for i in range(num_processes):
        #open the partial file
        print("Updating final netcdf with file", i+1, " of ", num_processes)
        part_path = output_path + "." + str(i)
        pds = nc.Dataset(part_path,"r")

        s1 = pds["offset"][0]
        s2 = pds["offset"][-1] + 1

        # use ds.variables so we dont have to see 'offset' again
        for var in ds.variables:
            if var == "ids":
                ds[var][s1:s2] = pds[var][:]
            else:
                ds[var][s1:s2,:] = pds[var][:,:]

        pds.close()

    ds.close



def main():
    #setup the argument parser
    parser = argparse.ArgumentParser()
    parser.add_argument("-i", dest="input_path", type=str, required=True, help="The input directory with csv files")
    parser.add_argument("-o", dest="output_path", type=str, required=True, help="The output file path")
    parser.add_argument("-j", dest="num_processes", type=int, required=False, default=96, help="The number of processes to run in parallel")
    parser.add_argument("-s", dest="skip_csv", action='store_true')
    args = parser.parse_args()

    #retrieve parsed values
    input_path = args.input_path
    output_path = args.output_path
    num_processes = args.num_processes

    #get the list of all input csv files
    csv_files = [f for f in listdir(input_path) if isfile(join(input_path, f)) and f.endswith(".csv")]

    #load the first csv to get the variable names
    df = pd.read_csv(join(input_path,csv_files[0]), na_values=["   nan"])

    #create the output file
    print("Creating output file")
    num_csv_inputs = len(csv_files)
    ds = create_netcdf(output_path, num_csv_inputs, df.columns)
    ds.close()
    print("Creating output created")
    del df

    #generate the data objects for child processes

    if args.skip_csv == False:
        csv_groups = np.array_split(np.array(csv_files), num_processes)
        pos_groups = np.array_split(np.array(range(num_csv_inputs)), num_processes)

        process_data = []
        process_list = []
        lock = Lock()

        print("Setup for parrallel execution")
        for i in range(num_processes):
            # fill the dictionary with needed at
            data = {}
            data["csv_files"] = csv_groups[i]
            data["offsets"] = pos_groups[i]
            data["input_path"] = input_path
            data["netcdf_path"] = output_path

            #append to the list
            process_data.append(data)

            p = Process(target=process_sublist, args=(data, lock, i))

            process_list.append(p)

        #start all processes
        print("Starting processes")
        for p in process_list:
            p.start()

        #wait for termination
        print("Waiting for child processes")
        for p in process_list:
            p.join()

    merge_partial_files(output_path, num_processes)

    


if __name__ == "__main__":
    main()
