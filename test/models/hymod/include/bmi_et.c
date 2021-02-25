#include <stdio.h>

#include "bmi.h"
#include "bmi_Et.h"
#include "Et_Calc_Function_test.hpp"

#define INPUT_VAR_NAME_COUNT 5 // 
#define OUTPUT_VAR_NAME_COUNT 2 // et_m_per_s; et_mm_per_d;

//---------------------------------------------------------------------------------------------------------------------
static const char *output_var_names[OUTPUT_VAR_NAME_COUNT] = {
  "et_m_per_s",
  "et_mm_per_d"
}

//---------------------------------------------------------------------------------------------------------------------
static const char *output_var_types[OUTPUT_VAR_NAME_COUNT] = {
  "double",
  "double"
};

//---------------------------------------------------------------------------------------------------------------------
static const int output_var_item_count[OUTPUT_VAR_NAME_COUNT] = {
  1,
  1
};

//---------------------------------------------------------------------------------------------------------------------
static const char *output_var_units[OUTPUT_VAR_NAME_COUNT] = {
  "m s-1",
  "mm s-1"
};

//---------------------------------------------------------------------------------------------------------------------
// Don't forget to update Get_value/Get_value_at_indices (and setter) implementation if these are adjusted
static const char *input_var_names[INPUT_VAR_NAME_COUNT] = {
  "wind",
  "saturation_VP",
  "actual_VP"
};

//---------------------------------------------------------------------------------------------------------------------
static const char *input_var_types[INPUT_VAR_NAME_COUNT] = {
  "double",
  "double",
  "double"
};

//---------------------------------------------------------------------------------------------------------------------
static const char *input_var_units[INPUT_VAR_NAME_COUNT] = {
  "m s-1",
  "Pa",
  "Pa"
};

//---------------------------------------------------------------------------------------------------------------------
static const int input_var_item_count[INPUT_VAR_NAME_COUNT] = {
  1,
  1
};

//---------------------------------------------------------------------------------------------------------------------
static int Get_start_time (Bmi *self, double * time)
{
    *time = 0.0;
    return BMI_SUCCESS;
}

//---------------------------------------------------------------------------------------------------------------------
static int Get_end_time (Bmi *self, double * time)
{
    Get_start_time(self, time);
    *time += (((Et_Calc_Function *) self->data)->num_timesteps * ((Et_Calc_Function *) self->data)->time_step_size);
    return BMI_SUCCESS;
}

//---------------------------------------------------------------------------------------------------------------------
// TODO: document that this will get the size of the current time step (the getter can access the full array)
static int Get_time_step (Bmi *self, double * dt)
{
    *dt = ((Et_Calc_Function *) self->data)->time_step_size;
    return BMI_SUCCESS;
}

static int Get_time_units (Bmi *self, char * units)
{
    strncpy (units, "s", BMI_MAX_UNITS_NAME);
    return BMI_SUCCESS;
}

//---------------------------------------------------------------------------------------------------------------------
static int Get_current_time (Bmi *self, double * time)
{
    Get_start_time(self, time);
#if CFE_DEGUG > 1
    printf("Current model time step: '%d'\n", ((Et_Calc_Function *) self->data)->current_time_step);
#endif
    *time += (((Et_Calc_Function *) self->data)->current_time_step * ((Et_Calc_Function *) self->data)->time_step_size);
    return BMI_SUCCESS;
}

//---------------------------------------------------------------------------------------------------------------------
/** Count the number of values in a delimited string representing an array of values. */
static int count_delimited_values(char* string_val, char* delimiter)
{
    char *copy, *copy_to_free, *value;
    int count = 0;

    // Make duplicate to avoid changing original string
    // Then work on copy, but keep 2nd pointer to copy so that memory can be freed
    copy_to_free = copy = strdup(string_val);
    while ((value = strsep(&copy, delimiter)) != NULL)
        count++;
    free(copy_to_free);
    return count;
}

//---------------------------------------------------------------------------------------------------------------------
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

    // TODO: document config file format (<param_key>=<param_val>, where array values are comma delim strings)

    // TODO: things needed in config file:
    //  - forcing file name
    //  - et_options
    //      - yes_aorc
    //      - yes_wrf
    //      - use_energy_balance_method
    //      - use_aerodynamic_method
    //      - use_combination_method
    //      - use_priestley_taylor_method
    //      - use_penman_monteith_method
    //  - et_parms
    //      - wind_speed_measurement_height_m
    //      - humidity_measurement_height_m=
    //      - vegetation_height_m
    //      - zero_plane_displacement_height_m
    //      - momentum_transfer_roughness_length
    //      - heat_transfer_roughness_length_m
    //  - surf_rad_params.
    //      - surface_longwave_emissivity
    //      - surface_shortwave_albedo
    //  - solar_options
    //      - cloud_base_height_known
    //  -solar_parms
    //      - latitude_degrees
    //      - longitude_degrees
    //      - site_elevation_m

    char config_line[max_config_line_length + 1];

    // TODO: may need to add other variables to track that everything that was required was properly set

    // Keep track of whether required values were set in config
    // TODO: do something more efficient, maybe using bitwise operations
    int is_et_options_set = FALSE;
    int is_et_parms_set = FALSE;
    int is_surf_rad_parms_set = FALSE;
    int is_solar_options_set = FALSE;
    int is_solar_parms_set = FALSE;

    // Additionally,
    // ...
    // ...

    return BMI_SUCCESS;
}


static int Initialize (Bmi *self, const char *file)
{
    et_model *et;

    if (!self || !file)
        return BMI_FAILURE;
    else
        et = (et_model *) self->data;

    cfe->current_time_step = 0;

    double instantaneous_et_rate_m_per_s,
           psychrometric_constant_Pa_per_C,
           slope_sat_vap_press_curve_Pa_s,
           air_saturation_vapor_pressure_Pa,
           air_actual_vapor_pressure_Pa,
           moist_air_density_kg_per_m3,
           water_latent_heat_of_vaporization_J_per_kg,
           moist_air_gas_constant_J_per_kg_K,
           moist_air_specific_humidity_kg_per_m3,
           vapor_pressure_deficit_Pa,
           liquid_water_density_kg_per_m3,
           lambda_et,
           delta,
           gamma;

    int config_read_result = read_init_config(file, et, &instantaneous_et_rate_m_per_s,
                                                        &psychrometric_constant_Pa_per_C, 
                                                        &slope_sat_vap_press_curve_Pa_s,
                                                        &air_saturation_vapor_pressure_Pa,
                                                        &air_actual_vapor_pressure_Pa,
                                                        &moist_air_density_kg_per_m3,
                                                        &water_latent_heat_of_vaporization_J_per_kg,
                                                        &moist_air_gas_constant_J_per_kg_K,
                                                        &moist_air_specific_humidity_kg_per_m3,
                                                        &vapor_pressure_deficit_Pa,
                                                        &liquid_water_density_kg_per_m3,
                                                        &lambda_et,
                                                        &delta,
                                                        &gamma);
    if (config_read_result == BMI_FAILURE)
        return BMI_FAILURE;

    // Figure out the number of lines first (also char count)
    int forcing_line_count, max_forcing_line_length;
    int count_result = read_file_line_counts(et->forcing_file, &forcing_line_count, &max_forcing_line_length);
    if (count_result == -1) {
        printf("Configured forcing file '%s' could not be opened for reading\n", et->forcing_file);
        return BMI_FAILURE;
    }
    if (forcing_line_count == 1) {
        printf("Invalid header-only forcing file '%s'\n", et->forcing_file);
        return BMI_FAILURE;
    }
    // Infer the number of time steps: assume a header, so equal to the number of lines minus 1
    et->num_timesteps = forcing_line_count - 1;

#if CFE_DEGUG > 0
    printf("Counts - Lines: %d | Max Line: %d | Num Time Steps: %d\n", forcing_line_count, max_forcing_line_length,
           et->num_timesteps);
#endif

    // Now initialize empty arrays that depend on number of time steps
    et->forcing_data_precip_kg_per_m2 = malloc(sizeof(double) * (et->num_timesteps + 1));
    et->forcing_data_surface_pressure_Pa = malloc(sizeof(double) * (et->num_timesteps + 1));
    et->forcing_data_time = malloc(sizeof(long) * (et->num_timesteps + 1));

    et->et_m_per_s = malloc(sizeof(double));
    et->et_mm_per_d = malloc(sizeof(double));

    // Now open it again to read the forcings
    FILE* ffp = fopen(et->forcing_file, "r");
    // Ensure still exists
    if (ffp == NULL) {
        printf("Forcing file '%s' disappeared!", et->forcing_file);
        return BMI_FAILURE;
    }

    // Read forcing file and parse forcings
    char line_str[max_forcing_line_length + 1];
    long year, month, day, hour, minute;
    double dsec;
    // First read the header line
    fgets(line_str, max_forcing_line_length + 1, ffp);
    
    aorc_forcing_data forcings;
    for (int i = 0; i < et->num_timesteps; i++) {
        fgets(line_str, max_forcing_line_length + 1, ffp);  // read in a line of AORC data.
        parse_aorc_line(line_str, &year, &month, &day, &hour, &minute, &dsec, &forcings);

        et->forcing_data_precip_kg_per_m2[i] = forcings.precip_kg_per_m2;
        et->forcing_data_surface_pressure_Pa[i] = forcings.surface_pressure_Pa;
        et->forcing_data_time[i] = forcings.time;
        
        // TODO: make sure the date+time (in the forcing itself) doesn't need to be converted somehow

        // TODO: make sure some kind of conversion isn't needed for the rain rate data
        // assumed 1000 kg/m3 density of water.  This result is mm/h;
        //rain_rate[i] = (double) aorc_data.precip_kg_per_m2;
    }

    et->epoch_start_time = et->forcing_data_time[0];

    return BMI_SUCCESS;
}


static int Update (Bmi *self)
{
    double current_time, end_time;
    self->get_current_time(self, &current_time);
    self->get_end_time(self, &end_time);
    if (current_time >= end_time) {
        return BMI_FAILURE;
    }

    run(((et_model *) self->data));

    return BMI_SUCCESS;
}


static int Update_until (Bmi *self, double t)
{

    // Don't continue if current time is at or beyond end time (or we can't determine this)
    double current_time, end_time;
    int current_time_result = self->get_current_time(self, &current_time);
    if (current_time_result == BMI_FAILURE)
        return BMI_FAILURE;
    int end_time_result = self->get_end_time(self, &end_time);
    if (end_time_result == BMI_FAILURE || current_time >= end_time)
        return BMI_FAILURE;

    // Handle easy case of t == current_time by just returning success
    if (t == current_time)
        return BMI_SUCCESS;

    et_model* et = ((et_model *) self->data);

    // First, determine if t is some future time that will be arrived at exactly after some number of future time steps
    int is_exact_future_time = (t == end_time) ? TRUE : FALSE;
    // Compare to time step endings unless obvious that t lines up (i.e., t == end_time) or doesn't (t <= current_time)
    if (is_exact_future_time == FALSE && t > current_time) {
        int future_time_step = et->current_time_step;
        double future_time_step_time = current_time;
        while (future_time_step < et->num_timesteps && future_time_step_time < end_time) {
            future_time_step_time += et->time_step_size;
            if (future_time_step_time == t) {
                is_exact_future_time = TRUE;
                break;
            }
        }
    }
    // If it is an exact time, advance to that time step
    if (is_exact_future_time == TRUE) {
        while (current_time < t) {
            run(et);
            self->get_current_time(self, &current_time);
        }
        return BMI_SUCCESS;
    }

    // If t is not an exact time, it could be a number of time step forward to proceed

    // The model doesn't support partial time step value args (i.e., fractions)
    int t_int = (int) t;
    if ((t - ((double)t_int)) != 0)
        return BMI_FAILURE;

    // Keep in mind the current_time_step hasn't been processed yet (hence, using <= for this test)
    // E.g., if (unprocessed) current_time_step = 0, t = 2, num_timesteps = 2, this is valid a valid t (run 0, run 1)
    if ((et->current_time_step + t_int) <= et->num_timesteps) {
        for (int i = 0; i < t_int; i++)
            run(et);
        return BMI_SUCCESS;
    }

    // If we arrive here, t wasn't an exact time at end of a time step or a valid relative time step jump, so invalid.
    return BMI_FAILURE;
}


static int Finalize (Bmi *self)
{
    // Function assumes everything that is needed is retrieved from the model before Finalize is called.
    if (self) {
        et_model* model = (et_model *)(self->data);

        
        free(model->air_temperature_C);
        free(model->relative_humidity_percent);
        free(model->specific_humidity_2m_kg_per_kg);
        free(model->air_pressure_Pa)
        free(model->wind_speed_m_per_s)

        free(model->et_m_per_s);
        free(model->et_mm_per_d);

        self->data = (void*)new_bmi_et();
    }

    return BMI_SUCCESS;
}


static int Get_adjusted_index_for_variable(const char *name)
{
    // Get an "adjusted index" value for the associated variable, where this is its index within the
    // associated names array, plus either:
    //      0 if it is in the output variable array or
    //      OUTPUT_VAR_NAME_COUNT if it is in the input variable array
    for (int i = 0; i < OUTPUT_VAR_NAME_COUNT; i++)
        if (strcmp(name, output_var_names[i]) == 0)
            return i;

    for (int i = 0; i < INPUT_VAR_NAME_COUNT; i++)
        if (strcmp(name, input_var_names[i]) == 0)
            return i + OUTPUT_VAR_NAME_COUNT;

    return -1;
}


// TODO: complete implementation
static int Get_grid_rank(Bmi *self, int grid, int *rank)
{
    return BMI_FAILURE;
}


// TODO: complete implementation
static int Get_grid_size(Bmi *self, int grid, int *size)
{
    return BMI_FAILURE;
}


// TODO: complete implementation
static int Get_grid_type(Bmi *self, int grid, char *type)
{
    return BMI_FAILURE;
}


// TODO: complete implementation
/* Uniform rectilinear */
static int Get_grid_shape(Bmi *self, int grid, int *shape)
{
    return BMI_FAILURE;
}


// TODO: complete implementation
static int Get_grid_spacing(Bmi *self, int grid, double *spacing)
{
    return BMI_FAILURE;
}


// TODO: complete implementation
static int Get_grid_origin(Bmi *self, int grid, double *origin)
{
    return BMI_FAILURE;
}


// TODO: complete implementation
/* Non-uniform rectilinear, curvilinear */
static int Get_grid_x(Bmi *self, int grid, double *x)
{
    return BMI_FAILURE;
}


// TODO: complete implementation
static int Get_grid_y(Bmi *self, int grid, double *y)
{
    return BMI_FAILURE;
}


// TODO: complete implementation
static int Get_grid_z(Bmi *self, int grid, double *z)
{
    return BMI_FAILURE;
}


// TODO: complete implementation
static int Get_grid_node_count(Bmi *self, int grid, int *count)
{
    return BMI_FAILURE;
}


// TODO: complete implementation
static int Get_grid_edge_count(Bmi *self, int grid, int *count)
{
    return BMI_FAILURE;
}


// TODO: complete implementation
static int Get_grid_face_count(Bmi *self, int grid, int *count)
{
    return BMI_FAILURE;
}


// TODO: complete implementation
static int Get_grid_edge_nodes(Bmi *self, int grid, int *edge_nodes)
{
    return BMI_FAILURE;
}


// TODO: complete implementation
static int Get_grid_face_edges(Bmi *self, int grid, int *face_edges)
{
    return BMI_FAILURE;
}


// TODO: complete implementation
static int Get_grid_face_nodes(Bmi *self, int grid, int *face_nodes)
{
    return BMI_FAILURE;
}


// TODO: complete implementation
static int Get_grid_nodes_per_face(Bmi *self, int grid, int *nodes_per_face)
{
    return BMI_FAILURE;
}


// TODO: complete implementation
static int Get_var_grid(Bmi *self, const char *name, int *grid)
{
    return BMI_FAILURE;
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


// TODO: complete implementation
static int Get_var_location(Bmi *self, const char *name, char *location)
{
    return BMI_FAILURE;
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
    int item_count = -1;
    for (int i = 0; i < INPUT_VAR_NAME_COUNT; i++) {
        if (strcmp(name, input_var_names[i]) == 0) {
            item_count = input_var_item_count[i];
            break;
        }
    }
    if (item_count < 1) {
        for (int i = 0; i < OUTPUT_VAR_NAME_COUNT; i++) {
            if (strcmp(name, output_var_names[i]) == 0) {
                item_count = input_var_item_count[i];
                break;
            }
        }
    }
    if (item_count < 1)
        item_count = ((cfe_model *) self->data)->num_timesteps;

    *nbytes = item_size * item_count;
    return BMI_SUCCESS;
}


static int Get_value_ptr (Bmi *self, const char *name, void **dest)
{
    if (strcmp (name, "EVAPOTRANSPIRATION_m_per_s") == 0) {
        *dest = (void*) ((et_model *)(self->data))->et_m_per_s;
        return BMI_SUCCESS;
    }

    if (strcmp (name, "EVAPOTRANSPIRATION_mm_per_d") == 0) {
        *dest = (void*) ((et_model *)(self->data))->et_mm_per_d;
        return BMI_SUCCESS;
    }
    if (strcmp (name, "AIR_TEMP") == 0) {
        et_model* model = (et_model *)(self->data);
        *dest = (void*)(model->air_temperature_C + model->current_time_step);
        return BMI_SUCCESS;
    }

    if (strcmp (name, "SURFACE_PRESSURE") == 0) {
        et_model* model = (et_model *)(self->data);
        *dest = (void*)(model->air_pressure_Pa + model->current_time_step);
        return BMI_SUCCESS;
    }

    if (strcmp (name, "RELATIVE_HUMIDITY") == 0) {
        et_model* model = (et_model *)(self->data);
        *dest = (void*)(model->relative_humidity_percent + model->current_time_step);
        return BMI_SUCCESS;
    }

    if (strcmp (name, "SPECIFIC_HUMIDITY") == 0) {
        et_model* model = (et_model *)(self->data);
        *dest = (void*)(model->specific_humidity_2m_kg_per_kg + model->current_time_step);
        return BMI_SUCCESS;
    }

    return BMI_FAILURE;
}

static int Get_value_at_indices (Bmi *self, const char *name, void *dest, int *inds, int len)
{
    if (len < 1)
        return BMI_FAILURE;

    // Start by getting an "adjusted index" value for the associated variable, where this is its index within the
    // associated names array, plus either:
    //      0 if it is in the output variable array or
    //      OUTPUT_VAR_NAME_COUNT if it is in the input variable array
    int adjusted_index = Get_adjusted_index_for_variable(name);
    if (adjusted_index < 0)
        return BMI_FAILURE;

    int var_item_size;
    int status = Get_var_itemsize(self, name, &var_item_size);
    if (status == BMI_FAILURE)
        return BMI_FAILURE;

    // For now, all variables are non-array scalar values, with only 1 item of type double
    
    // Thus, there is only ever one value to return (len must be 1) and it must always be from index 0
    if (len > 1 || inds[0] != 0) 
        return BMI_FAILURE;

    void* ptr;
    status = Get_value_ptr(self, name, &ptr);
    if (status == BMI_FAILURE)
        return BMI_FAILURE;
    memcpy(dest, ptr, var_item_size * len);
    return BMI_SUCCESS;
}


static int Get_value (Bmi *self, const char *name, void *dest)
{
    // Use nested call to "by index" version

    // Here, for now at least, we know all the variables are scalar, so
    int inds[] = {0};

    // Then we can just ...
    return Get_value_at_indices(self, name, dest, inds, 1);
}


static int Set_value_at_indices (Bmi *self, const char *name, int * inds, int len, void *src)
{
    if (len < 1)
        return BMI_FAILURE;

    // Get "adjusted_index" for variable
    int adjusted_index = Get_adjusted_index_for_variable(name);
    if (adjusted_index < 0)
        return BMI_FAILURE;

    int var_item_size;
    int status = Get_var_itemsize(self, name, &var_item_size);
    if (status == BMI_FAILURE)
        return BMI_FAILURE;

    // For now, all variables are non-array scalar values, with only 1 item of type double

    // Thus, there is only ever one value to return (len must be 1) and it must always be from index 0
    if (len > 1 || inds[0] != 0)
        return BMI_FAILURE;

    void* ptr;
    status = Get_value_ptr(self, name, &ptr);
    if (status == BMI_FAILURE)
        return BMI_FAILURE;
    memcpy(ptr, src, var_item_size * len);
    return BMI_SUCCESS;
}

static int Set_value (Bmi *self, const char *name, void *array)
{
    // Use nested call to "by index" version

    // Here, for now at least, we know all the variables are scalar, so
    int inds[] = {0};

    // Then we can just ...
    return Set_value_at_indices(self, name, inds, 1, array);
}


static int Get_component_name (Bmi *self, char * name)
{
    strncpy (name, "ET Model", BMI_MAX_COMPONENT_NAME);
    return BMI_SUCCESS;
}


static int Get_input_item_count (Bmi *self, int * count)
{
    *count = INPUT_VAR_NAME_COUNT;
    return BMI_SUCCESS;
}


static int Get_input_var_names (Bmi *self, char ** names)
{
    for (int i = 0; i < INPUT_VAR_NAME_COUNT; i++) {
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
    for (int i = 0; i < OUTPUT_VAR_NAME_COUNT; i++) {
        strncpy (names[i], output_var_names[i], BMI_MAX_VAR_NAME);
    }
    return BMI_SUCCESS;
}


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
    char c;
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


et_model *new_bmi_et(void)
{
    et_model *data;
    data = (et_model *) malloc(sizeof(et_model));
    data->time_step_size = 3600;
    return data;
}

Bmi* register_bmi(Bmi *model) {
    return register_bmi_cfe(model);
}


Bmi* register_bmi_cfe(Bmi *model)
{
    if (model) {
        model->data = (void*)new_bmi_et();

        model->initialize = Initialize;
        model->update = Update;
        model->update_until = Update_until;
        model->finalize = Finalize;

        model->get_component_name = Get_component_name;
        model->get_input_item_count = Get_input_item_count;
        model->get_output_item_count = Get_output_item_count;
        model->get_input_var_names = Get_input_var_names;
        model->get_output_var_names = Get_output_var_names;

        model->get_var_grid = Get_var_grid; // TODO: needs finished implementation
        model->get_var_type = Get_var_type;
        model->get_var_itemsize = Get_var_itemsize;
        model->get_var_units = Get_var_units;
        model->get_var_nbytes = Get_var_nbytes;
        model->get_var_location = Get_var_location; // TODO: needs finished implementation

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

        model->get_grid_size = Get_grid_size;    // TODO: needs finished implementation
        model->get_grid_rank = Get_grid_rank;    // TODO: needs finished implementation
        model->get_grid_type = Get_grid_type;    // TODO: needs finished implementation

        model->get_grid_shape = Get_grid_shape;    // TODO: needs finished implementation
        model->get_grid_spacing = Get_grid_spacing;    // TODO: needs finished implementation
        model->get_grid_origin = Get_grid_origin;    // TODO: needs finished implementation

        model->get_grid_x = Get_grid_x;    // TODO: needs finished implementation
        model->get_grid_y = Get_grid_y;    // TODO: needs finished implementation
        model->get_grid_z = Get_grid_z;    // TODO: needs finished implementation

        model->get_grid_node_count = Get_grid_node_count;    // TODO: needs finished implementation
        model->get_grid_edge_count = Get_grid_edge_count;    // TODO: needs finished implementation
        model->get_grid_face_count = Get_grid_face_count;    // TODO: needs finished implementation
        model->get_grid_edge_nodes = Get_grid_edge_nodes;    // TODO: needs finished implementation
        model->get_grid_face_edges = Get_grid_face_edges;    // TODO: needs finished implementation
        model->get_grid_face_nodes = Get_grid_face_nodes;    // TODO: needs finished implementation
        model->get_grid_nodes_per_face = Get_grid_nodes_per_face;    // TODO: needs finished implementation
    }

    return model;
}
