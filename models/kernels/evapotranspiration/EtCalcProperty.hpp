#ifndef ET_CALC_PPROPERTY_H
#define ET_CALC_PPROPERTY_H

#include "EtStruct.h"

#define TRUE  1
#define FALSE 0

#define CP  1.006e+03  //  specific heat of air at constant pressure, J/(kg K), a physical constant.
#define KV2 0.1681     //  von Karman's constant squared, equal to 0.41 squared, unitless
#define TK  273.15     //  temperature in Kelvin at zero degree Celcius
#define SB  5.67e-08   //  stefan_boltzmann_constant in units of W/m^2/K^4

namespace et {

    double calculate_net_radiation_W_per_sq_m
            (
                    evapotranspiration_options *opts,   // needed to tell if using aorc forcing values. Could be other options too.
                    surface_radiation_params *pars,
                    surface_radiation_forcing *forc
            );

    double calculate_aerodynamic_resistance
            (
                    double wind_speed_measurement_height_m,      // default =2.0 [m]
                    double humidity_measurement_height_m,        // default =2.0 [m],
                    double zero_plane_displacement_height_m,     // depends on surface roughness [m],
                    double momentum_transfer_roughness_length_m, // [m],
                    double heat_transfer_roughness_length_m,     // [m],
                    double wind_speed_m_per_s                    // [m s-1].
            );

    void calculate_solar_radiation
            (
                    solar_radiation_options *options,
                    solar_radiation_parameters *params,
                    solar_radiation_forcing *forcing,
                    solar_radiation_results *results
            );

    void calculate_intermediate_variables
            (
                    evapotranspiration_options *et_options,
                    evapotranspiration_params *et_params,
                    evapotranspiration_forcing *et_forcing,
                    intermediate_vars *inter_vars
            );

    int is_fabs_less_than_eps(double a, double epsilon);  // returns TRUE iff fabs(a)<epsilon

    double calc_air_saturation_vapor_pressure_Pa(double air_temperature_C);

    double calc_slope_of_air_saturation_vapor_pressure_Pa_per_C(double air_temperature_C);

    double calc_liquid_water_density_kg_per_m3(double water_temperature_C);

}

#endif // ET_CALC_PPROPERTY_H
