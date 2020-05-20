#include <iostream>
#include <cmath>
#include <cstring>

#define TRUE  1
#define FALSE 0

#define CP  1.006e+03  //  specific heat of air at constant pressure, J/(kg K), a physical constant.
#define KV2 0.1681     //  von Karman's constant squared, equal to 0.41 squared, unitless
#define TK  273.15     //  temperature in Kelvin at zero degree Celcius
#define SB  5.67e-08   //  stefan_boltzmann_constant in units of W/m^2/K^4

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
} ;

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

//####################################
// FUNCTION AND SUBROUTINE PROTOTYPES

extern double evapotranspiration_energy_balance_method
(
evapotranspiration_options *et_options,
evapotranspiration_params *et_params,
evapotranspiration_forcing *et_forcing
);

extern double evapotranspiration_aerodynamic_method
(
evapotranspiration_options *et_options,
evapotranspiration_params *et_params,
evapotranspiration_forcing *et_forcing,
intermediate_vars *inter_vars
);

extern double evapotranspiration_combination_method
(
evapotranspiration_options *et_options,
evapotranspiration_params *et_params,
evapotranspiration_forcing *et_forcing,
intermediate_vars *inter_vars
);

extern double evapotranspiration_priestley_taylor_method
(
evapotranspiration_options *et_options,
evapotranspiration_params *et_params,
evapotranspiration_forcing *et_forcing,
intermediate_vars *inter_vars
);

extern double evapotranspiration_penman_monteith_method
(
evapotranspiration_options *et_options,
evapotranspiration_params *et_params,
evapotranspiration_forcing *et_forcing,
intermediate_vars *inter_vars
);

extern double calculate_net_radiation_W_per_sq_m 
(
evapotranspiration_options *opts,   // needed to tell if using aorc forcing values. Could be other options too.
surface_radiation_params   *pars,
surface_radiation_forcing  *forc
);

extern double calculate_aerodynamic_resistance
(
double wind_speed_measurement_height_m,      // default =2.0 [m] 
double humidity_measurement_height_m,        // default =2.0 [m],
double zero_plane_displacement_height_m,     // depends on surface roughness [m],
double momentum_transfer_roughness_length_m, // [m],
double heat_transfer_roughness_length_m,     // [m],
double wind_speed_m_per_s                    // [m s-1].
);

extern void calculate_solar_radiation
(
solar_radiation_options *options, 
solar_radiation_parameters *params,
solar_radiation_forcing *forcing, 
solar_radiation_results *results
);

extern double penman_monteith_et_calculation
(
double delta,
double gamma,
double moist_air_density_kg_per_m3,
double vapor_pressure_deficit_Pa,
evapotranspiration_params *surf_rad_params,
evapotranspiration_forcing *et_forcing
);

extern void calculate_intermediate_variables
(
evapotranspiration_options *et_options,
evapotranspiration_params *et_params,
evapotranspiration_forcing *et_forcing,
intermediate_vars *inter_vars
);

extern int is_fabs_less_than_epsilon(double a,double epsilon);  // returns TRUE iff fabs(a)<epsilon

extern double calc_air_saturation_vapor_pressure_Pa(double air_temperature_C);

extern double calc_slope_of_air_saturation_vapor_pressure_Pa_per_C(double air_temperature_C);

extern double calc_liquid_water_density_kg_per_m3(double water_temperature_C);

extern int is_fabs_less_than_epsilon(double a,double epsilon);  // returns TRUE iff fabs(a)<epsilon

//############################################################*
// subroutine to calculate evapotranspiration using           *
// Chow, Maidment, and Mays textbook                          *
// F.L. Ogden, NOAA National Weather Service, 2020            *
//############################################################*
extern double evapotranspiration_energy_balance_method
(
evapotranspiration_options *et_options,
evapotranspiration_params *et_params,
evapotranspiration_forcing *et_forcing
)
{
// local varibles
double water_latent_heat_of_vaporization_J_per_kg;
double liquid_water_density_kg_per_m3;
double lambda_et;
double radiation_balance_evapotranspiration_rate_m_per_s;

// from FAO document: cp specific heat at constant pressure, 1.013 10-3 [MJ kg-1 °C-1],

// IF SOIL WATER TEMPERATURE NOT PROVIDED, USE A SANE VALUE
if(100.0 > et_forcing->water_temperature_C) et_forcing->water_temperature_C=22.0; // growing season

// CALCULATE VARS NEEDED FOR THE ALL METHODS:

liquid_water_density_kg_per_m3 = calc_liquid_water_density_kg_per_m3(et_forcing->water_temperature_C); // rho_w

water_latent_heat_of_vaporization_J_per_kg=2.501e+06-2370.0*et_forcing->water_temperature_C;  // eqn 2.7.6 Chow etal. 
                                                                                              // aka 'lambda'

// We need this in all options except for aerodynamic or Penman-Monteith methods.
// Radiation balance is the simplest method.  Involves only radiation calculations, no aerodynamic calculations.

lambda_et=0.0;
if( (et_options->use_aerodynamic_method == FALSE ) && (et_options->use_penman_monteith_method==FALSE) )
  {
  // This is equation 3.5.9 from Chow, Maidment, and Mays textbook.
  lambda_et=et_forcing->net_radiation_W_per_sq_m;
  radiation_balance_evapotranspiration_rate_m_per_s=lambda_et/
                                (liquid_water_density_kg_per_m3*water_latent_heat_of_vaporization_J_per_kg);
  }
return(radiation_balance_evapotranspiration_rate_m_per_s);
}

//############################################################*
// subroutine to calculate evapotranspiration using           *
// Chow, Maidment, and Mays textbook                          *
// F.L. Ogden, NOAA National Weather Service, 2020            *
//############################################################*
extern double evapotranspiration_aerodynamic_method
(
evapotranspiration_options *et_options,
evapotranspiration_params *et_params,
evapotranspiration_forcing *et_forcing,
intermediate_vars *inter_vars
)
{
// local varibles
double psychrometric_constant_Pa_per_C;
double slope_sat_vap_press_curve_Pa_s;
double air_saturation_vapor_pressure_Pa;
double air_actual_vapor_pressure_Pa;
double moist_air_density_kg_per_m3;
double water_latent_heat_of_vaporization_J_per_kg;
double moist_air_gas_constant_J_per_kg_K;
double vapor_pressure_deficit_Pa;
double liquid_water_density_kg_per_m3;
double aerodynamic_method_evapotranspiration_rate_m_per_s;
double mass_flux;
double von_karman_constant_squared=(double)KV2;  // a constant equal to 0.41 squared

calculate_intermediate_variables(et_options, et_params, et_forcing, inter_vars);

liquid_water_density_kg_per_m3 = inter_vars->liquid_water_density_kg_per_m3;
water_latent_heat_of_vaporization_J_per_kg=inter_vars->water_latent_heat_of_vaporization_J_per_kg;
air_saturation_vapor_pressure_Pa=inter_vars->air_saturation_vapor_pressure_Pa;
air_actual_vapor_pressure_Pa=inter_vars->air_actual_vapor_pressure_Pa;
vapor_pressure_deficit_Pa=inter_vars->vapor_pressure_deficit_Pa;
moist_air_gas_constant_J_per_kg_K=inter_vars->moist_air_gas_constant_J_per_kg_K;
moist_air_density_kg_per_m3=inter_vars->moist_air_density_kg_per_m3;
slope_sat_vap_press_curve_Pa_s=inter_vars->slope_sat_vap_press_curve_Pa_s;
psychrometric_constant_Pa_per_C=inter_vars->psychrometric_constant_Pa_per_C;

if( et_options->use_penman_monteith_method == FALSE)  // we don't use this term in Penman-Monteith method
  {
  // This is equation 3.5.16 from Chow, Maidment, and Mays textbook.
  mass_flux = 0.622*von_karman_constant_squared*moist_air_density_kg_per_m3*      // kg per sq. meter per sec.
              vapor_pressure_deficit_Pa*et_forcing->wind_speed_m_per_s/
              (et_forcing->air_pressure_Pa*
              pow(log(et_params->wind_speed_measurement_height_m/et_params->zero_plane_displacement_height_m),2.0));

  aerodynamic_method_evapotranspiration_rate_m_per_s=mass_flux/liquid_water_density_kg_per_m3;  
  }

return(aerodynamic_method_evapotranspiration_rate_m_per_s);
}

//############################################################*
// subroutine to calculate evapotranspiration using           *
// Chow, Maidment, and Mays textbook                          *
// F.L. Ogden, NOAA National Weather Service, 2020            *
//############################################################*
extern double evapotranspiration_combination_method
(
evapotranspiration_options *et_options,
evapotranspiration_params *et_params,
evapotranspiration_forcing *et_forcing,
intermediate_vars *inter_vars
)
{
// local varibles
double psychrometric_constant_Pa_per_C;
double slope_sat_vap_press_curve_Pa_s;
double air_saturation_vapor_pressure_Pa;
double air_actual_vapor_pressure_Pa;
double moist_air_density_kg_per_m3;
double water_latent_heat_of_vaporization_J_per_kg;
double moist_air_gas_constant_J_per_kg_K;
double vapor_pressure_deficit_Pa;
double liquid_water_density_kg_per_m3;
double lambda_et;
double radiation_balance_evapotranspiration_rate_m_per_s;
double aerodynamic_method_evapotranspiration_rate_m_per_s;
double instantaneous_et_rate_m_per_s;
double mass_flux;
double delta;
double gamma;
double von_karman_constant_squared=(double)KV2;  // a constant equal to 0.41 squared

calculate_intermediate_variables(et_options, et_params, et_forcing, inter_vars);

liquid_water_density_kg_per_m3 = inter_vars->liquid_water_density_kg_per_m3;
water_latent_heat_of_vaporization_J_per_kg=inter_vars->water_latent_heat_of_vaporization_J_per_kg;
air_saturation_vapor_pressure_Pa=inter_vars->air_saturation_vapor_pressure_Pa;
air_actual_vapor_pressure_Pa=inter_vars->air_actual_vapor_pressure_Pa;
vapor_pressure_deficit_Pa=inter_vars->vapor_pressure_deficit_Pa;
moist_air_gas_constant_J_per_kg_K=inter_vars->moist_air_gas_constant_J_per_kg_K;
moist_air_density_kg_per_m3=inter_vars->moist_air_density_kg_per_m3;
slope_sat_vap_press_curve_Pa_s=inter_vars->slope_sat_vap_press_curve_Pa_s;
water_latent_heat_of_vaporization_J_per_kg=inter_vars->water_latent_heat_of_vaporization_J_per_kg;
psychrometric_constant_Pa_per_C=inter_vars->psychrometric_constant_Pa_per_C;

delta=slope_sat_vap_press_curve_Pa_s;
gamma=psychrometric_constant_Pa_per_C;

if(et_options->use_combination_method==TRUE)
lambda_et=0.0;
if( (et_options->use_aerodynamic_method == FALSE ) && (et_options->use_penman_monteith_method==FALSE) )
  {
  // This is equation 3.5.9 from Chow, Maidment, and Mays textbook.
  lambda_et=et_forcing->net_radiation_W_per_sq_m;
  radiation_balance_evapotranspiration_rate_m_per_s=lambda_et/
                                      (liquid_water_density_kg_per_m3*water_latent_heat_of_vaporization_J_per_kg);
  mass_flux = 0.622*von_karman_constant_squared*moist_air_density_kg_per_m3*      // kg per sq. meter per sec.
              vapor_pressure_deficit_Pa*et_forcing->wind_speed_m_per_s/
              (et_forcing->air_pressure_Pa*
              pow(log(et_params->wind_speed_measurement_height_m/et_params->zero_plane_displacement_height_m),2.0));

  aerodynamic_method_evapotranspiration_rate_m_per_s=mass_flux/liquid_water_density_kg_per_m3;
  }
  {
  // This is equation 3.5.26 from Chow, Maidment, and Mays textbook
  instantaneous_et_rate_m_per_s=
                    delta/(delta+gamma)*radiation_balance_evapotranspiration_rate_m_per_s+
                    gamma/(delta+gamma)*aerodynamic_method_evapotranspiration_rate_m_per_s;
  }
return (instantaneous_et_rate_m_per_s);
}

//############################################################*
// subroutine to calculate evapotranspiration using           *
// Chow, Maidment, and Mays textbook                          *
// F.L. Ogden, NOAA National Weather Service, 2020            *
//############################################################*
extern double evapotranspiration_priestley_taylor_method
(
evapotranspiration_options *et_options,
evapotranspiration_params *et_params,
evapotranspiration_forcing *et_forcing,
intermediate_vars *inter_vars
)
{
// local varibles
double psychrometric_constant_Pa_per_C;
double slope_sat_vap_press_curve_Pa_s;
double air_saturation_vapor_pressure_Pa;
double air_actual_vapor_pressure_Pa;
double moist_air_density_kg_per_m3;
double water_latent_heat_of_vaporization_J_per_kg;
double moist_air_gas_constant_J_per_kg_K;
double moist_air_specific_humidity_kg_per_m3;
double vapor_pressure_deficit_Pa;
double liquid_water_density_kg_per_m3;
double lambda_et;
double radiation_balance_evapotranspiration_rate_m_per_s;
double instantaneous_et_rate_m_per_s;
double mass_flux;
double delta;
double gamma;

calculate_intermediate_variables(et_options, et_params, et_forcing, inter_vars);

liquid_water_density_kg_per_m3 = inter_vars->liquid_water_density_kg_per_m3;
water_latent_heat_of_vaporization_J_per_kg=inter_vars->water_latent_heat_of_vaporization_J_per_kg;
air_saturation_vapor_pressure_Pa=inter_vars->air_saturation_vapor_pressure_Pa;
air_actual_vapor_pressure_Pa=inter_vars->air_actual_vapor_pressure_Pa;
vapor_pressure_deficit_Pa=inter_vars->vapor_pressure_deficit_Pa;
moist_air_gas_constant_J_per_kg_K=inter_vars->moist_air_gas_constant_J_per_kg_K;
moist_air_density_kg_per_m3=inter_vars->moist_air_density_kg_per_m3;
slope_sat_vap_press_curve_Pa_s=inter_vars->slope_sat_vap_press_curve_Pa_s;
water_latent_heat_of_vaporization_J_per_kg=inter_vars->water_latent_heat_of_vaporization_J_per_kg;
psychrometric_constant_Pa_per_C=inter_vars->psychrometric_constant_Pa_per_C;

delta=slope_sat_vap_press_curve_Pa_s;
gamma=psychrometric_constant_Pa_per_C;

lambda_et=0.0;
if( (et_options->use_aerodynamic_method == FALSE ) && (et_options->use_penman_monteith_method==FALSE) )
  {
  // This is equation 3.5.9 from Chow, Maidment, and Mays textbook.
  lambda_et=et_forcing->net_radiation_W_per_sq_m;
  radiation_balance_evapotranspiration_rate_m_per_s=lambda_et/
                                      (liquid_water_density_kg_per_m3*water_latent_heat_of_vaporization_J_per_kg);
  }
  instantaneous_et_rate_m_per_s=1.3*delta/(delta+gamma)*radiation_balance_evapotranspiration_rate_m_per_s;
return(instantaneous_et_rate_m_per_s);
}
  
//############################################################*
// subroutine to calculate evapotranspiration using Penman-   *
// Monteith FAO reference ET procedure.                       *
// Reference: http://www.fao.org/3/X0490E/x0490e06.htm        *
// F.L. Ogden, NOAA National Weather Service, 2020            *
//############################################################*
extern double evapotranspiration_penman_monteith_method
(
evapotranspiration_options *et_options,
evapotranspiration_params *et_params,
evapotranspiration_forcing *et_forcing,
intermediate_vars *inter_vars
)
{
// local varibles
double instantaneous_et_rate_m_per_s;
double psychrometric_constant_Pa_per_C;
double slope_sat_vap_press_curve_Pa_s;
double air_saturation_vapor_pressure_Pa;
double air_actual_vapor_pressure_Pa;
double moist_air_density_kg_per_m3;
double water_latent_heat_of_vaporization_J_per_kg;
double moist_air_gas_constant_J_per_kg_K;
double moist_air_specific_humidity_kg_per_m3;
double vapor_pressure_deficit_Pa;
double liquid_water_density_kg_per_m3;
double lambda_et;
double delta;
double gamma;

calculate_intermediate_variables(et_options, et_params, et_forcing, inter_vars);

liquid_water_density_kg_per_m3 = inter_vars->liquid_water_density_kg_per_m3;
water_latent_heat_of_vaporization_J_per_kg=inter_vars->water_latent_heat_of_vaporization_J_per_kg;
air_saturation_vapor_pressure_Pa=inter_vars->air_saturation_vapor_pressure_Pa;
air_actual_vapor_pressure_Pa=inter_vars->air_actual_vapor_pressure_Pa;
vapor_pressure_deficit_Pa=inter_vars->vapor_pressure_deficit_Pa;
moist_air_gas_constant_J_per_kg_K=inter_vars->moist_air_gas_constant_J_per_kg_K;
moist_air_density_kg_per_m3=inter_vars->moist_air_density_kg_per_m3;
slope_sat_vap_press_curve_Pa_s=inter_vars->slope_sat_vap_press_curve_Pa_s;
water_latent_heat_of_vaporization_J_per_kg=inter_vars->water_latent_heat_of_vaporization_J_per_kg;
psychrometric_constant_Pa_per_C=inter_vars->psychrometric_constant_Pa_per_C;

delta=slope_sat_vap_press_curve_Pa_s;
gamma=psychrometric_constant_Pa_per_C;

if(et_options->use_penman_monteith_method==TRUE)
  {
  lambda_et = penman_monteith_et_calculation(delta,gamma,moist_air_density_kg_per_m3,vapor_pressure_deficit_Pa,
                                             et_params,et_forcing);
  }

instantaneous_et_rate_m_per_s= lambda_et/(liquid_water_density_kg_per_m3*water_latent_heat_of_vaporization_J_per_kg);

return(instantaneous_et_rate_m_per_s);  // meters per second

}

//#####################################################################################*
// subroutine to calculate latent heat flux (lambda*et)                                *
// using the Penman-Monteith equation as described by FAO                              *
//   see:   http://www.fao.org/3/X0490E/x0490e06.htm#aerodynamic%20resistance%20(ra)   *
// After Allen et al. ASCE Reference ET calculation method                             *
// F.L. Ogden, NOAA National Weather Service, 2020                                     *
//#####################################################################################*
extern double penman_monteith_et_calculation
(
double delta,
double gamma,
double moist_air_density_kg_per_m3,
double vapor_pressure_deficit_Pa,
evapotranspiration_params *et_params,
evapotranspiration_forcing *et_forcing
)

{
// local varibles
double numerator;
double denominator;
double aerodynamic_resistance_s_per_m;

// this method requires more calculations 
if(is_fabs_less_than_epsilon(et_params->vegetation_height_m,1.0e-06)==TRUE)
  {
  // the vegetation height was not specified.  TODO should warn??
  fprintf(stderr,"WARNING: Vegetation height not specified in the Penman-Monteith routine.  Using 0.5m.\n");
  et_params->vegetation_height_m=0.5;  // use a reasonable assumed value
  }
  
// use approximations from UN FAO: http://www.fao.org/3/X0490E/x0490e06.htm#aerodynamic%20resistance%20(ra)
et_params->zero_plane_displacement_height_m=2.0/3.0*et_params->vegetation_height_m;
et_params->momentum_transfer_roughness_length_m=0.123*et_params->vegetation_height_m;
et_params->heat_transfer_roughness_length_m=0.1*et_params->vegetation_height_m;

aerodynamic_resistance_s_per_m= calculate_aerodynamic_resistance
                                ( et_params->wind_speed_measurement_height_m, 
                                  et_params->humidity_measurement_height_m,
                                  et_params->zero_plane_displacement_height_m, 
                                  et_params->momentum_transfer_roughness_length_m,
                                  et_params->heat_transfer_roughness_length_m, 
                                  et_forcing->wind_speed_m_per_s);

// all the ingredients have been prepared.  Make Penman-Monteith soufle...
// from: http://www.fao.org/3/X0490E/x0490e06.htm#aerodynamic%20resistance%20(ra)

numerator = delta* (et_forcing->net_radiation_W_per_sq_m - et_forcing->ground_heat_flux_W_per_sq_m) + 
            moist_air_density_kg_per_m3 * CP *
            vapor_pressure_deficit_Pa/aerodynamic_resistance_s_per_m;

denominator = delta + gamma * (1.0+et_forcing->canopy_resistance_sec_per_m/aerodynamic_resistance_s_per_m);
          
return(numerator/denominator);  // Latent heat flux in Watts per sq. m., or J per (s m2)
}

//############################################################*
// subroutine to calculate net radiation from all components  *
// of the radiation budget, using values provided by WRF      *
// Reference, Bras, R.L., HYDROLOGY an Introduction to        *
// Hydrologic Science, Addison Wesley, 1990.                  *
// F.L. Ogden, NOAA National Weather Service, 2020            *
//############################################################*
extern double calculate_net_radiation_W_per_sq_m 
(
evapotranspiration_options *et_options,   //  et_options.yes_aorc == TRUE if we are using aorc forcing values.
surface_radiation_params *surf_rad_params,
surface_radiation_forcing *surf_rad_forcing
)

{
// local variables 
double net_radiation_W_per_sq_m;
double outgoing_longwave_radiation_W_per_sq_m;
double stefan_boltzmann_constant = SB;         //W/m^2/K^4
double atmosphere_longwave_emissivity;             // dimensionless, based on water vapor content of air
double saturation_water_vapor_partial_pressure_Pa;
double actual_water_vapor_partial_pressure_Pa;
double actual_water_vapor_mixing_ratio;
double cloud_base_temperature_C; 
double surface_longwave_albedo;
double N,Klw;


  
// CALCULATE OUTGOING LONGWAVE RADIATION FLUX FROM SURFACE
outgoing_longwave_radiation_W_per_sq_m=surf_rad_params->surface_longwave_emissivity*stefan_boltzmann_constant*
                                       pow(surf_rad_forcing->surface_skin_temperature_C+TK,4.0); 
                                       // must convert C to K

if(0.999 < surf_rad_params->surface_longwave_emissivity) 
  {
  surface_longwave_albedo=0.0;    // soil, rock, concrete, asphalt, vegetation, snow
  }
else 
  { 
  surface_longwave_albedo=0.03;   // water - actually not this simple, but close enough for now
  }
  
if(et_options->yes_aorc==FALSE)  // we must calculate longwave incoming from the atmosphere 
saturation_water_vapor_partial_pressure_Pa=calc_air_saturation_vapor_pressure_Pa(surf_rad_forcing->air_temperature_C); 

actual_water_vapor_partial_pressure_Pa=surf_rad_forcing->relative_humidity_percent/100.0*
                                       saturation_water_vapor_partial_pressure_Pa;

if(et_options->yes_aorc==FALSE)
  {
  // CALCULATE DOWNWELLING LONGWAVE RADIATION FLUX FROM ATMOSPHERE, W/m2.
  if(0.90 < surf_rad_forcing->cloud_cover_fraction) // very nearly overcast or overcast
    {
    // calculate longwave downwelling using overcast equation, with emissivity of cloud base =1.0
    cloud_base_temperature_C=surf_rad_forcing->air_temperature_C+
                             surf_rad_forcing->ambient_temperature_lapse_rate_deg_C_per_km*
                             surf_rad_forcing->cloud_base_height_m/1000.0;
    surf_rad_forcing->incoming_longwave_radiation_W_per_sq_m=stefan_boltzmann_constant*
                                            pow(cloud_base_temperature_C+TK,4.0);
    }
  else  // not overcast, use TVA (1972) formulation, taken from Bras R.L., textbook, pg. 44.
    {
    // use cloudy skies formulation
    // clear sky emissivity
    atmosphere_longwave_emissivity = 0.740 + 0.0049*actual_water_vapor_partial_pressure_Pa/100.0; //conv. Pa to mb
    N=surf_rad_forcing->cloud_cover_fraction;
    Klw=(1.0+0.17*N*N);  // effect of cloud cover from TVA (1972)
    surf_rad_forcing->incoming_longwave_radiation_W_per_sq_m=atmosphere_longwave_emissivity*Klw*
                                           stefan_boltzmann_constant*
                                           pow(surf_rad_forcing->air_temperature_C+TK,4.0);
    }
  }

net_radiation_W_per_sq_m=(1.0-surf_rad_params->surface_shortwave_albedo)*
                          surf_rad_forcing->incoming_shortwave_radiation_W_per_sq_m +
                         (1.0-surface_longwave_albedo)*
                         surf_rad_forcing->incoming_longwave_radiation_W_per_sq_m -
                         outgoing_longwave_radiation_W_per_sq_m;   // this is plus, negative grnd ht flx is downward 


return(net_radiation_W_per_sq_m);
}

//############################################################*
// subroutine to calculate aerodynamic resistance term needed *
// in both the Penman-Monteith FAO reference ET procedure.    *
// and the aerodynamic, combination, and Priestley-Taylor     *
// ET calculation methods.                                    *
// Reference: http://www.fao.org/3/X0490E/x0490e06.htm        *
// F.L. Ogden, NOAA National Weather Service, 2020            *
//############################################################*
extern double calculate_aerodynamic_resistance
(
double wind_speed_measurement_height_m,      // default =2.0 [m] 
double humidity_measurement_height_m,        // default =2.0 [m],
double zero_plane_displacement_height_m,     // depends on surface roughness [m],
double momentum_transfer_roughness_length_m, // [m],
double heat_transfer_roughness_length_m,     // [m],
double wind_speed_m_per_s                    // [m s-1].
)
{
// define local variables to ease computations:

double ra,zm,zh,d,zom,zoh,k,uz;
double von_karman_constant_squared=KV2;  // this is dimensionless universal constant [-], K=0.41, squared.

// input sanity checks.
if(1.0e-06 >=wind_speed_measurement_height_m ) wind_speed_measurement_height_m=2.0;  // standard measurement height
if(1.0e-06 >=humidity_measurement_height_m )     humidity_measurement_height_m=2.0;  // standard measurement height
if(1.0e-06 >= momentum_transfer_roughness_length_m)  
  fprintf(stderr,"momentum_transfer_roughness_length_m is tiny in calculate_aerodynamic_resistance().  Should not be tiny.\n");
if(1.0e-06 >= heat_transfer_roughness_length_m )  //warn.  Should not be tiny.
    fprintf(stderr,"heat_transfer_roughness_length_m is tiny in calculate_aerodynamic_resistance().  Should not be tiny.\n");

// convert to smaller local variable names to keep equation readable 
zm=wind_speed_measurement_height_m;
zh=humidity_measurement_height_m; 
d= zero_plane_displacement_height_m;
zom=momentum_transfer_roughness_length_m;
zoh=heat_transfer_roughness_length_m;
uz=wind_speed_m_per_s;

// here log is the natural logarithm.

ra=log((zm-d)/zom)*log((zh-d)/zoh)/(von_karman_constant_squared*uz);  // this is the equation for the aero. resist.
                                                                      // from the FAO reference ET document.

return(ra);
}

//############################################################*
// function to calculate saturation vapor pressure of air     *
// based on the exponential relationship defined in Chow,     *
// Maidment, and Mays,textbook.  Input is air temp C          *
// F.L. Ogden, NOAA National Weather Service, 2020            *
//############################################################*
double calc_air_saturation_vapor_pressure_Pa(double air_temperature_C)
{
double air_sat_vap_press_Pa= 611.0*exp(17.27*air_temperature_C/(237.3+air_temperature_C));  // it is 237.3

return(air_sat_vap_press_Pa);
}

//#################################################################*
// function to calculate slope saturation vapor pressure curve     *
// based on the exponential relationship defined in Chow,          *
// Maidment, and Mays,textbook, Eqn. 3.2.10.  Input is air temp C  *
// calls function calc_air_saturation_vapor_pressure_Pa().         *
// F.L. Ogden, NOAA National Weather Service, 2020                 *
//#################################################################*
double calc_slope_of_air_saturation_vapor_pressure_Pa_per_C(double air_temperature_C)
{
double slope_of_air_sat_vap_press_curve_Pa_per_C= 
                       4098.0*calc_air_saturation_vapor_pressure_Pa(air_temperature_C)/
                       pow((237.3+air_temperature_C),2.0);  // it is 237.3
return(slope_of_air_sat_vap_press_curve_Pa_per_C);
}

//############################################################*
// function to calculate density of liquid water by empirical *
// equation, as a function of water temperature in C          *
// fit of water density vs. temperature data by FLO.  Data    *
// from water properties table in Chow, Maidment, and Mays    *
// textbook.  Fit using tblcurve program.  r^2=0.9975         *
// this fit produces rho_w=1000.151 kg/m3 at T=0C, so I limit *
// it to 1000.0.  Doesn't accurately predict max. density at  *
// T=4C, so this equation is an approximation.  TODO maybe a  *
// better model exists.  But, this value is not required at   *
// super high precision to convert latent heat flux into a    *
// depth of water.                                            *
// F.L. Ogden, NOAA National Weather Service, 2020            *
//############################################################*
double calc_liquid_water_density_kg_per_m3(double water_temperature_C)
{
double a=0.0009998492;  // this precision is necessary
double b=4.9716595e-09; // ditto.

double water_density_kg_per_m3=1.0/(a+b*water_temperature_C*water_temperature_C);

if(988> water_density_kg_per_m3) fprintf(stderr,"strange water density value!\n");
if(1000<water_density_kg_per_m3) water_density_kg_per_m3=1000.0;   // this empirical function yield 1000.151 at 0C.

return(water_density_kg_per_m3);
}

//############################################################/
// subroutine to calculate the short-wave solar radiation     /
// reaching the land surface.  Mostly from Hydrology textbook /
// by R.L. Bras., with some updates on calculation of local   /
// hour angle, optical air mass, and atmospheric extinction.  /
// Inputs: air temp., cloud cover fraction, air turbidity,    /
//         day of year, zulu time                             /
// outputs solar radiation, solar elev. angle, solar azimuth. /
// F.L. Ogden, 2009, NOAA National Weather Service, 2020      /
//############################################################/

void calculate_solar_radiation(solar_radiation_options *options, solar_radiation_parameters *params, 
                  solar_radiation_forcing *forcing, solar_radiation_results *results)
{
double delta,r,equation_of_time_minutes,M,phi;
double sinalpha,tau,alpha,cosalpha,azimuth;
double Io,Ic,kshort,Ips;
double b,fh1;

// constants 
double solar_constant_W_per_sq_m; 


double solar_declination_angle_degrees;
double solar_declination_angle_radians;
double earth_sun_distance_ratio;
double local_hour_angle_degrees;
double local_hour_angle_radians;
double antipodal_hour_angle_degrees;
double antipodal_obs_longitude_degrees;
double obs_x,obs_y,sun_x,sun_y;
double zulu_time_h;
double optical_air_mass;

solar_constant_W_per_sq_m = 1361.6;     // Dudock de Wit et al. 2017 GRL, approx. avg. value

solar_declination_angle_degrees=23.45*M_PI/180.0*cos(2.0*M_PI/365.0*(172.0-forcing->day_of_year));
solar_declination_angle_radians=solar_declination_angle_degrees*M_PI/180.0;

earth_sun_distance_ratio=1.0+0.017*cos(2.0*M_PI/365*(186.0-forcing->day_of_year));

// calculate the local hour angle using a unit circle centered on the observer, with Obs. at x=1, y=0.
//      Note: G=Greenwich, A=180E=180W==antipode, O=obs., S=sun.
//--------------------------------------------------------------------------------------------------------------
//
//                                        y ^
//                                          |      Antipode
//                                          |     /
//                                         ---   /      all angles measured as in trigonometry
//                                     _--     --_        ^
//                                   /             \       \      If the sun were here, this would be a neg. LHA
//                                  |           rot |       |
//                                  |    EARTH   ^  |       |
//                                 |       +     |   |O ----------------------> x  (all angles measured ccw from here)
//                                  |    N.Pole  |  | Observer    \  t
//                                  |               |              \ t
//                                   \             /               |
//                                     --_     _--                 |
//                                     /   -_-    \     This is a positive local hour angle 
//                                    /            \    (LHA), after 
//                                   /              \   local noon.  This text prevents 
//                                  v                v           /   this from being 
//                                 G                 S          /    a cont. comment
//                              Greenwich       vector pointing
//                            Prime Meridian      <TO SUN>
//

// this is the "equation of time" that accounts for the analemma effect 
M=2.0*M_PI*forcing->day_of_year/365.242;     // mean anomaly of sun  from wikipedia, works for leap years 
equation_of_time_minutes=-7.655*sin(M)+9.873*sin(2.0*M+3.588);    // an approximation of the equation of time, minutes 
zulu_time_h=forcing->zulu_time_h-equation_of_time_minutes/1440.0; // adjust the position of the sun for analemma effect

// here I use the antipode as the time origin, because that is where the sun is overhead at 00:00Z
antipodal_hour_angle_degrees    =            zulu_time_h*15.0; // see note on above figure

// here I convert longitude of the observer to the same coordinate system to eliminate the problem of +-180 deg. long.
antipodal_obs_longitude_degrees =            180.0-params->longitude_degrees;

// here I convert these angles to points on a unit circle using the above coordinate system to go to a purely 
// geometric representation.  This helps deal with problems related to +-180 deg.
obs_x=1.0;
obs_y=0.0;

sun_x=cos(M_PI/180.0*(360.0-(antipodal_hour_angle_degrees-antipodal_obs_longitude_degrees)));
sun_y=sin(M_PI/180.0*(360.0-(antipodal_hour_angle_degrees-antipodal_obs_longitude_degrees)));

local_hour_angle_degrees= acos(obs_x*sun_x+obs_y*sun_y)*180.0/M_PI;  // acos(dot-product).

if(sun_y>0.0) local_hour_angle_degrees *= -1.0;  // Before local noon, the hour angle is defined as negative.

local_hour_angle_radians=local_hour_angle_degrees*M_PI/180.0;

// USE SIMPLER VARIABLE NAMES FOR THESE DENSE CALCULATIONS
tau=local_hour_angle_radians;                    // local hour angle, radians
delta=solar_declination_angle_radians;           // solar declination angle, radians
phi=params->latitude_degrees*M_PI/180.0;         // latitude, radians

// calculate solar elevation angle, sin(alpha) 

sinalpha=sin(delta)*sin(phi)+cos(delta)*cos(phi)*cos(tau);
alpha=asin(sinalpha);                                     // radians        
cosalpha=cos(alpha);

// azimuth pointing to the sun (radians)
azimuth=acos(sin(delta)/(cosalpha*cos(phi))-tan(alpha)*tan(phi));
if(tau>0.0)  // after local noon
   {
   azimuth=2.0*M_PI-azimuth; 
   }

results->solar_elevation_angle_degrees=alpha*180.0/M_PI;      // convert to degrees 

results->solar_azimuth_angle_degrees=azimuth*180.0/M_PI;      // convert to degrees  

results->solar_local_hour_angle_degrees=tau*180.0/M_PI;       // convert to degrees 

if(alpha>0.0)  // the sun is over the horizon 
  { 
  //       ==================== SHORTWAVE RADIATION CALCULATIONS =======================
  r=earth_sun_distance_ratio;           // the effect of non-circularity (eccentricity) of Earth's orbit
  Io=solar_constant_W_per_sq_m/(r*r);   //  J/(m^2 s) at top of atmosphere, adjusted for orbital eccentricity

  optical_air_mass=(1.002432*pow(sinalpha,2.0)+0.148386*sinalpha+0.0096467)/         // after Young 1994 
                   (pow(sinalpha,3.0)+0.149864*pow(sinalpha,2.0)+0.0102963*sinalpha+0.000303978);
                  
  // the following comes from Ineichen and Perez, 2002 
  fh1=exp(-1.0*params->site_elevation_m/8000.0);  // elev. in meters, effect of atmos. thickness on air mass
  b=0.664+0.163/fh1;
  
  // note, atm_turbidity is equal to Tlk in Ineichen and Perez, 2002.
  Ic=b*Io*exp(-0.09*optical_air_mass*(forcing->atmospheric_turbidity_factor-1.0));  // clear sky radiation

  // adjust for cloudiness effects using procedure from Bras' Hydrology text
  if(options->cloud_base_height_known==TRUE) 
    {
    // percent of cloudless insolation, z= cloud base elev km.
    kshort=0.18+0.0853*forcing->cloud_base_height_m/1000.0;   // convert cloud base height to km for this calc.                                   
    Ips=Ic*(1.0-(1.0-kshort)*forcing->cloud_cover_fraction);   // insolation considering clouds, Eagleson, 1970.
    }
  else
    {
    // cloud base elevation not known.
    kshort=0.65*forcing->cloud_cover_fraction*forcing->cloud_cover_fraction;  // (TVA, 1972)
    Ips=Ic*(1.0-kshort);
    }

  // all these results are calculated near the land surface, but above the canopy or snow pack.
  results->solar_radiation_flux_W_per_sq_m= Ic;   // no clouds. This is on a plane perpendicular to earth-sun line.
  results->solar_radiation_horizontal_flux_W_per_sq_m=Ic*sinalpha; // this is on a horizontal plane
  results->solar_radiation_cloudy_flux_W_per_sq_m=Ips; // Considers clouds, on a plane perpendicular to earth-sun line
  results->solar_radiation_horizontal_cloudy_flux_W_per_sq_m=Ips*sinalpha;  // on a horizontal plane tangent to earth
  
// I comment this out because it is more appripriate in a vegetation effect routine  
// Ipsg=Kt*Ips;

// I comment this out because it is part of net radiation calculations done elsewhere
// Ieff=Ipsg*(1.0-Albedo);              effective incoming shortwave radiation 
  }
  
return;
}

extern int is_fabs_less_than_epsilon(double a,double epsilon)  // returns true if fabs(a)<epsilon
{
if(fabs(a)<epsilon) return(TRUE);
else                return(FALSE);
}

// Function to calculate hydrological variables needed for evapotranspiration calculation
extern void calculate_intermediate_variables
(
evapotranspiration_options *et_options,
evapotranspiration_params *et_params,
evapotranspiration_forcing *et_forcing,
intermediate_vars *inter_vars
)
{
// local variables
double aerodynamic_resistance_sec_per_m;         //  value [s per m], computed in: calculate_aerodynamic_resistance()
double instantaneous_et_rate_m_per_s;
double aerodynamic_resistance_s_per_m;
double psychrometric_constant_Pa_per_C;
double slope_sat_vap_press_curve_Pa_s;
double air_saturation_vapor_pressure_Pa;
double air_actual_vapor_pressure_Pa;
double moist_air_density_kg_per_m3;
double water_latent_heat_of_vaporization_J_per_kg;
double moist_air_gas_constant_J_per_kg_K;
double moist_air_specific_humidity_kg_per_m3;
double vapor_pressure_deficit_Pa;
double liquid_water_density_kg_per_m3;
double delta;
double gamma;

// IF SOIL WATER TEMPERATURE NOT PROVIDED, USE A SANE VALUE
if(100.0 > et_forcing->water_temperature_C) et_forcing->water_temperature_C=22.0; // growing season

// CALCULATE VARS NEEDED FOR THE ALL METHODS:

liquid_water_density_kg_per_m3 = calc_liquid_water_density_kg_per_m3(et_forcing->water_temperature_C); // rho_w

water_latent_heat_of_vaporization_J_per_kg=2.501e+06-2370.0*et_forcing->water_temperature_C;  // eqn 2.7.6 Chow etal.
                                                                                              // aka 'lambda'
// all methods other than radiation balance method involve at least some of the aerodynamic method calculations

// IF HEAT/MOMENTUM ROUGHNESS LENGTHS NOT GIVEN, USE DEFAULTS SO THAT THEIR RATIO IS EQUAL TO 1.
if((1.0e-06> et_params->heat_transfer_roughness_length_m) ||
   (1.0e-06> et_params->momentum_transfer_roughness_length_m))   // zero should be passed down if these are unknown
   {
   et_params->heat_transfer_roughness_length_m     =1.0;     // decent default values, and the ratio of these is 1.0
   et_params->momentum_transfer_roughness_length_m =1.0;
   }

// e_sat is needed for all aerodynamic and Penman-Monteith methods

air_saturation_vapor_pressure_Pa=calc_air_saturation_vapor_pressure_Pa(et_forcing->air_temperature_C);

if( (0.0 < et_forcing->relative_humidity_percent) && (100.0 >= et_forcing->relative_humidity_percent) )
  {
  // meaningful relative humidity value provided
  air_actual_vapor_pressure_Pa=et_forcing->relative_humidity_percent/100.0 * air_saturation_vapor_pressure_Pa;
  
  // calculate specific humidity, q_v
  et_forcing->specific_humidity_2m_kg_per_kg=0.622*air_actual_vapor_pressure_Pa/et_forcing->air_pressure_Pa;
  }
else
  {
  // if here, we must be using AORC forcing that provides specific humidity instead of relative humidity
  air_actual_vapor_pressure_Pa=et_forcing->specific_humidity_2m_kg_per_kg*et_forcing->air_pressure_Pa/0.622;
  if(air_actual_vapor_pressure_Pa > air_saturation_vapor_pressure_Pa)
    {
    // this is bad.   Actual vapor pressure of air should not be higher than saturated value.
    // warn and reset to something meaningful
    fprintf(stderr,"Invalid value of specific humidity with no supplied rel. humidity in ET calc. function:\n");
    fprintf(stderr,"Relative Humidity: %lf percent\n",et_forcing->relative_humidity_percent);
    fprintf(stderr,"Specific Humidity: %lf kg/kg\n",et_forcing->specific_humidity_2m_kg_per_kg);
    air_actual_vapor_pressure_Pa=0.65*air_saturation_vapor_pressure_Pa;
    }
  }
  
// VPD
vapor_pressure_deficit_Pa = air_saturation_vapor_pressure_Pa - air_actual_vapor_pressure_Pa;

moist_air_gas_constant_J_per_kg_K=287.0*(1.0+0.608*et_forcing->specific_humidity_2m_kg_per_kg); //R_a

moist_air_density_kg_per_m3=et_forcing->air_pressure_Pa/(moist_air_gas_constant_J_per_kg_K*
                            (et_forcing->air_temperature_C+TK)); // rho_a

// DELTA
slope_sat_vap_press_curve_Pa_s=calc_slope_of_air_saturation_vapor_pressure_Pa_per_C(et_forcing->air_temperature_C); 
delta=slope_sat_vap_press_curve_Pa_s;

// gamma
water_latent_heat_of_vaporization_J_per_kg=2.501e+06-2370.0*et_forcing->water_temperature_C;  // eqn 2.7.6 Chow etal.
                                                                                              // aka 'lambda'
psychrometric_constant_Pa_per_C=CP*et_forcing->air_pressure_Pa*
                                et_params->heat_transfer_roughness_length_m/
                                (0.622*water_latent_heat_of_vaporization_J_per_kg);
gamma=psychrometric_constant_Pa_per_C;

inter_vars->liquid_water_density_kg_per_m3=liquid_water_density_kg_per_m3;
inter_vars->water_latent_heat_of_vaporization_J_per_kg=water_latent_heat_of_vaporization_J_per_kg;
inter_vars->air_saturation_vapor_pressure_Pa=air_saturation_vapor_pressure_Pa;
inter_vars->air_actual_vapor_pressure_Pa=air_actual_vapor_pressure_Pa;
inter_vars->vapor_pressure_deficit_Pa=vapor_pressure_deficit_Pa;
inter_vars->moist_air_gas_constant_J_per_kg_K=moist_air_gas_constant_J_per_kg_K;
inter_vars->moist_air_density_kg_per_m3=moist_air_density_kg_per_m3;
inter_vars->slope_sat_vap_press_curve_Pa_s=slope_sat_vap_press_curve_Pa_s;
inter_vars->water_latent_heat_of_vaporization_J_per_kg=inter_vars->water_latent_heat_of_vaporization_J_per_kg;
inter_vars->psychrometric_constant_Pa_per_C=psychrometric_constant_Pa_per_C;
}
