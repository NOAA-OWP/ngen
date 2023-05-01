#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "bmi_test_bmi_c.h"

#define DEFAULT_TIME_STEP_SIZE 3600
#define DEFAULT_TIME_STEP_COUNT 24

#define INPUT_VAR_NAME_COUNT 2
#define OUTPUT_VAR_NAME_COUNT 2
#define PARAM_VAR_NAME_COUNT 3

// Don't forget to update Get_value/Get_value_at_indices (and setter) implementation if these are adjusted
static const char *output_var_names[OUTPUT_VAR_NAME_COUNT] = { "OUTPUT_VAR_1", "OUTPUT_VAR_2" };
static const char *output_var_types[OUTPUT_VAR_NAME_COUNT] = { "double", "double" };
static const int output_var_item_count[OUTPUT_VAR_NAME_COUNT] = { 1, 1 };
static const char *output_var_units[OUTPUT_VAR_NAME_COUNT] = { "m", "m" };
static const int output_var_grids[OUTPUT_VAR_NAME_COUNT] = { 0, 0 };
static const char *output_var_locations[OUTPUT_VAR_NAME_COUNT] = { "node", "node" };

// Don't forget to update Get_value/Get_value_at_indices (and setter) implementation if these are adjusted
static const char *input_var_names[INPUT_VAR_NAME_COUNT] = { "INPUT_VAR_1", "INPUT_VAR_2" };
static const char *input_var_types[INPUT_VAR_NAME_COUNT] = { "double", "double" };
static const char *input_var_units[INPUT_VAR_NAME_COUNT] = {  "m", "m/s" };
static const int input_var_item_count[INPUT_VAR_NAME_COUNT] = { 1, 1 };
static const char *input_var_grids[INPUT_VAR_NAME_COUNT] = { 0, 0 };
static const char *input_var_locations[INPUT_VAR_NAME_COUNT] = { "node", "node" };

// Don't forget to update Get_value/Get_value_at_indices (and setter) implementation if these are adjusted
static const char *param_var_names[PARAM_VAR_NAME_COUNT] = { "PARAM_VAR_1", "PARAM_VAR_2", "PARAM_VAR_3" };
static const char *param_var_types[PARAM_VAR_NAME_COUNT] = { "int", "double", "double" };
static const char *param_var_units[PARAM_VAR_NAME_COUNT] = {  "m", "m/s", "m"};
static const int param_var_item_count[PARAM_VAR_NAME_COUNT] = { 1, 1, 2 };
static const char *param_var_grids[PARAM_VAR_NAME_COUNT] = { 0, 0, 0 };
static const char *param_var_locations[PARAM_VAR_NAME_COUNT] = { "node", "node", "node" };

static int Finalize (Bmi *self)
{
    // Function assumes everything that is needed is retrieved from the model before Finalize is called.
    if (self) {
        test_bmi_c_model* model = (test_bmi_c_model *)(self->data);
        if( model->input_var_1 != NULL )
            free(model->input_var_1);
        if( model->input_var_2 != NULL )
            free(model->input_var_2);
        if( model->output_var_1 != NULL )
            free(model->output_var_1);
        if( model->output_var_2 != NULL )
            free(model->output_var_2);
        if (model->param_var_3 != NULL )
            free(model->param_var_3);
        free(self->data);
    }

    return BMI_SUCCESS;
}


static int Get_component_name (Bmi *self, char * name)
{
    strncpy (name, "Testing BMI C Model", BMI_MAX_COMPONENT_NAME);
    return BMI_SUCCESS;
}


static int Get_current_time (Bmi *self, double * time)
{
    *time = ((test_bmi_c_model*)self->data)->current_model_time;
    return BMI_SUCCESS;
}


/**
 * Get the model's end time.
 *
 * @param self Pointer to the struct representing the model.
 * @param time Pointer to allocated memory location in which to store the end time value.
 * @return Whether the value was stored in the provided allocated memory pointer.
 */
static int Get_end_time (Bmi *self, double * time)
{
    self->get_start_time(self, time);
    *time += (((test_bmi_c_model *) self->data)->num_time_steps * ((test_bmi_c_model *) self->data)->time_step_size);
    return BMI_SUCCESS;
}


// TODO: consider making the grid functions such that values are determined in some way from config, for better testing
// TODO: finish implementing all the grid functions in some testable form
static int Get_grid_edge_count(Bmi *self, int grid, int *count)
{
    return BMI_FAILURE;
}


static int Get_grid_edge_nodes(Bmi *self, int grid, int *edge_nodes)
{
    return BMI_FAILURE;
}


static int Get_grid_face_count(Bmi *self, int grid, int *count)
{
    return BMI_FAILURE;
}


static int Get_grid_face_edges(Bmi *self, int grid, int *face_edges)
{
    return BMI_FAILURE;
}


static int Get_grid_face_nodes(Bmi *self, int grid, int *face_nodes)
{
    return BMI_FAILURE;
}


static int Get_grid_node_count(Bmi *self, int grid, int *count)
{
    return BMI_FAILURE;
}


static int Get_grid_nodes_per_face(Bmi *self, int grid, int *nodes_per_face)
{
    return BMI_FAILURE;
}


static int Get_grid_origin(Bmi *self, int grid, double *origin)
{
    return BMI_FAILURE;
}


static int Get_grid_rank (Bmi *self, int grid, int * rank)
{
    if (grid == 0) {
        *rank = 1;
        return BMI_SUCCESS;
    }
    else {
        *rank = -1;
        return BMI_FAILURE;
    }
}


static int Get_grid_shape(Bmi *self, int grid, int *shape)
{
    return BMI_FAILURE;
}


static int Get_grid_size(Bmi *self, int grid, int * size)
{
    if (grid == 0) {
        *size = 1;
        return BMI_SUCCESS;
    }
    else {
        *size = -1;
        return BMI_FAILURE;
    }
}


static int Get_grid_spacing(Bmi *self, int grid, double *spacing)
{
    return BMI_FAILURE;
}


static int Get_grid_type (Bmi *self, int grid, char * type)
{
    int status = BMI_FAILURE;

    if (grid == 0) {
        strncpy(type, "scalar", BMI_MAX_TYPE_NAME);
        status = BMI_SUCCESS;
    }
    else {
        type[0] = '\0';
        status = BMI_FAILURE;
    }
    return status;
}


static int Get_grid_x(Bmi *self, int grid, double *x)
{
    return BMI_FAILURE;
}


static int Get_grid_y(Bmi *self, int grid, double *y)
{
    return BMI_FAILURE;
}


static int Get_grid_z(Bmi *self, int grid, double *z)
{
    return BMI_FAILURE;
}


static int Get_input_var_names (Bmi *self, char ** names)
{
    for (size_t i = 0; i < INPUT_VAR_NAME_COUNT; i++)
        strncpy (names[i], input_var_names[i], BMI_MAX_VAR_NAME);
    return BMI_SUCCESS;
}


static int Get_input_item_count (Bmi *self, int * count)
{
    *count = INPUT_VAR_NAME_COUNT;
    return BMI_SUCCESS;
}


static int Get_output_item_count (Bmi *self, int * count)
{
    *count = OUTPUT_VAR_NAME_COUNT;
    return BMI_SUCCESS;
}


static int Get_output_var_names (Bmi *self, char ** names)
{
    for (size_t i = 0; i < OUTPUT_VAR_NAME_COUNT; i++)
        strncpy (names[i], output_var_names[i], BMI_MAX_VAR_NAME);
    return BMI_SUCCESS;
}


/**
 * Get the model's start time, which by convention is `0`.
 *
 * @param self Pointer to the struct representing the model.
 * @param time Pointer to allocated memory location in which to store the start time value.
 * @return Whether the value was stored in the provided allocated memory pointer.
 */
static int Get_start_time (Bmi *self, double * time)
{
    *time = 0.0;
    return BMI_SUCCESS;
}


/**
 * Get the model's time step size.
 *
 * @param self Pointer to the struct representing the model.
 * @param dt Pointer to allocated memory location in which to store the time step size value.
 * @return Whether the value was stored in the provided allocated memory pointer.
 */
static int Get_time_step (Bmi *self, double * dt)
{
    *dt = ((test_bmi_c_model *) self->data)->time_step_size;
    return BMI_SUCCESS;
}


/**
 * Get a representation of the model's native time units.
 *
 * @param self Pointer to the struct representing the model.
 * @param units Pointer to allocated memory location in which to store the representative string for the unit type.
 * @return Whether the value was stored in the provided allocated memory pointer.
 */
static int Get_time_units (Bmi *self, char * units)
{
    strncpy (units, "s", BMI_MAX_UNITS_NAME);
    return BMI_SUCCESS;
}


static int Get_value (Bmi *self, const char *name, void *dest)
{
    int i = 0;
    int item_count = -1;
    for (i = 0; i < PARAM_VAR_NAME_COUNT; i++) {
            if (strcmp(name, param_var_names[i]) == 0) {
                item_count = param_var_item_count[i];
                break;
            }
        }
    
    if( item_count < 1 ){
        // Since all the variables are scalar, use nested call to "by index" version, with just index 0
        int inds[] = {0};
        return self->get_value_at_indices(self, name, dest, inds, 1);
    }
    else{
        //All linear indicies
        int inds[item_count];
        for(i = 0; i < item_count; i++){
            inds[i] = i;
        }
        return self->get_value_at_indices(self, name, dest, inds, item_count);
    }
}


static int Get_value_at_indices (Bmi *self, const char *name, void *dest, int *inds, int len)
{
    if (len < 1)
        return BMI_FAILURE;

    void* ptr;
    if (self->get_value_ptr(self, name, &ptr) == BMI_FAILURE)
        return BMI_FAILURE;

    char var_type[BMI_MAX_TYPE_NAME];
    if (self->get_var_type(self, name, var_type) == BMI_FAILURE)
        return BMI_FAILURE;

    if (strcmp (var_type, "double") == 0) {
        for (size_t i = 0; i < len; ++i) {
            ((double*)dest)[i] = ((double*)ptr)[inds[i]];
        }
        return BMI_SUCCESS;
    }

    if (strcmp (var_type, "int") == 0) {
        for (size_t i = 0; i < len; ++i) {
            ((int*)dest)[i] = ((int*)ptr)[inds[i]];
        }
        return BMI_SUCCESS;
    }

    if (strcmp (var_type, "float") == 0) {
        for (size_t i = 0; i < len; ++i) {
            ((float*)dest)[i] = ((float*)ptr)[inds[i]];
        }
        return BMI_SUCCESS;
    }

    if (strcmp (var_type, "long") == 0) {
        for (size_t i = 0; i < len; ++i) {
            ((long*)dest)[i] = ((long*)ptr)[inds[i]];
        }
        return BMI_SUCCESS;
    }

    return BMI_FAILURE;
}


static int Get_value_ptr (Bmi *self, const char *name, void **dest)
{
    if (strcmp (name, "INPUT_VAR_1") == 0) {
        *dest = ((test_bmi_c_model *)(self->data))->input_var_1;
        return BMI_SUCCESS;
    }

    if (strcmp (name, "INPUT_VAR_2") == 0) {
        *dest = ((test_bmi_c_model *)(self->data))->input_var_2;
        return BMI_SUCCESS;
    }

    if (strcmp (name, "OUTPUT_VAR_1") == 0) {
        *dest = ((test_bmi_c_model *)(self->data))->output_var_1;
        return BMI_SUCCESS;
    }

    if (strcmp (name, "OUTPUT_VAR_2") == 0) {
        *dest = ((test_bmi_c_model *)(self->data))->output_var_2;
        return BMI_SUCCESS;
    }

    if (strcmp (name, "PARAM_VAR_1") == 0) {
        *dest = &((test_bmi_c_model *)(self->data))->param_var_1;
        return BMI_SUCCESS;
    }

    if (strcmp (name, "PARAM_VAR_2") == 0) {
        *dest = &((test_bmi_c_model *)(self->data))->param_var_2;
        return BMI_SUCCESS;
    }

    if (strcmp (name, "PARAM_VAR_3") == 0) {
        *dest = ((test_bmi_c_model *)(self->data))->param_var_3;
        return BMI_SUCCESS;
    }
    return BMI_FAILURE;
}


static int Get_var_grid(Bmi *self, const char *name, int *grid)
{
    return BMI_FAILURE;
}


static int Get_var_itemsize (Bmi *self, const char *name, int * size)
{
    char type[BMI_MAX_TYPE_NAME];
    if (self->get_var_type(self, name, type) != BMI_SUCCESS)
        return BMI_FAILURE;

    if (strcmp (type, "double") == 0) {
        *size = sizeof(double);
        return BMI_SUCCESS;
    }
    else if (strcmp (type, "float") == 0) {
        *size = sizeof(float);
        return BMI_SUCCESS;
    }
    else if (strcmp (type, "int") == 0) {
        *size = sizeof(int);
        return BMI_SUCCESS;
    }
    else if (strcmp (type, "short") == 0) {
        *size = sizeof(short);
        return BMI_SUCCESS;
    }
    else if (strcmp (type, "long") == 0) {
        *size = sizeof(long);
        return BMI_SUCCESS;
    }
    else {
        *size = 0;
        return BMI_FAILURE;
    }
}


static int Get_var_location (Bmi *self, const char *name, char * location)
{
    size_t i;
    // Check to see if in output array first
    for (i = 0; i < OUTPUT_VAR_NAME_COUNT; i++) {
        if (strcmp(name, output_var_names[i]) == 0) {
            strncpy(location, output_var_locations[i], BMI_MAX_LOCATION_NAME);
            return BMI_SUCCESS;
        }
    }
    // Then check to see if in input array
    for (i = 0; i < INPUT_VAR_NAME_COUNT; i++) {
        if (strcmp(name, input_var_names[i]) == 0) {
            strncpy(location, input_var_locations[i], BMI_MAX_LOCATION_NAME);
            return BMI_SUCCESS;
        }
    }
    // If we get here, it means the variable name wasn't recognized
    location[0] = '\0';
    return BMI_FAILURE;
}


static int Get_var_nbytes (Bmi *self, const char *name, int * nbytes)
{
    int item_size;
    if (self->get_var_itemsize(self, name, &item_size) != BMI_SUCCESS) {
        return BMI_FAILURE;
    }
    int item_count = -1;
    size_t i;
    for (i = 0; i < INPUT_VAR_NAME_COUNT; i++) {
        if (strcmp(name, input_var_names[i]) == 0) {
            item_count = input_var_item_count[i];
            break;
        }
    }
    if (item_count < 1) {
        for (i = 0; i < OUTPUT_VAR_NAME_COUNT; i++) {
            if (strcmp(name, output_var_names[i]) == 0) {
                item_count = output_var_item_count[i];
                break;
            }
        }
    }
    if (item_count < 1) {
        for (i = 0; i < PARAM_VAR_NAME_COUNT; i++) {
            if (strcmp(name, param_var_names[i]) == 0) {
                item_count = param_var_item_count[i];
                break;
            }
        }
    }
    if (item_count < 1)
        item_count = ((test_bmi_c_model *) self->data)->num_time_steps;

    *nbytes = item_size * item_count;
    return BMI_SUCCESS;
}


static int Get_var_type (Bmi *self, const char *name, char * type)
{
    size_t i;
    // Check to see if in output array first
    for (i = 0; i < OUTPUT_VAR_NAME_COUNT; i++) {
        if (strcmp(name, output_var_names[i]) == 0) {
            strncpy(type, output_var_types[i], BMI_MAX_TYPE_NAME);
            return BMI_SUCCESS;
        }
    }
    // Then check to see if in input array
    for (i = 0; i < INPUT_VAR_NAME_COUNT; i++) {
        if (strcmp(name, input_var_names[i]) == 0) {
            strncpy(type, input_var_types[i], BMI_MAX_TYPE_NAME);
            return BMI_SUCCESS;
        }
    }
    // Finally check to see if in param array
    for (i = 0; i < PARAM_VAR_NAME_COUNT; i++) {
        if (strcmp(name, param_var_names[i]) == 0) {
            strncpy(type, param_var_types[i], BMI_MAX_TYPE_NAME);
            return BMI_SUCCESS;
        }
    }
    // If we get here, it means the variable name wasn't recognized
    type[0] = '\0';
    return BMI_FAILURE;
}


static int Get_var_units (Bmi *self, const char *name, char * units)
{
    size_t i;
    // Check to see if in output array first
    for (i = 0; i < OUTPUT_VAR_NAME_COUNT; i++) {
        if (strcmp(name, output_var_names[i]) == 0) {
            strncpy(units, output_var_units[i], BMI_MAX_UNITS_NAME);
            return BMI_SUCCESS;
        }
    }
    // Then check to see if in input array
    for (i = 0; i < INPUT_VAR_NAME_COUNT; i++) {
        if (strcmp(name, input_var_names[i]) == 0) {
            strncpy(units, input_var_units[i], BMI_MAX_UNITS_NAME);
            return BMI_SUCCESS;
        }
    }
    // If we get here, it means the variable name wasn't recognized
    units[0] = '\0';
    return BMI_FAILURE;
}


/**
 * Execute model initialization.
 *
 * @param self The BMI model instance
 * @param file The path to the BMI initialization file.
 * @return The BMI return code indicating success or failure as appropriate.
 */
static int Initialize (Bmi *self, const char *file)
{
    test_bmi_c_model *model;

    if (!self || !file)
        return BMI_FAILURE;
    else
        model = (test_bmi_c_model *) self->data;

    if (read_init_config(file, model) == BMI_FAILURE)
        return BMI_FAILURE;

    self->get_start_time(self, &(model->current_model_time));

    // If neither of these is read from config (remain 0 as set in new_bmi_model()), fall back to default for ts count
    if (model->num_time_steps == 0 && model->model_end_time == 0) {
        model->num_time_steps = DEFAULT_TIME_STEP_COUNT;
    }
    // Now at least one must be set
    assert(model->model_end_time != 0 || model->num_time_steps != 0);
    // Whenever end time is not already set here, derive based on num_time_steps
    if (model->model_end_time == 0) {
        assert(model->num_time_steps != 0);
        model->model_end_time = model->current_model_time + (model->num_time_steps * model->time_step_size);
    }
    assert(model->model_end_time != 0);
    if (model->num_time_steps == 0) {
        model->num_time_steps = (int)((model->model_end_time - model->current_model_time) / model->time_step_size);
    }

    model->input_var_1 = malloc(sizeof(double));
    model->input_var_2 = malloc(sizeof(double));
    model->output_var_1 = malloc(sizeof(double));
    model->output_var_2 = malloc(sizeof(double));

    model->param_var_1 = 0;
    model->param_var_2 = 0.0;
    model->param_var_3 = malloc(2*sizeof(double));
    model->param_var_3[0] = 0.0;
    model->param_var_3[1] = 0.0;

    return BMI_SUCCESS;
}


static int Set_value_at_indices (Bmi *self, const char *name, int * inds, int len, void *src)
{
    if (len < 1)
        return BMI_FAILURE;

    void* ptr;
    if (self->get_value_ptr(self, name, &ptr) == BMI_FAILURE)
        return BMI_FAILURE;

    char var_type[BMI_MAX_TYPE_NAME];
    if (self->get_var_type(self, name, var_type) == BMI_FAILURE)
        return BMI_FAILURE;

    if (strcmp (var_type, "double") == 0) {
        for (size_t i = 0; i < len; ++i) {
            ((double*)ptr)[inds[i]] = ((double*)src)[i];
        }
        return BMI_SUCCESS;
    }

    if (strcmp (var_type, "int") == 0) {
        for (size_t i = 0; i < len; ++i) {
            ((int*)ptr)[inds[i]] = ((int*)src)[i];
        }
        return BMI_SUCCESS;
    }

    if (strcmp (var_type, "float") == 0) {
        for (size_t i = 0; i < len; ++i) {
            ((float*)ptr)[inds[i]] = ((float*)src)[i];
        }
        return BMI_SUCCESS;
    }

    if (strcmp (var_type, "long") == 0) {
        for (size_t i = 0; i < len; ++i) {
            ((long*)ptr)[inds[i]] = ((long*)src)[i];
        }
        return BMI_SUCCESS;
    }

    return BMI_FAILURE;
}


static int Set_value (Bmi *self, const char *name, void *array) {
    void *dest = NULL;
    if (self->get_value_ptr(self, name, &dest) == BMI_FAILURE)
        return BMI_FAILURE;

    int nbytes = 0;
    if (self->get_var_nbytes(self, name, &nbytes) == BMI_FAILURE)
        return BMI_FAILURE;

    memcpy (dest, array, nbytes);

    return BMI_SUCCESS;
}

static int Update (Bmi *self)
{
    test_bmi_c_model* model = (test_bmi_c_model*)self->data;
    return self->update_until(self, model->current_model_time + model->time_step_size);
}

/**
 * Advance the model to the specified time.
 *
 * @param self The BMI instance.
 * @param future_time The time in the future to when to advance the model.
 * @return The BMI return code indicating success or failure as appropriate.
 */
static int Update_until (Bmi *self, double future_time)
{
    test_bmi_c_model* model = (test_bmi_c_model*)self->data;

    if (run(model, (long)(future_time - model->current_model_time)) != 0)
        return BMI_FAILURE;

    if (model->current_model_time != future_time)
        model->current_model_time = future_time;

    return BMI_SUCCESS;
}


/**
 * Create a new model data struct instance, allocating memory for the struct itself but not any pointers within it.
 *
 * The ``time_step_size`` member is set to a defined default.  All other members are set to ``0`` or ``NULL`` (for
 * pointers).
 *
 * @return Pointer to the newly created @ref test_bmi_c_model struct instance in memory.
 */
test_bmi_c_model *new_bmi_model(void)
{
    test_bmi_c_model *data;
    data = (test_bmi_c_model *) malloc(sizeof(test_bmi_c_model));

    data->time_step_size = DEFAULT_TIME_STEP_SIZE;
    data->num_time_steps = 0;
    data->current_model_time = 0;
    data->model_end_time = 0;
    data->epoch_start_time = 0;

    data->input_var_1 = NULL;
    data->input_var_2 = NULL;
    data->output_var_1 = NULL;
    data->output_var_2 = NULL;

    return data;
}


/**
 * Read number of lines in file and max line length, returning -1 if it does not exist or could not be read.
 *
 * @param file_name The name of the file to open and read.
 * @param line_count A pointer to a location in which to write the value for the number of lines in the file.
 * @param max_line_length A pointer to a location in which to write the value of the max line length for the file.
 * @return 0 if successful or -1 otherwise.
 */
int read_file_line_counts(const char* file_name, int* line_count, int* max_line_length)
{
    *line_count = 0;
    *max_line_length = 0;
    int current_line_length = 0;
    FILE* fp = fopen(file_name, "r");
    // Ensure exists
    if (fp == NULL) {
        return -1;
    }
    int seen_non_whitespace = 0;
    int c; //EOF is a negative constant...and char may be either signed OR unsigned
    //depending on the compiler, system, achitectured, ect.  So there are cases
    //where this loop could go infinite comparing EOF to unsigned char
    //the return of fgetc is int, and should be stored as such!
    //https://stackoverflow.com/questions/35356322/difference-between-int-and-char-in-getchar-fgetc-and-putchar-fputc
    for (c = fgetc(fp); c != EOF; c = fgetc(fp)) {
        // keep track if this line has seen any char other than space or tab
        if (c != ' ' && c != '\t' && c != '\n')
            seen_non_whitespace++;
        // Update line count, reset non-whitespace count, adjust max_line_length (if needed), and reset current line count
        if (c == '\n') {
            *line_count += 1;
            seen_non_whitespace = 0;
            if (current_line_length > *max_line_length)
                *max_line_length = current_line_length;
            current_line_length = 0;
        }
        else {
            current_line_length += 1;
        }
    }
    fclose(fp);

    // If we saw some non-whitespace char on last line, assume last line didn't have its own \n, so count needs to be
    // incremented by 1.
    if (seen_non_whitespace > 0) {
        *line_count += 1;
    }

    // Before returning, increment the max line length by 1, since the \n will be on the line also.
    *max_line_length += 1;

    return 0;
}

/**
 * Read the BMI initialization config file and use its contents to set the state of the model.
 *
 * @param config_file The path to the config file.
 * @param model Pointer to the model struct instance.
 * @return The BMI return code indicating success or failure as appropriate.
 */
int read_init_config(const char* config_file, test_bmi_c_model* model)
{
    int config_line_count, max_config_line_length;
    // Note that this determines max line length including the ending return character, if present
    int count_result = read_file_line_counts(config_file, &config_line_count, &max_config_line_length);
    if (count_result == -1) {
        printf("Invalid config file '%s'", config_file);
        return BMI_FAILURE;
    }

    FILE* fp = fopen(config_file, "r");
    if (fp == NULL)
        return BMI_FAILURE;

    char config_line[max_config_line_length + 1];

    // TODO: may need to add other variables to track that everything that was required was properly set

    // Keep track of whether required values were set in config
    int is_epoch_start_time_set = FALSE;

    for (size_t i = 0; i < config_line_count; i++) {
        char *param_key, *param_value;
        fgets(config_line, max_config_line_length + 1, fp);

        char* config_line_ptr = config_line;
        config_line_ptr = strsep(&config_line_ptr, "\n");
        param_key = strsep(&config_line_ptr, "=");
        param_value = strsep(&config_line_ptr, "=");

#if DEGUG >= 1
        printf("Config Value - Param: '%s' | Value: '%s'\n", param_key, param_value);
#endif

        if (strcmp(param_key, "epoch_start_time") == 0) {
            model->epoch_start_time = strtol(param_value, NULL, 10);
            is_epoch_start_time_set = TRUE;
            continue;
        }
        if (strcmp(param_key, "num_time_steps") == 0) {
            model->num_time_steps = (int)strtol(param_value, NULL, 10);
            continue;
        }
        if (strcmp(param_key, "time_step_size") == 0) {
            model->time_step_size = (int)strtol(param_value, NULL, 10);
            continue;
        }
        if (strcmp(param_key, "model_end_time") == 0) {
            model->time_step_size = (int)strtol(param_value, NULL, 10);
            continue;
        }
    }

    if (is_epoch_start_time_set == FALSE) {
        printf("Config param 'epoch_start_time' not found in config file\n");
        return BMI_FAILURE;
    }

#if DEGUG >= 1
    printf("All test_bmi_c config params present; finished parsing config\n");
#endif

    return BMI_SUCCESS;
}


/**
 * Construct this BMI instance, creating the backing data struct and setting required function pointers.
 *
 * Function first creates a new data structure struct (i.e., @ref test_bmi_c_model) for the BMI instance via
 * @ref new_bmi_model, assigning the returned pointer to the BMI instance's ``data`` member.
 *
 * The function then sets all the BMI instance's function pointers, essentially "registering" other functions known here
 * so they can be accessible externally via this BMI instance.  The result is that the struct can then be used much like
 * a typical object from OO languages.
 *
 * @param model A pointer to the @ref Bmi instance to register/construct.
 * @return A pointer to the passed-in @ref Bmi instance.
 */
Bmi* register_bmi(Bmi *model) {
    if (model) {
        model->data = (void*)new_bmi_model();

        model->initialize = Initialize;
        model->update = Update;
        model->update_until = Update_until;
        model->finalize = Finalize;

        model->get_component_name = Get_component_name;
        model->get_input_item_count = Get_input_item_count;
        model->get_output_item_count = Get_output_item_count;
        model->get_input_var_names = Get_input_var_names;
        model->get_output_var_names = Get_output_var_names;

        model->get_var_grid = Get_var_grid;
        model->get_var_type = Get_var_type;
        model->get_var_itemsize = Get_var_itemsize;
        model->get_var_units = Get_var_units;
        model->get_var_nbytes = Get_var_nbytes;
        model->get_var_location = Get_var_location;

        model->get_current_time = Get_current_time;
        model->get_start_time = Get_start_time;
        model->get_end_time = Get_end_time;
        model->get_time_units = Get_time_units;
        model->get_time_step = Get_time_step;

        model->get_value = Get_value;
        model->get_value_ptr = Get_value_ptr;   // TODO: needs finished implementation
        model->get_value_at_indices = Get_value_at_indices;

        model->set_value = Set_value;
        model->set_value_at_indices = Set_value_at_indices;

        model->get_grid_size = Get_grid_size;
        model->get_grid_rank = Get_grid_rank;
        model->get_grid_type = Get_grid_type;

        model->get_grid_shape = Get_grid_shape;    // N/a for grid type scalar
        model->get_grid_spacing = Get_grid_spacing;    // N/a for grid type scalar
        model->get_grid_origin = Get_grid_origin;    // N/a for grid type scalar

        model->get_grid_x = Get_grid_x;    // N/a for grid type scalar
        model->get_grid_y = Get_grid_y;    // N/a for grid type scalar
        model->get_grid_z = Get_grid_z;    // N/a for grid type scalar

        model->get_grid_node_count = Get_grid_node_count;    // N/a for grid type scalar
        model->get_grid_edge_count = Get_grid_edge_count;    // N/a for grid type scalar
        model->get_grid_face_count = Get_grid_face_count;    // N/a for grid type scalar
        model->get_grid_edge_nodes = Get_grid_edge_nodes;    // N/a for grid type scalar
        model->get_grid_face_edges = Get_grid_face_edges;    // N/a for grid type scalar
        model->get_grid_face_nodes = Get_grid_face_nodes;    // N/a for grid type scalar
        model->get_grid_nodes_per_face = Get_grid_nodes_per_face;    // N/a for grid type scalar

    }

    return model;
}