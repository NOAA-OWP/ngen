#ifndef Et_Set_Params_H
#define Et_Set_Params_H

extern void et_setup(aorc_forcing_data          &aorc_forcing,
                     solar_radiation_options    &solar_options,
                     solar_radiation_parameters &solar_params,
                     solar_radiation_forcing    &solar_forcing,
                     evapotranspiration_options &et_options,
                     evapotranspiration_params  &et_params,
                     evapotranspiration_forcing &et_forcing,
                     surface_radiation_params   &surf_rad_params,
                     surface_radiation_forcing  &surf_rad_forcing);

extern void et_setup(aorc_forcing_data          &aorc_forcing,
                     solar_radiation_options    &solar_options,
                     solar_radiation_parameters &solar_params,
                     solar_radiation_forcing    &solar_forcing,
                     evapotranspiration_options &et_options,
                     evapotranspiration_params  &et_params,
                     evapotranspiration_forcing &et_forcing,
                     surface_radiation_params   &surf_rad_params,
                     surface_radiation_forcing  &surf_rad_forcing)
{
  // FLAGS
  int yes_aorc; // if TRUE then using AORC forcing data- if FALSE then we must calculate incoming short/longwave rad.
  int yes_wrf;  // if TRUE then we get radiation winds etc. from WRF output.  TODO not implemented.

  double wind_speed_at_2_m;
  double et_mm_per_d;
  double saturation_vapor_pressure_Pa;
  double actual_vapor_pressure_Pa;

  //###################################################################################################
  // THE VALUE OF THESE FLAGS DETERMINE HOW THIS CODE BEHAVES.  CYCLE THROUGH THESE FOR THE UNIT TEST.
  //###################################################################################################
  // Set this flag to TRUE if meteorological inputs come from AORC
  et_options.yes_aorc = TRUE;                      // if TRUE, it means that we are using AORC data.
  et_options.shortwave_radiation_provided = FALSE;

  //###################################################################################################
  // MAKE UP SOME TYPICAL AORC DATA.  THESE VALUES DRIVE THE UNIT TESTS.
  //###################################################################################################
  //read_aorc_data().  TODO: These data come from some aorc reading/parsing function.
  //---------------------------------------------------------------------------------------------------------------

  aorc_forcing.incoming_longwave_W_per_m2     =  117.1;
  aorc_forcing.incoming_shortwave_W_per_m2    =  599.7;
  aorc_forcing.surface_pressure_Pa            =  101300.0;
  aorc_forcing.specific_humidity_2m_kg_per_kg =  0.00778;      // results in a relative humidity of 40%
  aorc_forcing.air_temperature_2m_K           =  25.0+TK;
  aorc_forcing.u_wind_speed_10m_m_per_s       =  1.54;
  aorc_forcing.v_wind_speed_10m_m_per_s       =  3.2;
  aorc_forcing.latitude                       =  37.865211;
  aorc_forcing.longitude                      =  -98.12345;
  aorc_forcing.time                           =  111111112;


  // populate the evapotranspiration forcing data structure:
  // this part of code does not explicitly setting values, moved to et_wrapper_function()


  // ET forcing values that come from somewhere else...
  //---------------------------------------------------------------------------------------------------------------
  et_forcing.canopy_resistance_sec_per_m   = 50.0; // TODO: from plant growth model
  et_forcing.water_temperature_C           = 15.5; // TODO: from soil or lake thermal model
  et_forcing.ground_heat_flux_W_per_sq_m=-10.0;    // TODO from soil thermal model.  Negative denotes downward.

  if(et_options.yes_aorc==TRUE)
  {
    et_params.wind_speed_measurement_height_m=10.0;  // AORC uses 10m.  Must convert to wind speed at 2 m height.
  }    
  et_params.humidity_measurement_height_m=2.0; 
  et_params.vegetation_height_m=0.12;   // used for unit test of aerodynamic resistance used in Penman Monteith method.     
  et_params.zero_plane_displacement_height_m=0.0003;  // 0.03 cm for unit testing
  et_params.momentum_transfer_roughness_length_m=0.0;  // zero means that default values will be used in routine.
  et_params.heat_transfer_roughness_length_m=0.0;      // zero means that default values will be used in routine.


  // surface radiation parameter values that are a function of land cover.   Must be assigned from land cover type.
  //---------------------------------------------------------------------------------------------------------------
  surf_rad_params.surface_longwave_emissivity=1.0; // this is 1.0 for granular surfaces, maybe 0.97 for water
  surf_rad_params.surface_shortwave_albedo=0.22;  // this is a function of solar elev. angle for most surfaces.   


  if(et_options.yes_aorc!=TRUE)
  {
    // these values are needed if we don't have incoming longwave radiation measurements.
    surf_rad_forcing.incoming_shortwave_radiation_W_per_sq_m     = 440.1;     // must come from somewhere
    surf_rad_forcing.incoming_longwave_radiation_W_per_sq_m      = -1.0e+05;  // this huge negative value tells to calc.
    surf_rad_forcing.air_temperature_C                           = 15.0;      // from some forcing data file
    surf_rad_forcing.relative_humidity_percent                   = 63.0;      // from some forcing data file
    surf_rad_forcing.ambient_temperature_lapse_rate_deg_C_per_km = 6.49;      // ICAO standard atmosphere lapse rate
    surf_rad_forcing.cloud_cover_fraction                        = 0.6;       // from some forcing data file
    surf_rad_forcing.cloud_base_height_m                         = 2500.0/3.281; // assumed 2500 ft.
  }

    // these values are needed if we don't have incoming longwave radiation measurements.
    surf_rad_forcing.incoming_shortwave_radiation_W_per_sq_m     = 440.1;     // must come from somewhere
    surf_rad_forcing.incoming_longwave_radiation_W_per_sq_m      = -1.0e+05;  // this huge negative value tells to calc.
    surf_rad_forcing.air_temperature_C                           = 15.0;      // from some forcing data file
    surf_rad_forcing.relative_humidity_percent                   = 63.0;      // from some forcing data file
    surf_rad_forcing.ambient_temperature_lapse_rate_deg_C_per_km = 6.49;      // ICAO standard atmosphere lapse rate
    surf_rad_forcing.cloud_cover_fraction                        = 0.6;       // from some forcing data file
    surf_rad_forcing.cloud_base_height_m                         = 2500.0/3.281; // assumed 2500 ft.
    

  // Surface radiation forcing parameter values that must come from other models
  //---------------------------------------------------------------------------------------------------------------
  surf_rad_forcing.surface_skin_temperature_C = 12.0;  // TODO from soil thermal model or vegetation model.

  if(et_options.shortwave_radiation_provided=FALSE)
  {
    // populate the elements of the structures needed to calculate shortwave (solar) radiation, and calculate it
    // ### OPTIONS ###
    solar_options.cloud_base_height_known=FALSE;  // set to TRUE if the solar_forcing.cloud_base_height_m is known.

    // ### PARAMS ###
    solar_params.latitude_degrees      =  37.25;   // THESE VALUES ARE FOR THE UNIT TEST
    solar_params.longitude_degrees     = -97.5554; // THESE VALUES ARE FOR THE UNIT TEST
    solar_params.site_elevation_m      = 303.333;  // THESE VALUES ARE FOR THE UNIT TEST  

    // ### FORCING ###
    solar_forcing.cloud_cover_fraction         =   0.5;   // THESE VALUES ARE FOR THE UNIT TEST 
    solar_forcing.atmospheric_turbidity_factor =   2.0;   // 2.0 = clear mountain air, 5.0= smoggy air
    solar_forcing.day_of_year                  =  208;    // THESE VALUES ARE FOR THE UNIT TEST
    solar_forcing.zulu_time_h                  =  20.567; // THESE VALUES ARE FOR THE UNIT TEST

    // UNIT TEST RESULTS
    // CALCULATED SOLAR FLUXES
    // at time:     20.56700000 UTC
    // at site latitude: 37.250000 deg. longitude:-97.555400 deg.  elevation:303.333000 m
    // Shortwave radiation clear-sky flux calculations:
    // -above canopy/snow perpendicular to Earth-Sun line is      =964.56166277 W/m2
    // -at the top of a horizontal canopy/snow surface is:        =661.40396086 W/m2
    // Shortwave radiation clear-sky flux calculations with 0.5000 cloud cover fraction:
    // -above canopy/snow perpendicular to Earth-Sun line is      =807.82039257 W/m2
    // -at the top of a horizontal canopy/snow surface is:        =553.92581722 W/m2
    // CALCULATED ANGLES DESCRIBING VECTOR POINTING TO THE SUN
    // solar elevation angle:     43.29101185 degrees
    // solar azimuth:            225.06371958 degrees
    // local hour angle:          31.01549773 degrees
    // Number of tests passed=7 of 7.
    // UNIT TEST PASSED.
  }

  return;
}

#endif // Et_Set_Params_H
