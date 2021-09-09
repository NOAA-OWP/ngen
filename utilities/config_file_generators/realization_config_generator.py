#!/usr/bin/env python
"""
Script to generate a realization configuration JSON file with randomly selected
formulations for each catchment given a catchment data geojson file as input. 

Script and command line arg format:
realization_config_generator.py -i <path and file name to catchment data geojson file>
-o <file name of config output file>

Example:
python realization_config_generator.py -i ../catchment_data.geojson -o realization_config.json
"""

import geopandas as gpd
import getopt
import json
import os
import random
import subprocess
import sys
from shutil import copy

#Set directory path for catchment specific BMI configuration files
bmi_config_dir = "./catchment_bmi_configs"

#Set paths from where to retrieve catchment specific files
global_catchment_specific_file_from_path = "../../../global_catchment_specific_files"
formulation_1_catchment_specific_file_from_path = "../../../catchment_specific_files"

#Set forcing file path
forcing_file_path = "./data/forcing/"

#Set forcing file date time
forcing_file_date_time = "2012-05-01_00_00_00_2012-06-29_23_00_00"

#Set forcing file extenstion
forcing_file_extension = ".csv"

#Set the descriptor names of the formulations.
#Note: this is used just two distinguish the different
#formulation types and is not written to the config file
global_formulation = "bmi_cfe"
formulation_1 =  "bmi_topmodel"

#Set the names of the formulations
#Note: multiple formulations can have the same formulation
#name but have different parameters. All model formulations
#that use the BMI C interface will have their formulation
#name set to "bmi_c"
global_formulation_name = "bmi_c"
formulation_1_name =  "bmi_c"


#Set params
global_params = {
  "model_type_name": "bmi_c_cfe",
  "library_file": "../ngen_test_setup/ngen/extern/cfe/cmake_build/libcfebmi.so",
  "init_config": "",
  "main_output_variable": "Q_OUT",
  "registration_function": "register_bmi_cfe",
  "variables_names_map" : {
    "water_potential_evaporation_flux" : "potential_evapotranspiration",
    "atmosphere_water__liquid_equivalent_precipitation_rate" : "precip_rate",
    "atmosphere_air_water~vapor__relative_saturation" : "SPFH_2maboveground",
    "land_surface_air__temperature" : "TMP_2maboveground",
    "land_surface_wind__x_component_of_velocity" : "UGRD_10maboveground",
    "land_surface_wind__y_component_of_velocity" : "VGRD_10maboveground",
    "land_surface_radiation~incoming~longwave__energy_flux" : "DLWRF_surface",
    "land_surface_radiation~incoming~shortwave__energy_flux" : "DSWRF_surface",
    "land_surface_air__pressure" : "PRES_surface"
     },
  "uses_forcing_file": False,
  "allow_exceed_end_time": True
}

formulation_1_params = {
  "model_type_name": "bmi_topmodel",
  "library_file": "../ngen_test_setup/ngen/extern/topmodelbmi/cmake_build/libtopmodelbmi.so",
  "init_config": "",
  "main_output_variable": "Qout",
  "registration_function": "register_bmi_topmodel",
  "variables_names_map" : {
    "water_potential_evaporation_flux" : "potential_evapotranspiration",
    "atmosphere_water__liquid_equivalent_precipitation_rate" : "precip_rate"
     },
  "uses_forcing_file": False
}

#Set BMI config file params
global_bmi_params = {
  "forcing_file": "BMI",
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
   "name": "Catchment Calibration Data",
   "inputs_file": "inputs.dat", #currently unused file
   "subcat_file": "", #given unique name per catchment
   "params_file": "../param_files/params.dat", #enter path to single file shared by all catchments
   "topmod_output_file": "topmod.out", #leave as is for now
   "hydro_output_file": "hyd.out" #leave as is for now
}

#Set global forcing
global_forcing = {
  "file_pattern": ".*{{id}}.*." + forcing_file_extension,
  "path": forcing_file_path
}

#Set time values
start_time = "2012-05-01 00:00:00"
end_time = "2012-05-02 23:00:00"
output_interval = 3600


def create_catchment_bmi_directory_and_config_file(catchment_id, bmi_params, formulation_type):
  """
  Creates unique directory for a given catchment_id and within that directory,
  also creates the BMI configuration file.
  """

  catchment_dir = os.path.join(bmi_config_dir, catchment_id)

  os.makedirs(catchment_dir, exist_ok=True)

  #bmi_config_file = os.path.join(catchment_dir, "config.ini")

  #Use if config is JSON file
  #dump_dictionary_to_json(bmi_params, bmi_config_file)
  
  #Use if config is INI file
  #dump_dictionary_to_ini(bmi_params, bmi_config_file)

  #Copy formulation_1 input files to catchment directory
  #Currently only copies one input file per catchment
  if formulation_type == "formulation_1":
    #bmi_config_file = os.path.join(catchment_dir, "topmod.run")

    #dump_dictionary_values_to_text_file(bmi_params, bmi_config_file)

    input_file_found_flag = False

    #Temporarily hard coded for DAT files
    input_file_to_search_for = catchment_id + ".dat"

    #Search for input files to copy to specific catchment directories
    for input_file in os.listdir(formulation_1_catchment_specific_file_from_path):
      if input_file == input_file_to_search_for:
        input_file_with_path = os.path.join(formulation_1_catchment_specific_file_from_path, input_file)

        copy(input_file_with_path, catchment_dir)

        input_file_found_flag = True

        data_file = os.path.join(catchment_dir, input_file)

        #Remove windows carriage return
        subprocess.call(["sed", "-i", 's/\r$//', f"{data_file}"])

        #Set flags on first line to "1 1 0"
        subprocess.call(["sed", "-i", '1 s/^.*$/1  1  0/', f"{data_file}"])

    if not input_file_found_flag:
      print("WARNING: Forumulation_1 input file missing for catchment: " + catchment_id)

      data_file = "---------MISSING----------"
    
    #bmi_params_for_catchment = global_params.copy()
    bmi_params["subcat_file"] = data_file

    bmi_config_file = os.path.join(catchment_dir, "topmod.run")

    dump_dictionary_values_to_text_file(bmi_params, bmi_config_file)

  #For now, only create INI file for CFE models
  else:
    #bmi_config_file = os.path.join(catchment_dir, "config.ini")

    #dump_dictionary_to_ini(bmi_params, bmi_config_file)

    input_file_found_flag = False

    #Temporarily hard coded for INI files
    input_file_to_search_for = catchment_id + "_bmi_config_cfe_pass.txt"

    #Search for input files to copy to specific catchment directories
    for input_file in os.listdir(global_catchment_specific_file_from_path):
      if input_file == input_file_to_search_for:
        input_file_with_path = os.path.join(global_catchment_specific_file_from_path, input_file)

        copy(input_file_with_path, catchment_dir)

        input_file_found_flag = True

        bmi_config_file = os.path.join(catchment_dir, input_file)

        #Remove windows carriage return
        subprocess.call(["sed", "-i", 's/\r$//', f"{bmi_config_file}"])

    if not input_file_found_flag:
      print("WARNING: Global input file missing for catchment: " + catchment_id)

      bmi_config_file = "---------MISSING----------"

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

  catchment_df["Formulation_Name"] = ""

  for index, row in catchment_df.iterrows():

    #Random integer, 0 or 1
    random_int = random.randint(0, 1)

    if (random_int == 0):
      catchment_df.at[index, "Formulation"] = global_formulation 
      catchment_df.at[index, "Formulation_Name"] = global_formulation_name

    else:
      catchment_df.at[index, "Formulation"] = formulation_1
      catchment_df.at[index, "Formulation_Name"] = formulation_1_name

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
          "name": global_formulation_name,
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
    catchment_formulation_name = catchment_df.at[index, "Formulation_Name"]
    
    forcing_file_name = catchment_id + "_" + forcing_file_date_time + forcing_file_extension

    forcing_file_name_and_path = os.path.join(forcing_file_path, forcing_file_name) 

    forcing_dict = {"path": forcing_file_name_and_path}

    #Add catchment name/id and params
    if catchment_formulation == global_formulation:
      
      #Call to create unique directory to hold BMI configuraton file for the given catchment
      bmi_config_file = create_catchment_bmi_directory_and_config_file(catchment_id, global_bmi_params, "global_formulation")

      global_params_for_catchment = global_params.copy()

      global_params_for_catchment["init_config"] = bmi_config_file
      
      #Use below if forcing file is passed through BMI
      #global_params_for_catchment["forcing_file"] = forcing_file_name_and_path

      formulation_dict = {"name": catchment_formulation_name, "params": global_params_for_catchment}

    else:
      
      #Call to create unique directory to hold BMI configuraton file for the given catchment
      bmi_config_file = create_catchment_bmi_directory_and_config_file(catchment_id, formulation_1_bmi_params, "formulation_1")

      formulation_1_params_for_catchment = formulation_1_params.copy()

      formulation_1_params_for_catchment["init_config"] = bmi_config_file
      
      #Use below if forcing file is passed through BMI
      #formulation_1_params_for_catchment["forcing_file"] = forcing_file_name_and_path
      
      formulation_dict = {"name": catchment_formulation_name, "params": formulation_1_params_for_catchment}
      
    #Formulations is a list with currently only one formulation.
    #The ability to add multiple formulations for each catchment will
    #be added in the future.
    formulations_list = []

    formulations_list.append(formulation_dict)

    catchment_dict = {catchment_id: {"formulations": formulations_list, "forcing": forcing_dict}}

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


def dump_dictionary_values_to_text_file(config_dict, output_file):
  """
  Dump config dictionary values to text file
  """
    
  with open(output_file, "w") as open_output_file: 
    for key, value in config_dict.items():
      open_output_file.write(value + "\n")                 


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

  random.seed(0)

  catchment_df = randomly_select_formulations_for_catchments(catchment_id_series)

  config_dict = set_up_config_dict(catchment_df)

  dump_dictionary_to_json(config_dict, output_file)

if __name__ == "__main__":
  main(sys.argv[1:])
