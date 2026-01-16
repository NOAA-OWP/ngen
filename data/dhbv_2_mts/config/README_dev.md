# BMI Configuration
BMI requires a configuration file for each model. The LSTM configuration files contains key value pairs that are used by the BMI to run the model. Below are examples and descriptions of each of such keys, and what type of values are associated.

## Initialization Information
These key value pairs are used by the BMI to set up the model in some particular way  
- `train_cfg_file: ./trained_neuralhydrology_models/hourly_all_attributes_and_forcings/config.yml` found [here]( ./trained_neuralhydrology_models/hourly_all_attributes_and_forcings/config.yml). This is a very important part of the LSTM model. This is a configuration file used when training the model. It has critical information on the LSTM architecture and should not be altered.
- `initial_state: 'zero'` This is an option to set the initial states of the model to zero.
- `verbose: 0` Change to `1` in order to print additional BMI information during runtime.

## Static Attributes
These are static attributes that are particular to the catchment. These should be calculated in the same manner as the values which the LSTM was trained. Some description is provided below, but again see [Addor et al. 2017](https://doi.org/10.5194/hess-21-5293-2017) for more details. 
- `area_sqkm: 620.38` allows bmi to adjust a weighted output
- `elev_mean: 92.68` catchment mean elevation (m) above sea level
- `slope_mean: 17.79072` catchment mean slope (m km−1)
- `area_gages2: 573.60000` catchment area (GAGESII estimate), (km2)
- `frac_forest: 0.9232` forest fraction
- `lai_max: 4.87139` maximum monthly mean of the leaf area index (based on 12 monthly means)
- `lai_diff: 3.74669` difference between the maximum and minimum monthly mean of the leaf area index (based on 12 monthly means)
- `gvf_max: 0.863936` maximum monthly mean of the green vegetation fraction (based on 12 monthly means)
- `gvf_diff: 0.337712` difference between the maximum and minimum monthly mean of the green vegetation fraction (based on 12 monthly means)
- `soil_depth_pelletier: 17.412808` depth to bedrock (maximum 50 m) (m)
- `soil_depth_statsgo: 1.491846` soil depth (maximum 1.5 m; layers marked as water and bedrock were excluded) (m)
- `soil_porosity: 0.415905` volumetric porosity (saturated volumetric water content estimated using a multiple linear regression based on sand and clay fraction for the layers marked as USDA soil texture class and a default value (0.9) for layers marked as organic material; layers marked as water, bedrock, and “other” were excluded)
- `soil_conductivity: 2.375005` saturated hydraulic conductivity (estimated using a multiple linear regression based on sand and clay fraction for the layers marked as USDA soil texture class and a default value (36 cm h−1) for layers marked as organic material; layers marked as water, bedrock, and “other” were excluded) (cm h-1)
- `max_water_content: 0.626229` maximum water content (combination of porosity and soil depth statsgo; layers marked as water, bedrock, and “other” were excluded)
- `sand_frac: 59.390156` sand fraction (of the soil material smaller than 2mm; layers marked as organic material, water, bedrock, and “other” were excluded)
- `silt_frac: 28.080937` silt fraction (of the soil material smaller than 2mm; layers marked as organic material, water, bedrock, and “other” were excluded)
- `clay_frac: 12.037646` clay fraction (of the soil material smaller than 2mm; layers marked as organic material, water, bedrock, and “other” were excluded)
- `carbonate_rocks_frac: 0` fraction of the catchment area characterized as “carbonate sedimentary rocks”. GLiM
- `geol_permeability: -14.2138`
- `p_mean: 3.60813` mean daily precipitation (mm day-1)
- `pet_mean: 2.11926` mean daily PET, estimated by N15 using Priestley–Taylor formulation calibrated for each catchment (mm day-1)
- `aridity: 0.587356` aridity (PET /P, ratio of mean PET, estimated by N15 using Priestley–Taylor formulation calibrated for each catchment, to mean precipitation)
- `frac_snow: 0.245259` fraction of precipitation falling as snow (i.e., on days colder than 0 C)
- `high_prec_freq: 20.55` frequency of high precipitation days (≥5 times mean daily precipitation) (days yr-1)
- `high_prec_dur: 1.20528` average duration of high precipitation events (number of consecutive days ≥5 times mean daily precipitation)
- `low_prec_freq: 233.65` frequency of dry days (< 1mmday−1) (days yr-1)
- `low_prec_dur: 3.66223` average duration of dry periods (number of consecutive days < 1mmday−1) (days)

### Optional Metadata
These key value pairs contain metadata that are not required to run the model but can be useful to make sure that the model is running as expected.  It is best to consider items listed here as optional, but also as potential enhanced development looking ahead. 
- `time_step: '1 hour'` As of this writing, both `time_step_size` and `time_units` are defined during `bmi.initialze()`.  The next phase of development will provide the model with all time information via bmi configuration. 
- `basin_name: 'Narraguagus River at Cherryfield, Maine'` Not currently directly used but may be beneficial for bookkeeping.
- `basin_id: '01022500'` Future development will require unique ID for node-to-node routing; still under beta 
- `lat: 44.60797` Post-run analysis or plotting only
- `lon: -67.93524` Post-run analysis or plotting only