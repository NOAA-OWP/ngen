#ifndef TEST_BMI_C_H
#define TEST_BMI_C_H

#include <string.h>

#define TRUE 1
#define FALSE 0
#define DEBUG 1

struct test_bmi_c_model {
    // ***********************************************************
    // ***************** Non-dynamic allocations *****************
    // ***********************************************************

    // Epoch-based start time (BMI start time is considered 0.0)
    long epoch_start_time;
    int num_time_steps;
    double current_model_time;
    double model_end_time;
    int time_step_size;

    // ***********************************************************
    // ******************* Dynamic allocations *******************
    // ***********************************************************
    double* input_var_1;
    double* input_var_2;

    double* output_var_1;
    double* output_var_2;

    int param_var_1;
    double param_var_2;
    double* param_var_3;
};
typedef struct test_bmi_c_model test_bmi_c_model;

/**
 * Run this model into the future.
 *
 * @param model The model struct instance.
 * @param dt The number of seconds into the future to advance the model.
 * @return 0 if successful or 1 otherwise.
 */
extern int run(test_bmi_c_model* model, long dt);

#endif //TEST_BMI_C_H
