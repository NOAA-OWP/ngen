#ifndef CFE_BMI_CFE_H
#define CFE_BMI_CFE_H

#if defined(__cplusplus)
extern "C" {
#endif

#include "bmi.h"
#include "cfe.h"

Bmi* register_bmi_cfe(Bmi *model);
cfe_model * new_bmi_cfe(void);


#if defined(__cplusplus)
}
#endif

#endif
