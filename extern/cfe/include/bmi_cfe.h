#ifndef CFE_BMI_CFE_H
#define CFE_BMI_CFE_H

#if defined(__cplusplus)
extern "C" {
#endif

#include "bmi.h"
#include "cfe.h"

/** Read number of lines in file and max line length, returning -1 if it does not exist or could not be read. */
int read_file_line_counts(const char* file_name, int* line_count, int* max_line_length);

int read_init_config(const char* config_file, cfe_model* model, double* alpha_fc, double* soil_storage,
                     int* is_soil_storage_ratio);

Bmi* register_bmi(Bmi *model);

Bmi* register_bmi_cfe(Bmi *model);

cfe_model * new_bmi_cfe(void);

#if defined(__cplusplus)
}
#endif

#endif
