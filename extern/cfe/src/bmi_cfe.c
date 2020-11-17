#include "bmi.h"
#include "bmi_cfe.h"
#include "cfe.h"

#define INPUT_VAR_NAME_COUNT 3
#define OUTPUT_VAR_NAME_COUNT 5


// Don't forget to update Get_value/Get_value_at_indices (and setter) implementation if these are adjusted
static const char *output_var_names[OUTPUT_VAR_NAME_COUNT] = {
        "SCHAAKE_OUTPUT_RUNOFF",
        "GIUH_RUNOFF",
        "NASH_LATERAL_RUNOFF",
        "DEEP_GW_TO_CHANNEL_FLUX",
        "Q_OUT"
};

static const char *output_var_types[OUTPUT_VAR_NAME_COUNT] = {
        "double",
        "double",
        "double",
        "double",
        "double"
};

static const char *output_var_units[OUTPUT_VAR_NAME_COUNT] = {
        "m",
        "m",
        "m",
        "m",
        "m"
};


// Don't forget to update Get_value/Get_value_at_indices (and setter) implementation if these are adjusted
static const char *input_var_names[INPUT_VAR_NAME_COUNT] = {
        "TIME_STEP_DELTA"
        "RAIN_RATE",
        // Pa
        "SURFACE_PRESSURE"
};

static const char *input_var_types[INPUT_VAR_NAME_COUNT] = {
        "int",
        "double",
        "double"
};

static const char *input_var_units[INPUT_VAR_NAME_COUNT] = {
        "s",
        "m",
        "Pa"
};


static int Get_start_time (Bmi *self, double * time)
{
    *time = ((cfe_model *) self->data)->start_time;
    return BMI_SUCCESS;
}


static int Get_end_time (Bmi *self, double * time)
{
    *time = ((cfe_model *) self->data)->start_time;
    for (int i = 0; i < ((cfe_model *) self->data)->num_timesteps; ++i) {
        *time += ((cfe_model *) self->data)->time_step_sizes[i];
    }
    return BMI_SUCCESS;
}

// TODO: document that this will get the size of the current time step (the getter can access the full array)
static int Get_time_step (Bmi *self, double * dt)
{
    *dt = ((cfe_model *) self->data)->time_step_sizes[((cfe_model *) self->data)->current_time_step];
    return BMI_SUCCESS;
}


static int Get_time_units (Bmi *self, char * units)
{
    strncpy (units, "s", BMI_MAX_UNITS_NAME);
    return BMI_SUCCESS;
}


static int Get_current_time (Bmi *self, double * time)
{
    *time = ((cfe_model *) self->data)->start_time;
    for (int i = 0; i < ((cfe_model *) self->data)->current_time_step; ++i) {
        *time += ((cfe_model *) self->data)->time_step_sizes[i];
    }
    return BMI_SUCCESS;
}


static int Initialize (Bmi *self, const char *file)
{
    cfe_model *cfe;

    if (!self)
        return BMI_FAILURE;
    else
        cfe = (cfe_model *) self->data;

    cfe->current_time_step = 0;

    // TODO: need to initialize and retrieve forcing data

    // TODO: need to initialize (at least to empty values) several other data fields for model

    if (file) {
        //cfe->num_timesteps = // TODO
        //heat_from_input_file(&heat, file);
    }
    else {
        //heat_from_default(&heat);
        cfe->num_timesteps = 10000;

    }

    return BMI_SUCCESS;
}


static int Update (Bmi *self)
{
    run(((cfe_model *) self->data));

    return BMI_SUCCESS;
}

/*
static int Update_until (Bmi *self, double t)
{

    return BMI_SUCCESS;
}
 */


static int Finalize (Bmi *self)
{
    if (self) {
        // TODO: get anything necessary out of it first (e.g., fluxes) and copy as needed

        free_cfe_model((cfe_model *)(self->data));
        self->data = (void*)new_bmi_cfe();
    }

    return BMI_SUCCESS;
}


static int Get_var_type (Bmi *self, const char *name, char * type)
{
    // Check to see if in output array first
    for (int i = 0; i < OUTPUT_VAR_NAME_COUNT; i++) {
        if (strcmp(name, output_var_names[i]) == 0) {
            strncpy(type, output_var_types[i], BMI_MAX_TYPE_NAME);
            return BMI_SUCCESS;
        }
    }
    // Then check to see if in input array
    for (int i = 0; i < INPUT_VAR_NAME_COUNT; i++) {
        if (strcmp(name, input_var_names[i]) == 0) {
            strncpy(type, input_var_types[i], BMI_MAX_TYPE_NAME);
            return BMI_SUCCESS;
        }
    }
    // If we get here, it means the variable name wasn't recognized
    type[0] = '\0';
    return BMI_FAILURE;
}


static int Get_var_itemsize (Bmi *self, const char *name, int * size)
{
    char type[BMI_MAX_TYPE_NAME];
    int type_result = Get_var_type(self, name, type);
    if (type_result != BMI_SUCCESS) {
        return BMI_FAILURE;
    }

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


static int Get_var_units (Bmi *self, const char *name, char * units)
{
    // Check to see if in output array first
    for (int i = 0; i < OUTPUT_VAR_NAME_COUNT; i++) {
        if (strcmp(name, output_var_names[i]) == 0) {
            strncpy(units, output_var_units[i], BMI_MAX_TYPE_NAME);
            return BMI_SUCCESS;
        }
    }
    // Then check to see if in input array
    for (int i = 0; i < INPUT_VAR_NAME_COUNT; i++) {
        if (strcmp(name, input_var_names[i]) == 0) {
            strncpy(units, input_var_units[i], BMI_MAX_TYPE_NAME);
            return BMI_SUCCESS;
        }
    }
    // If we get here, it means the variable name wasn't recognized
    units[0] = '\0';
    return BMI_FAILURE;
}


static int Get_var_nbytes (Bmi *self, const char *name, int * nbytes)
{
    int item_size;
    int item_size_result = Get_var_itemsize(self, name, &item_size);
    if (item_size_result != BMI_SUCCESS) {
        return BMI_FAILURE;
    }
    *nbytes = item_size * ((cfe_model *) self->data)->num_timesteps;
    return BMI_SUCCESS;
}


/*
static int Get_var_location (Bmi *self, const char *name, char *location)
{
    if (strcmp (name, "plate_surface__temperature") == 0) {
        strncpy (location, "node", BMI_MAX_UNITS_NAME);
        return BMI_SUCCESS;
    }
    else {
        location[0] = '\0';
        return BMI_FAILURE;
    }
}
 */

/*
static int Get_value_ptr (Bmi *self, const char *name, void **dest)
{
    int status = BMI_FAILURE;

    void *src = NULL;

    if (strcmp (name, "SCHAAKE_OUTPUT_RUNOFF") == 0) {
        src = ((cfe_model *) self->data)->z[0];
    }


    {


        *dest = src;

        if (src)
            status = BMI_SUCCESS;
    }

    return status;
}
 */


static int Get_value_at_indices (Bmi *self, const char *name, void *dest, int *inds, int len)
{
    if (len < 0)
        return BMI_FAILURE;

    int var_size;
    int status = Get_var_itemsize(self, name, &var_size);
    if (status == BMI_FAILURE)
        return BMI_FAILURE;
    int nbytes = var_size * len;

    if (strcmp (name, "SCHAAKE_OUTPUT_RUNOFF") == 0) {
        double results[len];
        for (int i = 0; i < len; i++) {
            results[i] = ((cfe_model *)(self->data))->fluxes[inds[i]].Schaake_output_runoff_m;
        }
        memcpy(dest, results, nbytes);
        return BMI_SUCCESS;
    }

    if (strcmp (name, "GIUH_RUNOFF") == 0) {
        double results[len];
        for (int i = 0; i < len; i++) {
            results[i] = ((cfe_model *)(self->data))->fluxes[inds[i]].giuh_runoff_m;
        }
        memcpy(dest, results, nbytes);
        return BMI_SUCCESS;
    }

    if (strcmp (name, "NASH_LATERAL_RUNOFF") == 0) {
        double results[len];
        for (int i = 0; i < len; i++) {
            results[i] = ((cfe_model *)(self->data))->fluxes[inds[i]].nash_lateral_runoff_m;
        }
        memcpy(dest, results, nbytes);
        return BMI_SUCCESS;
    }

    if (strcmp (name, "DEEP_GW_TO_CHANNEL_FLUX") == 0) {
        double results[len];
        for (int i = 0; i < len; i++) {
            results[i] = ((cfe_model *)(self->data))->fluxes[inds[i]].flux_from_deep_gw_to_chan_m;
        }
        memcpy(dest, results, nbytes);
        return BMI_SUCCESS;
    }

    if (strcmp (name, "Q_OUT") == 0) {
        double results[len];
        for (int i = 0; i < len; i++) {
            results[i] = ((cfe_model *)(self->data))->fluxes[inds[i]].Qout_m;
        }
        memcpy(dest, results, nbytes);
        return BMI_SUCCESS;
    }

    if (strcmp (name, "TIME_STEP_DELTA") == 0) {
        int results[len];
        for (int i = 0; i < len; i++) {
            results[i] = ((cfe_model *)(self->data))->time_step_sizes[inds[i]];
        }
        memcpy(dest, results, nbytes);
        return BMI_SUCCESS;
    }

    if (strcmp (name, "RAIN_RATE") == 0) {
        double results[len];
        for (int i = 0; i < len; i++) {
            results[i] = ((cfe_model *)(self->data))->rain_rates[inds[i]];
        }
        memcpy(dest, results, nbytes);
        return BMI_SUCCESS;
    }

    if (strcmp (name, "SURFACE_PRESSURE") == 0) {
        double results[len];
        for (int i = 0; i < len; i++) {
            results[i] = ((cfe_model *)(self->data))->surface_pressure[inds[i]];
        }
        memcpy(dest, results, nbytes);
        return BMI_SUCCESS;
    }

    return BMI_FAILURE;
}


static int Get_value (Bmi *self, const char *name, void *dest)
{
    // Use nested call to "by index" version
    // Create array of indexes, have it be as big as the variable arrays, and have each index value be itself
    if (((cfe_model *)(self->data))->num_timesteps < 0)
        return BMI_FAILURE;
    int inds[((cfe_model *)(self->data))->num_timesteps];
    for (int i = 0; i < ((cfe_model *)(self->data))->num_timesteps; i++)
        inds[i] = i;

    // Then we can just ...
    return Get_value_at_indices(self, name, dest, inds, ((cfe_model *)(self->data))->num_timesteps);
}


static int Set_value_at_indices (Bmi *self, const char *name, int * inds, int len, void *src)
{
    if (len < 0)
        return BMI_FAILURE;

    int var_size;
    int status = Get_var_itemsize(self, name, &var_size);
    if (status == BMI_FAILURE)
        return BMI_FAILURE;
    int nbytes = var_size * len;

    if (strcmp (name, "SCHAAKE_OUTPUT_RUNOFF") == 0) {
        double results[len];
        memcpy(results, src, nbytes);
        for (int i = 0; i < len; i++) {
            ((cfe_model *)(self->data))->fluxes[inds[i]].Schaake_output_runoff_m = results[i];
        }
        return BMI_SUCCESS;
    }

    if (strcmp (name, "GIUH_RUNOFF") == 0) {
        double results[len];
        memcpy(results, src, nbytes);
        for (int i = 0; i < len; i++) {
            ((cfe_model *)(self->data))->fluxes[inds[i]].giuh_runoff_m = results[i];
        }
        return BMI_SUCCESS;
    }

    if (strcmp (name, "NASH_LATERAL_RUNOFF") == 0) {
        double results[len];
        memcpy(results, src, nbytes);
        for (int i = 0; i < len; i++) {
            ((cfe_model *)(self->data))->fluxes[inds[i]].nash_lateral_runoff_m = results[i];
        }
        return BMI_SUCCESS;
    }

    if (strcmp (name, "DEEP_GW_TO_CHANNEL_FLUX") == 0) {
        double results[len];
        memcpy(results, src, nbytes);
        for (int i = 0; i < len; i++) {
            ((cfe_model *)(self->data))->fluxes[inds[i]].flux_from_deep_gw_to_chan_m = results[i];
        }
        return BMI_SUCCESS;
    }

    if (strcmp (name, "Q_OUT") == 0) {
        double results[len];
        memcpy(results, src, nbytes);
        for (int i = 0; i < len; i++) {
            ((cfe_model *)(self->data))->fluxes[inds[i]].Qout_m = results[i];
        }
        return BMI_SUCCESS;
    }

    if (strcmp (name, "TIME_STEP_DELTA") == 0) {
        int results[len];
        memcpy(results, src, nbytes);
        for (int i = 0; i < len; i++) {
            ((cfe_model *)(self->data))->time_step_sizes[inds[i]] = results[i];
        }
        return BMI_SUCCESS;
    }

    if (strcmp (name, "RAIN_RATE") == 0) {
        double results[len];
        memcpy(results, src, nbytes);
        for (int i = 0; i < len; i++) {
            ((cfe_model *)(self->data))->rain_rates[inds[i]] = results[i];
        }
        return BMI_SUCCESS;
    }

    if (strcmp (name, "SURFACE_PRESSURE") == 0) {
        double results[len];
        memcpy(results, src, nbytes);
        for (int i = 0; i < len; i++) {
            ((cfe_model *)(self->data))->surface_pressure[inds[i]] = results[i];
        }
        return BMI_SUCCESS;
    }

    return BMI_FAILURE;
}


static int Set_value (Bmi *self, const char *name, void *array)
{
    // Use nested call to "by index" version
    // Create array of indexes, have it be as big as the variable arrays, and have each index value be itself
    if (((cfe_model *)(self->data))->num_timesteps < 0)
        return BMI_FAILURE;
    int inds[((cfe_model *)(self->data))->num_timesteps];
    for (int i = 0; i < ((cfe_model *)(self->data))->num_timesteps; i++)
        inds[i] = i;

    // Then we can just ...
    return Set_value_at_indices(self, name, inds, ((cfe_model *)(self->data))->num_timesteps, array);
}


static int Get_component_name (Bmi *self, char * name)
{
    strncpy (name, "The CFE Model", BMI_MAX_COMPONENT_NAME);
    return BMI_SUCCESS;
}


static int Get_input_item_count (Bmi *self, int * count)
{
    *count = INPUT_VAR_NAME_COUNT;
    return BMI_SUCCESS;
}


static int Get_input_var_names (Bmi *self, char ** names)
{
    int i;
    for (i=0; i<INPUT_VAR_NAME_COUNT; i++) {
        strncpy (names[i], input_var_names[i], BMI_MAX_VAR_NAME);
    }
    return BMI_SUCCESS;
}


static int Get_output_item_count (Bmi *self, int * count)
{
    *count = OUTPUT_VAR_NAME_COUNT;
    return BMI_SUCCESS;
}


static int Get_output_var_names (Bmi *self, char ** names)
{
    int i;
    for (i=0; i<OUTPUT_VAR_NAME_COUNT; i++) {
        strncpy (names[i], output_var_names[i], BMI_MAX_VAR_NAME);
    }
    return BMI_SUCCESS;
}


cfe_model *new_bmi_cfe(void)
{
    cfe_model *data;

    // TODO: this probably isn't sufficient
    data = (cfe_model *) malloc(sizeof(cfe_model));
    return data;
}


Bmi* register_bmi_cfe(Bmi *model)
{
    if (model) {
        model->data = (void*)new_bmi_cfe();

        model->initialize = Initialize;
        model->update = Update;
        model->update_until = NULL;
        model->finalize = Finalize;

        model->get_component_name = Get_component_name;
        model->get_input_item_count = Get_input_item_count;
        model->get_output_item_count = Get_output_item_count;
        model->get_input_var_names = Get_input_var_names;
        model->get_output_var_names = Get_output_var_names;

        model->get_var_grid = NULL;
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
        model->get_value_ptr = Get_value_ptr;
        model->get_value_at_indices = Get_value_at_indices;

        model->set_value = Set_value;
        model->set_value_at_indices = Set_value_at_indices;

        model->get_grid_size = NULL;
        model->get_grid_rank = NULL;
        model->get_grid_type = NULL;

        model->get_grid_shape = NULL;
        model->get_grid_spacing = NULL;
        model->get_grid_origin = NULL;

        model->get_grid_x = NULL;
        model->get_grid_y = NULL;
        model->get_grid_z = NULL;

        model->get_grid_node_count = NULL;
        model->get_grid_edge_count = NULL;
        model->get_grid_face_count = NULL;
        model->get_grid_edge_nodes = NULL;
        model->get_grid_face_edges = NULL;
        model->get_grid_face_nodes = NULL;
        model->get_grid_nodes_per_face = NULL;
    }

    return model;
}
