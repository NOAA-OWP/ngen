# Output Variable Units from Running Model Engine

When running model engine with hydrofabric, the outputs are typically stored in two types of files, cat-###.csv, and nex-###_output.csv, where "###" represents a series of digits (not necessarily 3, but at least 1). The model engine at present does not write the output data with units, the values in the cat-###.csv files come from the model formulation running for that catchment. There are additional output variables that are not witten out to the files. The units of these variables are dictated by the formulation, and the model engine doesn't (currently) do any inspection of the units for output. One option currently is to look at the formulation documentation and see what each BMI variable it advertises as an output variable has for units. We may, in the future, use some BMI functionality to try to attach those units to output automatically, but that is difficult to do with the CSV output we currently produce, [#744](https://github.com/NOAA-OWP/ngen/pull/744) may help to start addressing that.

As for the nex-###.csv outputs, these are the accumulated overland flow contributions at the point from all directly connected catchments assoicated with the nexus. These values should be in units of m^3/s given that a formulation's main_output_variable returns a rate, e.g. m/s, the main_output_variable is currently automatically multiplied by the catchment's area to produce the volumetric flow rate.

That said, for users who are interested in using the CSV format, we tabulate below output variables with their unuts for a few commonly used hydrologic models for users' convenniece.

## Conceptual Functional Equivalent (CFE) Model
| Output Variable Name | Physical Meaning | Units |
| ------------- | :-----: | :--------: |
| RAIN_RATE | the amount of rain that falls over a specific time period per unit area | m/h |
| DIRECT_RUNOFF | Water that flows over the ground surface directly into water bodies |m/h |
| GIUH_RUNOFF | rainfall-runoff from Geomorphological Instantaneous Unit Hydrograph model | m/h |
| NASH_LATERAL_RUNOFF | lateral runoff from Nash-cascade of reservoirs model | m/h |
| DEEP_GW_TO_CHANNEL_FLUX | flux from the deep reservoir into the channels | m/h |
| SOIL_TO_GW_FLUX | the movement of water from the soil layer into the groundwater table below | m/h |
| Q_OUT | Total runoff (sum of GIUH_RUNOFF, NASH_LATERAL_RUNOFF, DEEP_GW_TO_CHANNEL_FLUX | m/h |
| POTENTIAL_ET | the maximum possible amount of water that would evaporate and transpire from a given surface | m/h |
| ACTUAL_ET | the actual amount of water lost from a surface due to evaporation and transpiration | m/h |
| GW_STORAGE | the amount of water that is stored aquifers, i.e., underground water reservoirs | m |
| SOIL_STORAGE | The amount of water that is held in the soil | m |
| SOIL_STORAGE_CHANGE | changes in soil water storage | m |
| SURF_RUNOFF_SCHEME | Schaake or Xinanjiang | none |


## Potential Evapotranspiration (PET)
| Output Variable Name | Physical Meaning | Units |
| ------------- | :-----: | :--------: |
| water_potential_evaporation_flux | the maximum possible amount of water that would evaporate and transpire from a given surface | (m/s) |


## Noah OWP Modular
Output Variable Name | Physical Meaning | Units |
| ------------- | :-----: | :--------: |
| QINSUR | total liquid water input to surface rate | m/s |
| ETRAN | transpiration rate | mm/s |
| QSEVA | evaporation rate | m/s |
| TG | surface/ground temperature; becomes snow surface temperature when snow is present | K |
| SNEQV | snow water equivalent | mm |
| TGS | ground temperature (equal to TG when no snow; equal to bottom of snow temperature when there is snow | K |
| ACSNOM | Accumulated meltwater from bottom snow layer (NWM 3.0 output variable) | mm |
| SNOWT_AVG | Average snow temperature (by layer mass) (NWM 3.0 output variable) | K |
| ISNOW | Number of snow layers (NWM 3.0 output variable) | none |
| QRAIN | Rainfall rate on the ground (NWM 3.0 output variable) | mm/s |
| FSNO | Snow-cover fraction on the ground (NWM 3.0 output variable) | none |
| SNOWH | Snow depth (NWM 3.0 output variable) | m |
| SNLIQ | Snow layer liquid water (NWM 3.0 output variable) | mm |
| QSNOW | Snowfall rate on the ground (NWM 3.0 output variable)| mm/s |
| ECAN | evaporation of intercepted water (NWM 3.0 output variable) | mm |
| GH | Heat flux into the soil (NWM 3.0 output variable) | W/m-2 |
| TRAD | Surface radiative temperature (NWM 3.0 output variable) | K |
| FSA | Total absorbed SW radiation (NWM 3.0 output variable) | W/m-2 |
| CMC | Total canopy water (liquid + ice) (NWM 3.0 output variable) | mm |
| LH | Total latent heat to the atmosphere (NWM 3.0 output variable) | W/m-2 |
| FIRA | Total net LW radiation to atmosphere (NWM 3.0 output variable) | W/m-2 |
| FSH | Total sensible heat to the atmosphere (NWM 3.0 output variable) | W/m-2 |


## Topmodel
Output Variable Name | Physical Meaning | Units |
| ------------- | :-----: | :--------: |
| Qout (= qb + qof) | accumulated discharge | m/h |
| atmosphere_water__liquid_equivalent_precipitation_rate_out (p) | adjusted rainfall | m/h |
| water_potential_evaporation_flux_out (ep) | adjusted potential evaporation | m/h |
| land_surface_water__runoff_mass_flux (Q[i]) | simulated discharge histogram | m/h |
| soil_water_root-zone_unsat-zone_top__recharge_volume_flux (qz) | flow from root zone to unsaturated zone | m/h |
| land_surface_water__baseflow_volume_flux (qb) | subsurface flow or baseflow | m/h |
| soil_water__domain_volume_deficit (sbar) | the catchment average soil moisture deficit | m |
| land_surface_water__domain_time_integral_of_overland_flow_volume_flux (qof) | flow from saturated area and infiltration excess flow  | m/h |
| land_surface_water__domain_time_integral_of_precipitation_volume_flux (sump) | accumulated rainfall | m |
| land_surface_water__domain_time_integral_of_evaporation_volume_flux (sumae) | accumulated evapotranspiration | m |
| land_surface_water__domain_time_integral_of_runoff_volume_flux (sumq) | accumulated discharge (Qout) | m |
| soil_water__domain_root-zone_volume_deficit (sumrz) | deficit_root_zone over the whole watershed | m |
| soil_water__domain_unsaturated-zone_volume (sumuz) | stor_unsat_zone over the whole watershed | m |
| land_surface_water__water_balance_volume (bal) | the residual of the water balance | m |

* The symbols in the parentheses in the Output Variable Name column are the actual variable names appearing in output files.
