#include <iostream>
#include <cmath>
#include <cstring>
#include "EtCalcProperty.hpp"

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

//####################################
// FUNCTION AND SUBROUTINE PROTOTYPES

extern double evapotranspiration_priestley_taylor_method
(
evapotranspiration_options *et_options,
evapotranspiration_params *et_params,
evapotranspiration_forcing *et_forcing,
intermediate_vars *inter_vars
);

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
