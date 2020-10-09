#ifndef ET_CALC_PPROPERTY_H
#define ET_CALC_PPROPERTY_H

#define TRUE  1
#define FALSE 0

#define CP  1.006e+03  //  specific heat of air at constant pressure, J/(kg K), a physical constant.
#define KV2 0.1681     //  von Karman's constant squared, equal to 0.41 squared, unitless
#define TK  273.15     //  temperature in Kelvin at zero degree Celcius
#define SB  5.67e-08   //  stefan_boltzmann_constant in units of W/m^2/K^4

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

extern void calculate_intermediate_variables
(
  evapotranspiration_options *et_options,
  evapotranspiration_params *et_params,
  evapotranspiration_forcing *et_forcing,
  intermediate_vars *inter_vars
);

extern int is_fabs_less_than_eps(double a,double epsilon);  // returns TRUE iff fabs(a)<epsilon

extern double calc_air_saturation_vapor_pressure_Pa(double air_temperature_C);

extern double calc_slope_of_air_saturation_vapor_pressure_Pa_per_C(double air_temperature_C);

extern double calc_liquid_water_density_kg_per_m3(double water_temperature_C);


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

extern int is_fabs_less_than_eps(double a,double epsilon)  // returns true if fabs(a)<epsilon
{
  if(fabs(a)<epsilon) return(TRUE);
  else                return(FALSE);
}

#endif // ET_CALC_PPROPERTY_H
