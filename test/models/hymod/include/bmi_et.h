#ifndef CFE_BMI_CFE_H
#define CFE_BMI_CFE_H

#if defined(__cplusplus)
extern "C" {
#endif

#include "bmi.h"
#include "EtCalcFunction.hpp"

/** Read number of lines in file and max line length, returning -1 if it does not exist or could not be read. */
int read_file_line_counts(const char* file_name, int* line_count, int* max_line_length);

int read_init_config(const char* config_file, et_model* model,
                     double* instantaneous_et_rate_m_per_s,
                     double* psychrometric_constant_Pa_per_C,
                     double* slope_sat_vap_press_curve_Pa_s,
                     double* air_saturation_vapor_pressure_Pa,
                     double* air_actual_vapor_pressure_Pa,
                     double* moist_air_density_kg_per_m3,
                     double* water_latent_heat_of_vaporization_J_per_kg,
                     double* moist_air_gas_constant_J_per_kg_K,
                     double* moist_air_specific_humidity_kg_per_m3,
                     double* vapor_pressure_deficit_Pa,
                     double* liquid_water_density_kg_per_m3,
                     double* lambda_et,
                     double* delta,
                     double* gamma)

Bmi* register_bmi(Bmi *model);

Bmi* register_bmi_cfe(Bmi *model);

et_model * new_bmi_et(void);

#if defined(__cplusplus)
}
#endif

#endif