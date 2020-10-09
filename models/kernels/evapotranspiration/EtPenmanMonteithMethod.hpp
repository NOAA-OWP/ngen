#ifndef ET_PENMAN_MONTEITH_METHOD_H
#define ET_PENMAN_MONTEITH_METHOD_H

// FUNCTION AND SUBROUTINE PROTOTYPES

extern double evapotranspiration_penman_monteith_method
(
  evapotranspiration_options *et_options,
  evapotranspiration_params *et_params,
  evapotranspiration_forcing *et_forcing,
  intermediate_vars *inter_vars
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
  if(is_fabs_less_than_eps(et_params->vegetation_height_m,1.0e-06)==TRUE)
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

#endif // ET_PENMAN_MONTEITH_METHOD_H
