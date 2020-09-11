#ifndef ET_WRAPPER_FUNCTION_H
#define ET_WRAPPER_FUNCTION_H

#include <stdio.h>
#include <iostream>
#include <cmath>
#include <cstring>

//local includes
#include "EtStruct.h"
#include "EtCalcProperty.hpp"
#include "EtEnergyBalanceMethod.hpp"
#include "EtAerodynamicMethod.hpp"
#include "EtCombinationMethod.hpp"
#include "EtPriestleyTaylorMethod.hpp"
#include "EtPenmanMonteithMethod.hpp"

// NOTE: SET YOUR EDIT WINDOW TO 120 CHARACTER WIDTH TO READ THIS CODE IN ITS ENTIRETY.

//#####################################
// evapotranspiration (ET) module, 
// Version 1.0 by Fred L. Ogden, NOAA-NWS-OWP, May, 2020.
//
// includes five different methods to clculate ET, from Chow, Maidment & Mays Textbook, and UNFAO Penman-Monteith:
// 1. energy balance method
// 2. aerodynamic method
// 3. combination method, which combines 1 & 2.
// 4. Priestley-Taylor method, which assumes the ratio between 1 & 2, and only calculates 1.
// 5. Penman-Monteith method, which requires a value of canopy resistance term, and does not rely on 1 or 2.
// 
// This subroutine requires a considerable amount of meteorological data as input.
// a. temperature and (relative-humidity or specific humidity) and the heights at which they are measured.
// b. near surface wind speed measurement and the height at which it was measured.
// c. the ambient atmospheric temperature lapse rate
// d. the fraction of the sky covered by clouds
// e. (optional) the height above ground to the cloud base. If not provided, then assumed.
// f. the day of the year (1-366) and time of day (UTC only!)
// g. the skin temperature of the earth's surface, TODO: should come from another module to calc. soil or veg. temp.
// h. the zero-plane roughness height of the atmospheric boundary layer assuming log-law behavior (from land cover)
// i. the average root zone soil temperature, or near-surface water temperature in the case of lake evaporation.
// j. the incoming solar (shortwave) radiation.  If not provided it is computed from d,e,f, using an
//    updated method similar to the one presented in Bras, R.L. Hydrology.  Requires value of the Linke atmospheric
//    turbidity factor, which varies from 2 for clear mountain air to 5 for smoggy air.  According to Hove & Manyumbu
//    2012, who calculated values over Zimbabwe that varied from 2.14 to 3.71.  Other values exist in the literature.
//    TODO: This turbidity factor could be calculated from satellite obs. or maybe NOAA already does this?
//
// All radiation calculations needed for 1, 3, 4, and 5 require net radiation calculations at the land surface.
// the net radiation is calculated using a, c, d, e, f, g, j, plus the Linke turbidity factor, which can be estimate
// from satellite observations.
//

// NOTE THE VALUE OF evapotranspiration_params.zero_plane_displacement_height COMES FROM LAND COVER DATA.
// Taken from:    https://websites.pmc.ucsc.edu/~jnoble/wind/extrap/
//
//Roughness Roughness  Landscape Type
// Class    Length (m)	
//----------------------------------------------------------------------------------------------------------------
// 0         0.0002     Smooth water surface
// 0.2       0.0005     Inlet water
// 0.5       0.0024     Completely open terrain, smooth surface, e.g. concrete runways in airports, mowed grass, etc.
// 1         0.03       Open agricultural area without fences and hedgerows and very scattered buildings. Only softly
//                        rounded hills
// 1.5       0.055      Agricultural land with some houses and 8 metre tall sheltering hedgerows with a distance of 
//                        approximately 1250 metres
// 2         0.1        Agricultural land with some houses and 8 metre tall sheltering hedgerows with a distance of 
//                        approximately 500 metres
// 2.5       0.2        Agricultural land with many houses, shrubs and plants, or 8 metre tall sheltering hedgerows 
//                        with a distance of approximately 250 metres
// 3         0.4        Villages, small towns, agricultural land with many or tall sheltering hedgerows, forests 
//                        and very rough and uneven terrain
// 3.5       0.8        Larger cities with tall buildings
// 4         1.6        Very large cities with tall buildings and skyscrapers
// Roughness definitions according to the European Wind Atlas. 
//
// According to the UN FAO Penman-Monteith example here: 
//        http://www.fao.org/3/X0490E/x0490e06.htm#aerodynamic%20resistance%20(ra)
// the zero plane roughness length,"d" can be approximated as 2/3 of the vegetation height (H): d=2/3*H.
// the momentum roughness height "zom" can be estimated as 0.123*H.
// the heat transfer roughness height "zoh" can be approximated as 0.1 * zom.
//-----------------------------------------------------------------------------------------------------------------

double et_wrapper_function(aorc_forcing_data*          aorc_forcing,
                           solar_radiation_options*    solar_rad_options,
                           solar_radiation_parameters* solar_rad_params,
                           solar_radiation_forcing*    solar_rad_forcing,
                           evapotranspiration_options* set_et_options,
                           evapotranspiration_params*  set_et_params,
                           evapotranspiration_forcing* set_et_forcing,
                           surface_radiation_params*   surface_rad_params,
                           surface_radiation_forcing*  surface_rad_forcing)
{

  // FLAGS
  int yes_aorc; // if TRUE then using AORC forcing data- if FALSE then we must calculate incoming short/longwave rad.
  int yes_wrf;  // if TRUE then we get radiation winds etc. from WRF output.  TODO not implemented.

  struct aorc_forcing_data aorc;

  struct evapotranspiration_options et_options;
  struct evapotranspiration_params  et_params;
  struct evapotranspiration_forcing et_forcing;
  struct intermediate_vars inter_vars;

  struct surface_radiation_params   surf_rad_params;
  struct surface_radiation_forcing  surf_rad_forcing;

  struct solar_radiation_options    solar_options; 
  struct solar_radiation_parameters solar_params;
  struct solar_radiation_forcing    solar_forcing;
  struct solar_radiation_results    solar_results;


  double wind_speed_at_2_m;
  double et_m_per_s;
  double et_mm_per_d;
  double numerator,denominator;
  double saturation_vapor_pressure_Pa;
  double actual_vapor_pressure_Pa;

  //###################################################################################################
  // THE VALUE OF THESE FLAGS DETERMINE HOW THIS CODE BEHAVES.  CYCLE THROUGH THESE FOR THE UNIT TEST.
  //###################################################################################################
  // Set this flag to TRUE if meteorological inputs come from AORC
  // et_options.yes_aorc = TRUE;                      // if TRUE, it means that we are using AORC data.
  et_options.yes_aorc = set_et_options->yes_aorc;

  // set the et_options method value.  Only one of these should be TRUE. This is now set in calling fn
  // et_options.use_energy_balance_method   = FALSE;  
  // et_options.use_aerodynamic_method      = FALSE;
  // et_options.use_combination_method      = FALSE;
  // et_options.use_priestley_taylor_method = FALSE;
  // et_options.use_penman_monteith_method  = TRUE; 
  et_options.use_energy_balance_method   = set_et_options->use_energy_balance_method;
  et_options.use_aerodynamic_method      = set_et_options->use_aerodynamic_method;
  et_options.use_combination_method      = set_et_options->use_combination_method;
  et_options.use_priestley_taylor_method = set_et_options->use_priestley_taylor_method;
  et_options.use_penman_monteith_method  = set_et_options->use_penman_monteith_method;

  aorc.incoming_longwave_W_per_m2     =  aorc_forcing->incoming_longwave_W_per_m2;
  aorc.incoming_shortwave_W_per_m2    =  aorc_forcing->incoming_shortwave_W_per_m2;
  aorc.surface_pressure_Pa            =  aorc_forcing->surface_pressure_Pa;
  aorc.specific_humidity_2m_kg_per_kg =  aorc_forcing->specific_humidity_2m_kg_per_kg;   // results in a relative humidity of 40%
  aorc.air_temperature_2m_K           =  aorc_forcing->air_temperature_2m_K;
  aorc.u_wind_speed_10m_m_per_s       =  aorc_forcing->u_wind_speed_10m_m_per_s;
  aorc.v_wind_speed_10m_m_per_s       =  aorc_forcing->v_wind_speed_10m_m_per_s;
  aorc.latitude                       =  aorc_forcing->latitude;
  aorc.longitude                      =  aorc_forcing->longitude;
  aorc.time                           =  aorc_forcing->time;


  // populate the evapotranspiration forcing data structure:
  //---------------------------------------------------------------------------------------------------------------
  et_forcing.air_temperature_C             = (double)aorc.air_temperature_2m_K-TK;  // gotta convert it to C
  et_forcing.relative_humidity_percent     = (double)-99.9; // this negative number means use specific humidity
  et_forcing.specific_humidity_2m_kg_per_kg= (double)aorc.specific_humidity_2m_kg_per_kg;
  et_forcing.air_pressure_Pa               = (double)aorc.surface_pressure_Pa;
  et_forcing.wind_speed_m_per_s            = hypot((double)aorc.u_wind_speed_10m_m_per_s,
                                                   (double)aorc.v_wind_speed_10m_m_per_s);                 


  // ET forcing values that come from somewhere else...
  //---------------------------------------------------------------------------------------------------------------
  et_forcing.canopy_resistance_sec_per_m   = set_et_forcing->canopy_resistance_sec_per_m; // TODO: from plant growth model
  et_forcing.water_temperature_C           = set_et_forcing->water_temperature_C; // TODO: from soil or lake thermal model
  et_forcing.ground_heat_flux_W_per_sq_m   = set_et_forcing->ground_heat_flux_W_per_sq_m; \
                                             // TODO from soil thermal model.  Negative denotes downward.

  if(et_options.yes_aorc==TRUE)
  {
    et_params.wind_speed_measurement_height_m = set_et_params->wind_speed_measurement_height_m; \
                                                // AORC uses 10m.  Must convert to wind speed at 2 m height.
  }    
  et_params.humidity_measurement_height_m = set_et_params->humidity_measurement_height_m; 
  et_params.vegetation_height_m = set_et_params->vegetation_height_m; \
                                  // used for unit test of aerodynamic resistance used in Penman Monteith method.     
  et_params.zero_plane_displacement_height_m = set_et_params->zero_plane_displacement_height_m;  // 0.03 cm for unit testing
  et_params.momentum_transfer_roughness_length_m = set_et_params->momentum_transfer_roughness_length_m; \
                                                   // zero means that default values will be used in routine.
  et_params.heat_transfer_roughness_length_m = set_et_params->heat_transfer_roughness_length_m; \
                                               // zero means that default values will be used in routine.

  if(et_options.yes_aorc==TRUE)
  {
    // wind speed was measured at 10.0 m height, so we need to calculate the wind speed at 2.0m
    numerator=log(2.0/et_params.zero_plane_displacement_height_m);
    denominator=log(et_params.wind_speed_measurement_height_m/et_params.zero_plane_displacement_height_m);
    et_forcing.wind_speed_m_per_s = et_forcing.wind_speed_m_per_s*numerator/denominator;  // this is the 2 m value
    et_params.wind_speed_measurement_height_m=2.0;  // change because we converted from 10m to 2m height.
  }

  // surface radiation parameter values that are a function of land cover.   Must be assigned from land cover type.
  //---------------------------------------------------------------------------------------------------------------
  surf_rad_params.surface_longwave_emissivity = surface_rad_params->surface_longwave_emissivity; \
                                                // this is 1.0 for granular surfaces, maybe 0.97 for water
  surf_rad_params.surface_shortwave_albedo = surface_rad_params->surface_shortwave_albedo; \
                                             // this is a function of solar elev. angle for most surfaces.   

  if(et_options.yes_aorc==TRUE) 
  {
    // transfer aorc forcing data into our data structure for surface radiation calculations
    surf_rad_forcing.incoming_shortwave_radiation_W_per_sq_m = (double)aorc.incoming_shortwave_W_per_m2;
    surf_rad_forcing.incoming_longwave_radiation_W_per_sq_m  = (double)aorc.incoming_longwave_W_per_m2; 
    surf_rad_forcing.air_temperature_C                       = (double)aorc.air_temperature_2m_K-TK;
    // compute relative humidity from specific humidity..
    saturation_vapor_pressure_Pa = calc_air_saturation_vapor_pressure_Pa(surf_rad_forcing.air_temperature_C);
    actual_vapor_pressure_Pa = (double)aorc.specific_humidity_2m_kg_per_kg*(double)aorc.surface_pressure_Pa/0.622;
    surf_rad_forcing.relative_humidity_percent = 100.0*actual_vapor_pressure_Pa/saturation_vapor_pressure_Pa;
    // sanity check the resulting value.  Should be less than 100%.  Sometimes air can be supersaturated.
    if(100.0< surf_rad_forcing.relative_humidity_percent) surf_rad_forcing.relative_humidity_percent = 99.0;
  }
  else
  {
    // these values are needed if we don't have incoming longwave radiation measurements.
    surf_rad_forcing.incoming_shortwave_radiation_W_per_sq_m \
                     = surface_rad_forcing->incoming_shortwave_radiation_W_per_sq_m; \
                     // must come from somewhere
    surf_rad_forcing.incoming_longwave_radiation_W_per_sq_m \
                     = surface_rad_forcing->incoming_longwave_radiation_W_per_sq_m; \
                     // this huge negative value tells to calc.
    surf_rad_forcing.air_temperature_C \
                     = surface_rad_forcing->air_temperature_C;  // from some forcing data file
    surf_rad_forcing.relative_humidity_percent \
                     = surface_rad_forcing->relative_humidity_percent;  // from some forcing data file
    surf_rad_forcing.ambient_temperature_lapse_rate_deg_C_per_km \
                     = surface_rad_forcing->ambient_temperature_lapse_rate_deg_C_per_km; \
                     // ICAO standard atmosphere lapse rate
    surf_rad_forcing.cloud_cover_fraction = surface_rad_forcing->cloud_cover_fraction;  // from some forcing data file
    surf_rad_forcing.cloud_base_height_m  = surface_rad_forcing->cloud_base_height_m;   // assumed 2500 ft.
  }

  // Surface radiation forcing parameter values that must come from other models
  //---------------------------------------------------------------------------------------------------------------
  surf_rad_forcing.surface_skin_temperature_C = surface_rad_forcing->surface_skin_temperature_C;  // TODO from soil thermal model or vegetation model.

  if(et_options.shortwave_radiation_provided=FALSE)
  {
    // populate the elements of the structures needed to calculate shortwave (solar) radiation, and calculate it
    // ### OPTIONS ###
    solar_options.cloud_base_height_known=FALSE;  // set to TRUE if the solar_forcing.cloud_base_height_m is known.

    // ### PARAMS ###
    solar_params.latitude_degrees  = solar_rad_params->latitude_degrees;   // THESE VALUES ARE FOR THE UNIT TEST
    solar_params.longitude_degrees = solar_rad_params->longitude_degrees;  // THESE VALUES ARE FOR THE UNIT TEST
    solar_params.site_elevation_m  = solar_rad_params->site_elevation_m;   // THESE VALUES ARE FOR THE UNIT TEST  

    // ### FORCING ###
    solar_forcing.cloud_cover_fraction         = solar_rad_forcing->cloud_cover_fraction; // THESE VALUES ARE FOR THE UNIT TEST 
    solar_forcing.atmospheric_turbidity_factor = solar_rad_forcing->atmospheric_turbidity_factor; // 2.0 = clear mountain air, 5.0= smoggy air
    solar_forcing.day_of_year                  = solar_rad_forcing->day_of_year; // THESE VALUES ARE FOR THE UNIT TEST
    solar_forcing.zulu_time_h                  = solar_rad_forcing->zulu_time_h; // THESE VALUES ARE FOR THE UNIT TEST

    calculate_solar_radiation(&solar_options, &solar_params, &solar_forcing, &solar_results);
    surf_rad_forcing.incoming_shortwave_radiation_W_per_sq_m = 
            solar_results.solar_radiation_horizontal_cloudy_flux_W_per_sq_m;
  }
  
  // we must calculate the net radiation before calling the ET subroutine.
  if(et_options.use_aerodynamic_method==FALSE) 
  {
    // NOTE don't call this function use_aerodynamic_method option is TRUE
    et_forcing.net_radiation_W_per_sq_m=calculate_net_radiation_W_per_sq_m(&et_options,&surf_rad_params, 
                                                                         &surf_rad_forcing);
  }

  if(et_options.use_energy_balance_method ==TRUE)
    et_m_per_s=evapotranspiration_energy_balance_method(&et_options,&et_params,&et_forcing);
  if(et_options.use_aerodynamic_method ==TRUE)
    et_m_per_s=evapotranspiration_aerodynamic_method(&et_options,&et_params,&et_forcing,&inter_vars);
  if(et_options.use_combination_method ==TRUE)
    et_m_per_s=evapotranspiration_combination_method(&et_options,&et_params,&et_forcing,&inter_vars);
  if(et_options.use_priestley_taylor_method ==TRUE)
    et_m_per_s=evapotranspiration_priestley_taylor_method(&et_options,&et_params,&et_forcing,&inter_vars);
  if(et_options.use_penman_monteith_method ==TRUE)
    et_m_per_s=evapotranspiration_penman_monteith_method(&et_options,&et_params,&et_forcing,&inter_vars);

  if(et_options.use_energy_balance_method ==TRUE)   printf("energy balance method:\n");
  if(et_options.use_aerodynamic_method ==TRUE)      printf("aerodynamic method:\n");
  if(et_options.use_combination_method ==TRUE)      printf("combination method:\n");
  if(et_options.use_priestley_taylor_method ==TRUE) printf("Priestley-Taylor method:\n");
  if(et_options.use_penman_monteith_method ==TRUE)  printf("Penman Monteith method:\n");
                                                 
  printf("calculated instantaneous potential evapotranspiration (PET) =%8.6e m/s\n",et_m_per_s);
  printf("calculated instantaneous potential evapotranspiration (PET) =%8.6lf mm/d\n",et_m_per_s*86400.0*1000.0);

  return et_m_per_s;

}

#endif  // ET_WRAPPER_FUNCTION_H
