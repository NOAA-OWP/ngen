# LSTM Model

* [Summary](#summary)
* [Formulation Config](#formulation-config)
    * [Required Parameters](#required-parameters)

## Summary

A long short-term memory (LSTM) model and corresponding realization is included in the ngen framework. The model is not included in the default ngen build because it requires the LibTorch library in order to compile and run, and this is not a basic required library for ngen. 

The basic outline of steps needed to run the LSTM model is:
  * [Install the LibTorch library version 1.8 or later.](https://pytorch.org/docs/stable/cpp_index.html)
  * Create the build directory including the option to activate the lstm: 
  
      `cmake -DCMAKE_BUILD_TYPE=Debug -B cmake-build-debug -DLSTM_TORCH_LIB_ACTIVE:BOOL=ON -S .`  
  
  * Unit tests for the LSTM model and realization can then be built and run from the main directory with the following two commands:
  
      `cmake --build cmake-build-debug --target test_lstm`  <br />
      
      `./cmake-build-debug/test/test_lstm`  
  
  * The formulation config and required parameters to run the LSTM for a given catchment are below.

## Formulation Config
An [example realization.](https://github.com/NOAA-OWP/ngen/tree/master/data/lstm/example_lstm_realization_config.json)
This example realziation can be run with the following command from the main ngen directory:  

`cmake-build-debug/ngen ./data/catchment_data.geojson "cat-67" ./data/nexus_data.geojson "nex-68" ./data/lstm/example_lstm_realization_config.json`

### Required Parameters
The following must be present in the formulation/realization JSON config for a catchment entry using the LSTM formulation type:
* `pytorch_model_path`
  * String name with path of a trained LSTM PyTorch model
* `normalization_path`
  * String name with path of a CSV file containing normalization parameters to the inputs of the LSTM model
* `initial_state_path`
  * String name with path of a CSV file containing the initial states of the LSTM model
  * The model currently requires an initial state though the code could be updated to not require one
* `latitude`
  * Double holding the catchment latitude
* `longitude`
  * Double holding the catchment longitude
* `area_square_km`
  * Double holding the catchment area in sqaure km
* `useGPU`
  * Boolean giving option to load and run and model on a GPU

