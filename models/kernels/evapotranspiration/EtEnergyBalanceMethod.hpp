#ifndef ET_ENERGY_BALANCE_METHOD_H
#define ET_ENERGY_BALANCE_METHOD_H

// FUNCTION AND SUBROUTINE PROTOTYPES

extern double evapotranspiration_energy_balance_method
(
  evapotranspiration_options *et_options,
  evapotranspiration_params *et_params,
  evapotranspiration_forcing *et_forcing
);


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

#endif // ET_ENERGY_BALANCE_METHOD_H
