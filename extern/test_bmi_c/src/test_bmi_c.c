#include "test_bmi_c.h"

/**
 * Run this model into the future.
 *
 * @param model The model struct instance.
 * @param dt The number of seconds into the future to advance the model.
 * @return 0 if successful or 1 otherwise.
 */
extern int run(test_bmi_c_model* model, long dt)
{
    if (dt == model->time_step_size) {
        *model->output_var_1 = *model->input_var_1;
        *model->output_var_2 = 2.0 * *model->input_var_2;
    }
    else {
        *model->output_var_1 = *model->input_var_1 * (double) dt / model->time_step_size;
        *model->output_var_2 = 2.0 * *model->input_var_2 * (double) dt / model->time_step_size;
    }
    model->current_model_time += (double)dt;

    return 0;
}