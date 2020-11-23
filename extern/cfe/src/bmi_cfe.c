#include "bmi.h"
#include "bmi_cfe.h"
#include "cfe.h"

#define CFE_DEGUG 0

#define INPUT_VAR_NAME_COUNT 0
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
        //"TIME_STEP_DELTA"
        //"RAIN_RATE",
        // Pa
        //"SURFACE_PRESSURE"
};

static const char *input_var_types[INPUT_VAR_NAME_COUNT] = {
        //"int",
        //"double",
        //"double"
};

static const char *input_var_units[INPUT_VAR_NAME_COUNT] = {
        //"s",
        //"m",
        //"Pa"
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

int read_init_config(const char* config_file, cfe_model* model, double* alpha_fc, double* soil_storage,
                     int* is_soil_storage_ratio)
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
    //  - refkdt (optional, defaults to 3.0)
    //  - soil params:
    //    // - D, or depth
    //    //  - bb, or b
    //    //  - mult, or multiplier
    //    //  - satdk
    //    //  - satpsi
    //    //  - slop, or slope
    //    //  - smcmax, or maxsmc
    //    //  - wltsmc
    //    - additional gw res params
    //    //  - max_gw_storage
    //    //  - Cgw
    //    //  - expon
    //    //  - starting S_gw (may be literal or ratio, control by checking for "%")
    //    - additionally lateral flow res params
    //    //  - alpha_fc
    //    //  - starting S_lf (may be literal or ratio, control by checking for "%")
    //    - number of Nash lf reservoirs (optional, defaults to 2, ignored if storage values present)
    //    - K_nash
    //    - initial Nash storage values (optional, defaults to 0.0 for all reservoirs according to number)
    //    - GIUH ordinates

    char config_line[max_config_line_length + 1];

    // TODO: may need to add other variables to track that everything that was required was properly set

    // Keep track of whether required values were set in config
    // TODO: do something more efficient, maybe using bitwise operations
    int is_forcing_file_set = FALSE;
    int is_soil_params__depth_set = FALSE;
    int is_soil_params__bb_set = FALSE;
    int is_soil_params__mult_set = FALSE;
    int is_soil_params__satdk_set = FALSE;
    int is_soil_params__satpsi_set = FALSE;
    int is_soil_params__slop_set = FALSE;
    int is_soil_params__smcmax_set = FALSE;
    int is_soil_params__wltsmc_set = FALSE;
    int is_Cgw_set = FALSE;
    int is_expon_set = FALSE;
    int is_alpha_fc_set = FALSE;
    int is_soil_storage_set = FALSE;
    int is_K_nash_set = FALSE;

    // Keep track these in particular, because the "true" storage value may be a ratio and need both storage and max
    int is_gw_max_set = FALSE;
    int is_gw_storage_set = FALSE;

    int is_giuh_originates_string_val_set = FALSE;

    // Default value
    double refkdt = 3.0;

    int is_gw_storage_ratio = FALSE;
    double gw_storage_literal;
    // Also keep track of Nash stuff and properly set at the end of reading the config file
    int num_nash_lf = 2;
    char* nash_storage_string_val;
    int is_nash_storage_string_val_set = FALSE;
    // Similarly as for Nash, track stuff for GIUH ordinates
    int num_giuh_ordinates = 1;
    char* giuh_originates_string_val;


    // Additionally,

    for (int i = 0; i < config_line_count; i++) {
        char *param_key, *param_value;
        fgets(config_line, max_config_line_length + 1, fp);
#if CFE_DEGUG >= 3
        printf("Line value: ['%s']\n", config_line);
#endif
        char* config_line_ptr = config_line;
        config_line_ptr = strsep(&config_line_ptr, "\n");
        param_key = strsep(&config_line_ptr, "=");
        param_value = strsep(&config_line_ptr, "=");

        if (strcmp(param_key, "forcing_file") == 0) {
            model->forcing_file = strdup(param_value);
            is_forcing_file_set = TRUE;
            continue;
        }
        if (strcmp(param_key, "refkdt") == 0) {
            refkdt = strtod(param_value, NULL);
            continue;
        }
        if (strcmp(param_key, "soil_params.D") == 0 || strcmp(param_key, "soil_params.depth") == 0) {
            model->NWM_soil_params.D = strtod(param_value, NULL);
            is_soil_params__depth_set = TRUE;
            continue;
        }
        if (strcmp(param_key, "soil_params.bb") == 0 || strcmp(param_key, "soil_params.b") == 0) {
            model->NWM_soil_params.bb = strtod(param_value, NULL);
            is_soil_params__bb_set = TRUE;
            continue;
        }
        if (strcmp(param_key, "soil_params.multiplier") == 0 || strcmp(param_key, "soil_params.mult") == 0) {
            model->NWM_soil_params.mult = strtod(param_value, NULL);
            is_soil_params__mult_set = TRUE;
            continue;
        }
        if (strcmp(param_key, "soil_params.satdk") == 0) {
            model->NWM_soil_params.satdk = strtod(param_value, NULL);
            is_soil_params__satdk_set = TRUE;
            continue;
        }
        if (strcmp(param_key, "soil_params.satpsi") == 0) {
            model->NWM_soil_params.satpsi = strtod(param_value, NULL);
            is_soil_params__satpsi_set = TRUE;
            continue;
        }
        if (strcmp(param_key, "soil_params.slope") == 0 || strcmp(param_key, "soil_params.slop") == 0) {
            model->NWM_soil_params.slop = strtod(param_value, NULL);
            is_soil_params__slop_set = TRUE;
            continue;
        }
        if (strcmp(param_key, "soil_params.smcmax") == 0 || strcmp(param_key, "soil_params.maxsmc") == 0) {
            model->NWM_soil_params.smcmax = strtod(param_value, NULL);
            is_soil_params__smcmax_set = TRUE;
            continue;
        }
        if (strcmp(param_key, "soil_params.wltsmc") == 0) {
            model->NWM_soil_params.wltsmc = strtod(param_value, NULL);
            is_soil_params__wltsmc_set = TRUE;
            continue;
        }
        if (strcmp(param_key, "max_gw_storage") == 0) {
            model->gw_reservoir.storage_max_m = strtod(param_value, NULL);
            is_gw_max_set = TRUE;
            // Also set the true storage if storage was already read and was a ratio, and so we were waiting for this
            if (is_gw_storage_set == TRUE && is_gw_storage_ratio == TRUE) {
                model->gw_reservoir.storage_m = (gw_storage_literal / 100.0) * model->gw_reservoir.storage_max_m;
            }
            continue;
        }
        if (strcmp(param_key, "Cgw") == 0) {
            model->gw_reservoir.coeff_primary = strtod(param_value, NULL);
            is_Cgw_set = TRUE;
            continue;
        }
        if (strcmp(param_key, "expon") == 0) {
            model->gw_reservoir.exponent_primary = strtod(param_value, NULL);
            is_expon_set = TRUE;
            continue;
        }
        if (strcmp(param_key, "gw_storage") == 0) {
            is_gw_storage_set = TRUE;
            char* trailing_chars;
            gw_storage_literal = strtod(param_value, &trailing_chars);
            if (strcmp(trailing_chars, "%") == 0) {
                is_gw_storage_ratio = TRUE;
            }
            // Can't set the struct value yet unless storage is non-ratio or max storage was already set
            if (is_gw_storage_ratio == FALSE) {
                model->gw_reservoir.storage_m = gw_storage_literal;
            }
            if (is_gw_storage_ratio == TRUE && is_gw_max_set == TRUE) {
                model->gw_reservoir.storage_m = (gw_storage_literal / 100.0) * model->gw_reservoir.storage_max_m;
            }
            continue;
        }
        if (strcmp(param_key, "alpha_fc") == 0) {
            *alpha_fc = strtod(param_value, NULL);
            is_alpha_fc_set = TRUE;
            continue;
        }
        if (strcmp(param_key, "soil_storage") == 0) {
            char* trailing_chars;
            double parsed_value = strtod(param_value, &trailing_chars);
            *is_soil_storage_ratio = strcmp(trailing_chars, "%") == 0 ? TRUE : FALSE;
            *soil_storage = *is_soil_storage_ratio == TRUE ? (parsed_value / 100.0) : parsed_value;
            is_soil_storage_set = TRUE;
            continue;
        }
        if (strcmp(param_key, "number_nash_reservoirs") == 0 || strcmp(param_key, "N_nash") == 0) {
            num_nash_lf = strtol(param_value, NULL, 10);
            continue;
        }
        if (strcmp(param_key, "K_nash") == 0) {
            model->K_nash = strtod(param_value, NULL);
            is_K_nash_set = TRUE;
            continue;
        }
        if (strcmp(param_key, "nash_storage") == 0) {
            nash_storage_string_val = strdup(param_value);
            is_nash_storage_string_val_set = TRUE;
            continue;
        }
        if (strcmp(param_key, "giuh_ordinates") == 0) {
            giuh_originates_string_val = strdup(param_value);
            is_giuh_originates_string_val_set = TRUE;
            continue;
        }
    }

    if (is_forcing_file_set == FALSE) {
        return BMI_FAILURE;
    }
    if (is_soil_params__depth_set == FALSE) {
        return BMI_FAILURE;
    }
    if (is_soil_params__bb_set == FALSE) {
        return BMI_FAILURE;
    }
    if (is_soil_params__mult_set == FALSE) {
        return BMI_FAILURE;
    }
    if (is_soil_params__satdk_set == FALSE) {
        return BMI_FAILURE;
    }
    if (is_soil_params__satpsi_set == FALSE) {
        return BMI_FAILURE;
    }
    if (is_soil_params__slop_set == FALSE) {
        return BMI_FAILURE;
    }
    if (is_soil_params__smcmax_set == FALSE) {
        return BMI_FAILURE;
    }
    if (is_soil_params__wltsmc_set == FALSE) {
        return BMI_FAILURE;
    }
    if (is_Cgw_set == FALSE) {
        return BMI_FAILURE;
    }
    if (is_expon_set == FALSE) {
        return BMI_FAILURE;
    }
    if (is_alpha_fc_set == FALSE) {
        return BMI_FAILURE;
    }
    if (is_soil_storage_set == FALSE) {
        return BMI_FAILURE;
    }
    if (is_K_nash_set == FALSE) {
        return BMI_FAILURE;
    }
    if (is_gw_max_set == FALSE) {
        return BMI_FAILURE;
    }
    if (is_gw_storage_set == FALSE) {
        return BMI_FAILURE;
    }

    model->Schaake_adjusted_magic_constant_by_soil_type = refkdt * model->NWM_soil_params.satdk / 0.000002;
    
    // Used for parsing strings representing arrays of values below
    char *copy, *value;

    // Handle GIUH ordinates, bailing if they were not provided
    if (is_giuh_originates_string_val_set == FALSE)
        return BMI_FAILURE;
    model->num_giuh_ordinates = count_delimited_values(giuh_originates_string_val, ",");
    if (model->num_giuh_ordinates < 1)
        return BMI_FAILURE;
    model->giuh_ordinates = malloc(sizeof(double) * model->num_giuh_ordinates);
    // Work with copy of the string pointer to make sure the original pointer remains unchanged, so mem can be freed at end
    copy = giuh_originates_string_val;
    // Now iterate back through and get the values (this modifies the string, which is why we needed the full string copy above)
    int i = 0;
    while ((value = strsep(&copy, ",")) != NULL)
        model->giuh_ordinates[i++] = strtod(value, NULL);
    // Finally, free the original string memory
    free(giuh_originates_string_val);

    // Now handle the Nash storage array properly
    if (is_nash_storage_string_val_set == TRUE) {
        // First, when there are values, read how many there are, and have that override any set count value
        int value_count = count_delimited_values(nash_storage_string_val, ",");
        // TODO: consider adding a warning if value_count and N_nash (assuming it was read from the config and not default) disagree
        // Ignore the values if there are not enough, and use whatever was set, or defaults
        if (value_count < 2) {
            model->num_lateral_flow_nash_reservoirs = num_nash_lf;
            model->nash_storage = malloc(sizeof(double) * num_nash_lf);
            for (int j = 0; j < num_nash_lf; j++)
                model->nash_storage[j] = 0.0;
        }
        else {
            model->num_lateral_flow_nash_reservoirs = value_count;
            model->nash_storage = malloc(sizeof(double) * value_count);
            // Work with copy the string pointer to make sure the original remains unchanged, so it can be freed at end
            copy = nash_storage_string_val;
            // Now iterate back through and get the values
            int k = 0;
            while ((value = strsep(&copy, ",")) != NULL)
                model->nash_storage[k++] = strtod(value, NULL);
        }
        // Make sure at the end to free this too, since it was a copy
        free(nash_storage_string_val);
    }
    // If Nash storage values weren't set, initialize them to 0.0
    else {
        model->num_lateral_flow_nash_reservoirs = num_nash_lf;
        model->nash_storage = malloc(sizeof(double) * num_nash_lf);
        for (int j = 0; j < num_nash_lf; j++)
            model->nash_storage[j] = 0.0;
    }

    return BMI_SUCCESS;
}


static int Initialize (Bmi *self, const char *file)
{
    cfe_model *cfe;

    if (!self || !file)
        return BMI_FAILURE;
    else
        cfe = (cfe_model *) self->data;

    cfe->current_time_step = 0;

    double alpha_fc, max_soil_storage, S_soil;
    int is_S_soil_ratio;

    int config_read_result = read_init_config(file, cfe, &alpha_fc, &S_soil, &is_S_soil_ratio);
    if (config_read_result == BMI_FAILURE)
        return BMI_FAILURE;

    max_soil_storage = cfe->NWM_soil_params.D * cfe->NWM_soil_params.smcmax;

    // Figure out the number of lines first (also char count)
    int forcing_line_count, max_forcing_line_length;
    int count_result = read_file_line_counts(cfe->forcing_file, &forcing_line_count, &max_forcing_line_length);
    if (count_result == -1) {
        printf("Invalid forcing file '%s'", cfe->forcing_file);
        return BMI_FAILURE;
    }
    if (forcing_line_count == 1) {
        printf("Invalid header-only forcing file '%s'", cfe->forcing_file);
        return BMI_FAILURE;
    }
    // Infer the number of time steps: assume a header, so equal to the number of lines minus 1
    cfe->num_timesteps = forcing_line_count - 1;

    // Now initialize empty arrays that depend on number of time steps
    cfe->time_step_sizes = malloc(sizeof(int) * cfe->num_timesteps);
    // TODO: double check these are correct (i.e., doesn't need to break down into sum of the struct member sizes)
    cfe->forcings = malloc(sizeof(aorc_forcing_data) * cfe->num_timesteps);
    cfe->fluxes = malloc(sizeof(result_fluxes) * cfe->num_timesteps);

    // Now open it again to read the forcings
    FILE* ffp = fopen(cfe->forcing_file, "r");
    // Ensure still exists
    if (ffp == NULL) {
        printf("Forcing file '%s' disappeared!", cfe->forcing_file);
        return BMI_FAILURE;
    }

    // Read forcing file and parse forcings
    char line_str[max_forcing_line_length + 1];
    long year, month, day, hour, minute;
    double dsec;
    // First read the header line
    fgets(line_str, max_forcing_line_length + 1, ffp);

    for (int i = 0; i < cfe->num_timesteps; i++) {
        fgets(line_str, max_forcing_line_length + 1, ffp);  // read in a line of AORC data.
        parse_aorc_line(line_str, &year, &month, &day, &hour, &minute, &dsec, &(cfe->forcings[i]));
        // TODO: make sure the date+time (in the forcing itself) doesn't need to be converted somehow

        // TODO: make sure some kind of conversion isn't needed for the rain rate data
        // assumed 1000 kg/m3 density of water.  This result is mm/h;
        //rain_rate[i] = (double) aorc_data.precip_kg_per_m2;
    }

    cfe->start_time = cfe->forcings[0].time;

    // This will set all but the last time step size
    for (int i = 0; i < cfe->num_timesteps - 1; i++) {
        // TODO: do we need to worry about type shortening here?
        cfe->time_step_sizes[i] = cfe->forcings[i+1].time - cfe->forcings[i].time;
    }
    // For the last time step size, if there is only a single time step, default to an hour
    if (cfe->num_timesteps == 1) {
        cfe->time_step_sizes[0] = 3600;
    }
    // Otherwise, use the same size as the previous
    else {
        cfe->time_step_sizes[cfe->num_timesteps - 1] = cfe->time_step_sizes[cfe->num_timesteps - 2];
    }

    // Initialize the rest of the groundwater conceptual reservoir (some was done when reading in the config)
    cfe->gw_reservoir.is_exponential = TRUE;
    cfe->gw_reservoir.storage_threshold_primary_m = 0.0;    // 0.0 means no threshold applied
    cfe->gw_reservoir.storage_threshold_secondary_m = 0.0;  // 0.0 means no threshold applied
    cfe->gw_reservoir.coeff_secondary = 0.0;                // 0.0 means that secondary outlet is not applied
    cfe->gw_reservoir.exponent_secondary = 1.0;             // linear

    // Initialize soil conceptual reservoirs
    init_soil_reservoir(cfe, alpha_fc, max_soil_storage, S_soil, is_S_soil_ratio);

    // Initialize the runoff queue to empty to start with
    cfe->runoff_queue_m_per_timestep = malloc(sizeof(double) * cfe->num_giuh_ordinates + 1);
    for (int i = 0; i < cfe->num_giuh_ordinates + 1; i++)
        cfe->runoff_queue_m_per_timestep[i] = 0.0;

    return BMI_SUCCESS;
}


static int Update (Bmi *self)
{
    // TODO: look at how the time step size (in seconds) effects 'coeff_primary' for lat flow reservoir, and whether
    //  this dictates that, if time step size is not fixed, an adjustment needs to be made to reservoir on each call
    //  here according to the size of the next time step.

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
    // Function assumes everything that is needed is retrieved from the model before Finalize is called.
    if (self) {
        cfe_model* model = (cfe_model *)(self->data);

        free(model->forcings);
        free(model->time_step_sizes);
        free(model->giuh_ordinates);
        free(model->nash_storage);
        free(model->runoff_queue_m_per_timestep);
        free(model->fluxes);

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

    /*
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
            results[i] = ((cfe_model *)(self->data))->surface_pressures[inds[i]];
        }
        memcpy(dest, results, nbytes);
        return BMI_SUCCESS;
    }
     */

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

    /*
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
            ((cfe_model *)(self->data))->surface_pressures[inds[i]] = results[i];
        }
        return BMI_SUCCESS;
    }
     */

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

    return 0;
}


cfe_model *new_bmi_cfe(void)
{
    cfe_model *data;
    data = (cfe_model *) malloc(sizeof(cfe_model));
    return data;
}

Bmi* register_bmi(Bmi *model) {
    return register_bmi(model);
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
        model->get_var_location = NULL;

        model->get_current_time = Get_current_time;
        model->get_start_time = Get_start_time;
        model->get_end_time = Get_end_time;
        model->get_time_units = Get_time_units;
        model->get_time_step = Get_time_step;

        model->get_value = Get_value;
        model->get_value_ptr = NULL;
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
