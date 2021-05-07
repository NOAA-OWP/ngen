#!/usr/bin/env python
'''
Script to generate a realization configuration JSON file with randomly selected
formulations for each catchment given a catchment data geojson file as input. 
'''

import sys, getopt
import geopandas as gpd
import random

#Set the names of the formulations
global_formulation = "tshirt_c"
formulation_1 = "top_model"

#Set params
global_params = {}
formulation_1_params = {}

#Set time values
start_time = "2015-12-01 00:00:00"
end_time = "2015-12-30 23:00:00"
output_interval = 3600

def read_catchment_data_geojson(catchment_data_file):
  catchment_data_df = gpd.read_file(catchment_data_file)
  #features = gj['features'][0]
  #print (catchment_data_df)
  catchment_id_series = catchment_data_df["id"]
  #print (catchment_id_list)
  return catchment_id_series

def randomly_select_formulations_for_catchments(catchment_id_series):
  catchment_df = catchment_id_series.to_frame()

  catchment_df["Formulation"] = ""

  for index, row in catchment_df.iterrows():

    print ("value")
    #print (value)
    print (row)

    random_int = random.randint(0, 1)
    print (random_int)

    if (random_int == 0):
      catchment_df.at[index, "Formulation"] = global_formulation 

    else:
      catchment_df.at[index, "Formulation"] = formulation_1
      
  print("catchment_df")
  print(catchment_df)

  return catchment_df

#TEMP
def pretty(d, indent=0):
   for key, value in d.items():
      print('\t' * indent + str(key))
      if isinstance(value, dict):
         pretty(value, indent+1)
      else:
         print('\t' * (indent+1) + str(value))


def set_up_config_dict(catchment_df):

  #Convert catchment_df to dictionary
  catchment_dict = catchment_df.to_dict()
  print("catchment_dict")
  print(catchment_dict)



  config_dict = {
    "global": {
      "formulations": 
      [
        {
          "name": global_formulation,
          "params": global_params
        }
      ],
      "forcing": {}
    },
    "time":
    {
      "start_time": start_time,
      "end_time": end_time,
      "output_interval": output_interval
    },
    "catchments": {}
  }


  for index, row in catchment_df.iterrows():
    catchment_id = catchment_df.at[index, "id"]
    catchment_formulation = catchment_df.at[index, "Formulation"]

    print ("---------------")
    print (catchment_id)
    print (catchment_formulation) 

    #TODO: assign corresponding params 
    formulation_dict = {"name": catchment_formulation, "params": {}}

    formulations_list = []

    formulations_list.append(formulation_dict)

    formulations_dict = {"formulations": formulations_list}

    forcing_dict = {"forcing": "forcing_file_path_and_name"}

    #catchment_values = {formulations_dict, forcing_dict}

    #catchment_dict = {catchment_id: {formulations_list, forcing_dict}} 
    #catchment_dict = {catchment_id: {formulations_dict, forcing_dict}} 
    #catchment_dict = {catchment_id: catchment_values} 

    catchment_dict = {catchment_id: {"formulations": formulations_list, "forcing": "forcing_file_path_and_name"}}

    #catchment_dict = {catchment_id: forcing_dict} 
    #catchment_dict = {catchment_id: formulations_list} 


    #config_dict["catchments"].update(catchment_id: {})
    #config_dict.update(catchment_id: {})

    #config_dict.update(catchment_dict)
    config_dict["catchments"].update(catchment_dict)


  print ("====================")
  #print (config_dict)
  pretty(config_dict)


def main(argv):
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
  print ('Input file is ', input_file)
  print ('Output file is ', output_file)

  catchment_id_series = read_catchment_data_geojson(input_file)
  print (catchment_id_series)

  catchment_df = randomly_select_formulations_for_catchments(catchment_id_series)

  set_up_config_dict(catchment_df)

if __name__ == "__main__":
  main(sys.argv[1:])
