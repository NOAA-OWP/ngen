import json
import os
import geopandas as gpd
import numpy as np
from os import listdir
from os.path import isfile, join
import argparse
import pandas as pd
from  multiprocessing import Process, Lock
import multiprocessing
import time

# examples
#input_path = "/local/ngen/data/huc01/huc_01/hydrofabric/spatial/catchment_data.geojson"
#output_dir = "./huc01/polygons"
#n = 14632

def get_catchment_geometry(n, cat_file, output_dir):
    cat_df_full = gpd.read_file(cat_file)
    print(cat_df_full.head(2), "\n")

    g = [i for i in cat_df_full.geometry]
    h = [i for i in cat_df_full.id]
    x,y = g[n].exterior.coords.xy
    all_coords = np.dstack((x,y))

    print("cat-id = {}".format(h[n]))
    cat_geom_file = os.path.join(output_dir, '{}_poly.csv'.format(h[n]))
    print("cat_geom_file = {}".format(cat_geom_file))
    with open (cat_geom_file, 'w') as f:
        k = 0
        f.write("index,lon,lat\n")
        coord_list = all_coords[0]
        for s in coord_list:
            f.write('%d ' %k)
            f.write('%.15f ' %s[0])
            f.write('%.15f ' %s[1])
            f.write("\n")
            k += 1

def process_sublist(data : dict, lock: Lock, num: int):
    num_inputs = len(data["num_cats"])
    input_path = data["input_path"]
    output_dir = data["output_dir"]
    for i in range(num_inputs):
        num_cat = data["num_cats"][i]
        get_catchment_geometry(num_cat, input_path, output_dir)

def main():
    #n = 14632
    #setup the argument parser
    parser = argparse.ArgumentParser()
    parser.add_argument("-i", dest="input_path", type=str, required=True, help="The input file name")
    parser.add_argument("-o", dest="output_dir", type=str, required=True, help="The output files directory")
    parser.add_argument("-n", dest="num_cats", type=int, required=True, help="The number of catchments in the catchment geojson file")
    parser.add_argument("-s", dest="skip_csv", action='store_true')
    args = parser.parse_args()

    #retrieve parsed values
    input_path = args.input_path
    output_dir = args.output_dir
    n = args.num_cats

    num_list = list(range(n))
    num_csv_inputs = len(num_list)
    num_processes = 50
    #generate the data objects for child processes

    csv_groups = np.array_split(np.array(num_list), num_processes)

    process_data = []
    process_list = []
    lock = Lock()

    print("Setup for parrallel execution running {} processes".format(num_processes))
    for i in range(num_processes):
        # fill the dictionary with needed at
        data = {}
        data["num_cats"] = csv_groups[i]
        data["input_path"] = input_path
        data["output_dir"] = output_dir

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

if __name__ == '__main__':
    main()
