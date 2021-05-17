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

import os
import sys, getopt
import geopandas as gpd
import random
import json
from shutil import copy


#Set directory path for catchment specific BMI configuration files
bmi_config_dir = "./catchment_bmi_configs"

#Set the names of the formulations
global_formulation = "bmi_c"
formulation_1 =  "top_model"

#Set params
global_params = {
  "model_type_name": "bmi_c_cfe",
  "library_file": "./extern/cfe/cmake_cfe_lib/libcfemodel.so",
  "forcing_file": "./data/forcing/cat-27_2015-12-01 00_00_00_2015-12-30 23_00_00.csv",
  "init_config": "./data/bmi/c/cfe/cat_27_bmi_config.txt",
  "main_output_variable": "Q_OUT",
  "uses_forcing_file": True
}

formulation_1_params = {}

#Set BMI config file params
global_bmi_params = {
  "forcing_file": "./forcings/cat58_01Dec2015.csv",
  "soil_params.depth": "2.0",
  "soil_params.b": "4.05",
  "soil_params.mult": "1000.0",
  "soil_params.satdk": "0.00000338",
  "soil_params.satpsi": "0.355",
  "soil_params.slop": "1.0",
  "soil_params.smcmax": "0.439",
  "soil_params.wltsmc": "0.066",
  "max_gw_storage": "16.0",
  "Cgw": "0.01",
  "expon": "6.0",
  "gw_storage": "50%",
  "alpha_fc": "0.33",
  "soil_storage": "66.7%",
  "K_nash": "0.03",
  "K_lf": "0.01",
  "nash_storage": "0.0,0.0",
  "giuh_ordinates": "0.06,0.51,0.28,0.12,0.03"
}

formulation_1_bmi_params = {
  "model_type": "top_model"
}

#Set path from where to retrieve catchment specific files
catchment_specific_file_from_path = "../../../catchment_specific_files"

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


def create_catchment_bmi_directory_and_config_file(catchment_id, bmi_params, formulation_type):
  """
  Creates unique directory for a given catchment_id and within that directory,
  also creates the BMI configuration file.
  """

  catchment_dir = os.path.join(bmi_config_dir, catchment_id)

  os.makedirs(catchment_dir, exist_ok=True)

  bmi_config_file = os.path.join(catchment_dir, "config.ini")

  #Use if config is JSON file
  #dump_dictionary_to_json(bmi_params, bmi_config_file)
  
  #Use if config is INI file
  dump_dictionary_to_ini(bmi_params, bmi_config_file)

  #Copy formulation_1 input files to catchment directory
  #Currently only copies one input file per catchment
  if formulation_type == "formulation_1":
    for input_file in os.listdir(catchment_specific_file_from_path):
      if catchment_id in input_file:
        input_file_with_path = os.path.join(catchment_specific_file_from_path, input_file)

        copy(input_file_with_path, catchment_dir)

  return bmi_config_file


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
      print (catchment_id + " global")
      #Call to create unique directory to hold BMI configuraton file for the given catchment
      bmi_config_file = create_catchment_bmi_directory_and_config_file(catchment_id, global_bmi_params, "global_formulation")

      formulation_dict = {"name": catchment_formulation, "params": global_params, "bmi_config_file": bmi_config_file}

    else:
      
      #Call to create unique directory to hold BMI configuraton file for the given catchment
      bmi_config_file = create_catchment_bmi_directory_and_config_file(catchment_id, formulation_1_bmi_params, "formulation_1")

      formulation_dict = {"name": catchment_formulation, "params": formulation_1_params, "bmi_config_file": bmi_config_file}
      
    #Formulations is a list with currently only one formulation.
    #The ability to add multiple formulations for each catchment will
    #be added in the future.
    formulations_list = []

    formulations_list.append(formulation_dict)

    #formulations_dict = {"formulations": formulations_list, "bmi_confi_file": bmi_config_file}

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


def dump_dictionary_to_ini(config_dict, output_file):
  """
  Dump config dictionary to INI file
  """
    
  with open(output_file, "w") as open_output_file: 
    for key, value in config_dict.items():
      open_output_file.write(key + "=" +  value + "\n")                 



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

  #Create BMI directory for catchment specific BMI configuration files 
  os.makedirs(bmi_config_dir, exist_ok=True)

  catchment_id_series = read_catchment_data_geojson(input_file)

  catchment_df = randomly_select_formulations_for_catchments(catchment_id_series)

  config_dict = set_up_config_dict(catchment_df)

  dump_dictionary_to_json(config_dict, output_file)

if __name__ == "__main__":
  main(sys.argv[1:])
