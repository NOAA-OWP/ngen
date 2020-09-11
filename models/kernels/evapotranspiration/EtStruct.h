#ifndef ET_STRUCT_H
#define ET_STRUCT_H

#define TRUE  1
#define FALSE 0

#define CP  1.006e+03  //  specific heat of air at constant pressure, J/(kg K), a physical constant.
#define KV2 0.1681     //  von Karman's constant squared, equal to 0.41 squared, unitless
#define TK  273.15     //  temperature in Kelvin at zero degree Celcius
#define SB  5.67e-08   //  stefan_boltzmann_constant in units of W/m^2/K^4

//DATA STRUCTURE TO HOLD AORC FORCING DATA
struct aorc_forcing_data
{
  // struct NAME                          DESCRIPTION                                            ORIGINAL AORC NAME     
  //____________________________________________________________________________________________________________________
  float precip_kg_per_m2;                // Surface precipitation "kg/m^2"                         | APCP_surface
  float incoming_longwave_W_per_m2 ;     // Downward Long-Wave Rad. Flux at 0m height, W/m^2       | DLWRF_surface
  float incoming_shortwave_W_per_m2;     // Downward Short-Wave Radiation Flux at 0m height, W/m^2 | DSWRF_surface
  float surface_pressure_Pa;             // Surface atmospheric pressure, Pa                       | PRES_surface
  float specific_humidity_2m_kg_per_kg;  // Specific Humidity at 2m height, kg/kg                  | SPFH_2maboveground
  float air_temperature_2m_K;            // Air temparture at 2m height, K                         | TMP_2maboveground
  float u_wind_speed_10m_m_per_s;        // U-component of Wind at 10m height, m/s                 | UGRD_10maboveground
  float v_wind_speed_10m_m_per_s;        // V-component of Wind at 10m height, m/s                 | VGRD_10maboveground
  float latitude;                        // degrees north of the equator.  Negative south          | latitude
  float longitude;                       // degrees east of prime meridian. Negative west          | longitude
  long int time; //TODO: type?           // seconds since 1970-01-01 00:00:00.0 0:00               | time
};

struct evapotranspiration_options  // these determine which method is applied to calculate ET.
{
  // element NAME                       DESCRIPTION
  //____________________________________________________________________________________________________________________
  int yes_aorc;                     // set to TRUE if forcing data come from AORC
  int shortwave_radiation_provided; // set to TRUE if yes_aorc==TRUE or if solar radiation inputs are provided

  // NOTE: these following options are exclusive.  Only one should be true for a particular catchment.
  int use_energy_balance_method;    // set to TRUE if using just the energy balance method for calculating PET
  int use_aerodynamic_method;       // set to TRUE if using just the aerodynamic method for calculating PET
  int use_combination_method;       // set to TRUE if using just the combination method for calculating PET
  int use_priestley_taylor_method;  // set to TRUE if using just the Priestley-Taylor method for calculating PET
  int use_penman_monteith_method;   // set to TRUE if using just the Penman Monteith method for calculating PET
};

struct evapotranspiration_params
{
  // element NAME                                    DESCRIPTION                                                
  //____________________________________________________________________________________________________________________
  double wind_speed_measurement_height_m;          // set to 0.0 if unknown, will default =2.0 [m] 
  double humidity_measurement_height_m;            // set to 0.0 if unknown, will default =2.0 [m]
  double vegetation_height_m;                      // TODO this should come from land cover data and a veg. height model
  double zero_plane_displacement_height_m;         // depends on surface roughness [m],
  double momentum_transfer_roughness_length_m;     // poorly defined.  If unknown, pass down 0.0 as a default
  double heat_transfer_roughness_length_m;         // poorly defined.  If unknown, pass down 0.0 as a default
  double latitude;                                 // could be used to adjust canopy resistance for seasonality
  double longitude;                                // could be used to adjust canopy resistance for seasonality
  int    day_of_year;                              // could be used to adjust canopy resistance for seasonality
};

struct evapotranspiration_forcing
{
  // element NAME                          DESCRIPTION                                                                  
  //___________________________________________________________________________________________________________________
  double net_radiation_W_per_sq_m;       // NOTE: ground heat flux is subtracted out in calculation subroutine
  double air_temperature_C;
  double relative_humidity_percent;      // this and specific_humidity_2m_kg_per_kg are redundant, so-  
  double specific_humidity_2m_kg_per_kg; // specify the missing one using a negative number, the other will be used
  double air_pressure_Pa;  
  double wind_speed_m_per_s;             // this is the value measured 2 m above the canopy
  double canopy_resistance_sec_per_m;    // depends on vegetation type and point in growing season 
  double water_temperature_C;            // if >100, assume 15 C.  Used to calculate latent heat of vaporization
  double ground_heat_flux_W_per_sq_m;    // from a model or assumed =0.  typically small on average
};

struct surface_radiation_params
{
  // element NAME                          DESCRIPTION                                                                  
  //____________________________________________________________________________________________________________________
  double surface_longwave_emissivity;  // dimensionless (0-1) < 1.0 for water and snow, all other surfaces = 1.0
  double surface_shortwave_albedo;     // dimensionless (0-1) from land cover.  Dynamic seasonally
};

struct surface_radiation_forcing
{
  // element NAME                          DESCRIPTION                                                                  
  //____________________________________________________________________________________________________________________
  double incoming_shortwave_radiation_W_per_sq_m;  // TODO could be calculated if unavailable, but not now.
  double incoming_longwave_radiation_W_per_sq_m;  // set to a large negative number if unknown (e.g. -1.0e-5)
  double air_temperature_C;            // usually value at 2.0 m., maybe 30 m if from WRF
  double relative_humidity_percent;    // usually value at 2.0 m., maybe 30 m if from WRF
  double surface_skin_temperature_C;   // from a model or assumed...  smartly.  could be soil/rock, veg., snow, water
  double ambient_temperature_lapse_rate_deg_C_per_km;  // This is a standard WRF output.  Typ. 6.49 K/km ICAO std. atm.
  double cloud_cover_fraction;         // dimensionless (0-1).  This should be a WRF output.
  double cloud_base_height_m;          // the height from ground to bottom of clouds in m.  From WRF output
  double atmospheric_turbidity_factor; // Linke turbidity factor needed iff et_options.shortwave_radiation_provided=FALSE
  int    day_of_year;
  double zulu_time;                    // (0.0-23.999999) hours
};

struct solar_radiation_forcing
{
  // element NAME                          DESCRIPTION                                                                  
  //____________________________________________________________________________________________________________________
  double cloud_cover_fraction;          // dimensionless (0-1).  1.0= overcast.  varies hourly
  double cloud_base_height_m;           // not used if solar_radiation_options->cloud_base_height_known == FALSE
  double atmospheric_turbidity_factor;  // Linke turbidity factor.  This dimensionless (2-5) parameter varies daily.
                                      // Typ. not available: 2.0=clear mountain air, 2.5-3.5 normal, 5.0=v. smoggy air
  double day_of_year;                   // this is a number from 1 to 365 or 366 if leap year
  double zulu_time_h;                   // decimal hours, 0-23.999999999
};

struct solar_radiation_options
{
  // element NAME                          DESCRIPTION
  //____________________________________________________________________________________________________________________
  int cloud_base_height_known;   // set this to TRUE to use the default values from the Bras textbook.
};

struct solar_radiation_parameters
{
  // element NAME                          DESCRIPTION                                                                  
  //____________________________________________________________________________________________________________________
  double latitude_degrees;       // positive north of the equator, negative south
  double longitude_degrees;      // negative west of prime meridian, positive east
  double site_elevation_m;       // elevation of the observer, m
};

struct solar_radiation_results
{
  // element NAME                          DESCRIPTION                                                                  
  //____________________________________________________________________________________________________________________
  double solar_radiation_flux_W_per_sq_m;            // on a plane perpendicular to the earth-sun line
  double solar_radiation_horizontal_flux_W_per_sq_m; // on a horizontal plane tangent to earth
  double solar_radiation_cloudy_flux_W_per_sq_m;     // on a plane perpendicular to the earth-sun line
  double solar_radiation_horizontal_cloudy_flux_W_per_sq_m; // on a horizontal plane tangent to earth considering clouds
  double solar_elevation_angle_degrees;              // height of the sun above (+) or below (-) horizon, degrees.
  double solar_azimuth_angle_degrees;                // azimuth pointing towards the sun, degrees (0-360)
  double solar_local_hour_angle_degrees;             // local hour angle (deg.) to the sun, negative=a.m., positive=p.m.
};

struct intermediate_vars
{
  // element NAME                       DESCRIPTION
  //____________________________________________________________________________________________________________________
  double liquid_water_density_kg_per_m3;       // rho_w
  double water_latent_heat_of_vaporization_J_per_kg;    // eqn 2.7.6 Chow etal., // aka 'lambda'
  double air_saturation_vapor_pressure_Pa;
  double air_actual_vapor_pressure_Pa;
  double vapor_pressure_deficit_Pa;            // VPD
  double moist_air_gas_constant_J_per_kg_K;    // R_a
  double moist_air_density_kg_per_m3;          // rho_a
  double slope_sat_vap_press_curve_Pa_s;       // delta
  //double water_latent_heat_of_vaporization_J_per_kg;
  double psychrometric_constant_Pa_per_C;      // gamma
};

#endif // ET_STRUCT_H
