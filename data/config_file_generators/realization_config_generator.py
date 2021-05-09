#!/usr/bin/env python
"""
Script to generate a realization configuration JSON file with randomly selected
formulations for each catchment given a catchment data geojson file as input. 

Script and command line arg format:
realization_config_generator.py -i <path and file name to catchment data geojson file>
-o <file name of config output file>

Example:
python realization_config_generator.py -i ../catchment_data.geojson -o realization_config.json

TODO: Determine method to pass each catchment's forcing file to catchment JSON object
"""

import sys, getopt
import geopandas as gpd
import random
import json

#Set the names of the formulations
global_formulation = "tshirt_c"
formulation_1 = "top_model"

#Set params
global_params = {
  "maxsmc": 0.439,
  "wltsmc": 0.066,
  "satdk": 0.00000338,
  "satpsi": 0.355,
  "slope": 1.0,
  "scaled_distribution_fn_shape_parameter": 4.05,
  "multiplier": 0.0,
  "alpha_fc": 0.33,
  "Klf": 0.01,
  "Kn": 0.03,
  "nash_n": 2,
  "Cgw": 0.01,
  "expon": 6.0,
  "max_groundwater_storage_meters": 1.0,
  "nash_storage": [
    0.0,
    0.0
  ],
  "soil_storage_percentage": 0.667,
  "groundwater_storage_percentage": 0.5,
  "timestep": 3600,
  "giuh": {
    "giuh_path": "./test/data/giuh/GIUH.json",
    "crosswalk_path": "./data/crosswalk.json"
  }
}

formulation_1_params = {}

#Set global forcing
global_forcing = {
  "file_pattern": ".*{{id}}.*.csv",
  "path": "./data/forcing/"
}

#TODO: Find method to pass each catchment's forcing to dictionary

#Set time values
start_time = "2015-12-01 00:00:00"
end_time = "2015-12-30 23:00:00"
output_interval = 3600


def read_catchment_data_geojson(catchment_data_file):
  """
  Read cathcment data from geojson file and return a series
  of catchment ids.
  """
  catchment_data_df = gpd.read_file(catchment_data_file)
  
  catchment_id_series = catchment_data_df["id"]
  
  return catchment_id_series


def randomly_select_formulations_for_catchments(catchment_id_series):
  """
  Randomly select between currently two formulations for each catchment
  and return a dataframe for with catchment ids and formulation names.
  """

  catchment_df = catchment_id_series.to_frame()

  catchment_df["Formulation"] = ""

  for index, row in catchment_df.iterrows():

    #Random integer, 0 or 1
    random_int = random.randint(0, 1)

    if (random_int == 0):
      catchment_df.at[index, "Formulation"] = global_formulation 

    else:
      catchment_df.at[index, "Formulation"] = formulation_1

  return catchment_df


def set_up_config_dict(catchment_df):
  """
  Construct and return a dictionary for the entire configuration
  """
  #Convert catchment_df to dictionary
  catchment_dict = catchment_df.to_dict()

  #Outline of full config dictionary
  config_dict = {
    "global": {
      "formulations": 
      [
        {
          "name": global_formulation,
          "params": global_params
        }
      ],
      "forcing": global_forcing
    },
    "time":
    {
      "start_time": start_time,
      "end_time": end_time,
      "output_interval": output_interval
    },
    "catchments": {}
  }

  #Cycle through each catcment and add the catchment name/id, formulation,
  #and the formulation's corresponding params to the config dictionary.
  for index, row in catchment_df.iterrows():
    catchment_id = catchment_df.at[index, "id"]
    catchment_formulation = catchment_df.at[index, "Formulation"]

    #Add catchment name/id and params
    if catchment_formulation == global_formulation:
      formulation_dict = {"name": catchment_formulation, "params": global_params}
    else:
      formulation_dict = {"name": catchment_formulation, "params": formulation_1_params}

    #Formulations is a list with currently only one formulation.
    #The ability to add multiple formulations for each catchment will
    #be added in the future.
    formulations_list = []

    formulations_list.append(formulation_dict)

    formulations_dict = {"formulations": formulations_list}

    forcing_dict = {"forcing": "forcing_file_path_and_name"}

    #TODO: Update how a catchment's forcing is passed here
    catchment_dict = {catchment_id: {"formulations": formulations_list, "forcing": "forcing_file_path_and_name"}}

    config_dict["catchments"].update(catchment_dict)

  return config_dict


def dump_dictionary_to_json(config_dict, output_file):
  """
  Dump config dictionary to JSON file
  """

  with open(output_file, "w") as open_output_file: 
    json.dump(config_dict, open_output_file, indent=4)


def main(argv):
  """
  Main function grabs command line file args
  """
  input_file = ''
  output_file = ''
  try:
     opts, args = getopt.getopt(argv,"hi:o:",["ifile=","ofile="])
  except getopt.GetoptError:
     print ('realization_config_generator.py -i <input_file> -o <output_file>')
     sys.exit(2)
  for opt, arg in opts:
     if opt == '-h':
        print ('realization_config_generator.py -i <input_file> -o <output_file>')
        sys.exit()
     elif opt in ("-i", "--ifile"):
        input_file = arg
     elif opt in ("-o", "--ofile"):
        output_file = arg

  catchment_id_series = read_catchment_data_geojson(input_file)

  catchment_df = randomly_select_formulations_for_catchments(catchment_id_series)

  config_dict = set_up_config_dict(catchment_df)

  dump_dictionary_to_json(config_dict, output_file)

if __name__ == "__main__":
  main(sys.argv[1:])
