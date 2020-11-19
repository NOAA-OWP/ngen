#include "../include/cfe.h"

#ifndef WATER_SPECIFIC_WEIGHT
#define WATER_SPECIFIC_WEIGHT 9810
#endif

extern void alloc_cfe_model(cfe_model *model) {
    // TODO: *******************
}

extern void free_cfe_model(cfe_model *model) {
    // TODO: ******************
}

extern double get_K_lf_for_time_step(cfe_model* cfe, int time_step_index)
{
    // TODO: make sure this should be the slope param, or set to something else
    double assumed_near_channel_water_table_slope = 0.01; // [L/L]
    double drainage_density_km_per_km2 = 3.5;   // this is approx. the average blue line drainage density for CONUS
    // Equation 10 in parameter equivalence document.
    double Klf = 2.0 * assumed_near_channel_water_table_slope * cfe->NWM_soil_params.mult * cfe->NWM_soil_params.satdk *
                 cfe->NWM_soil_params.D * drainage_density_km_per_km2;   // m/s
    Klf = Klf * cfe->time_step_sizes[time_step_index];  // convert to m/time-step
    return Klf;
}

extern void init_ground_water_reservoir(cfe_model* cfe, double Cgw, double expon, double max_storage, double storage,
                                        int is_storage_ratios)
{
    cfe->gw_reservoir.is_exponential = TRUE;
    cfe->gw_reservoir.storage_max_m = max_storage;

    cfe->gw_reservoir.coeff_primary = Cgw;                  // per h
    cfe->gw_reservoir.exponent_primary = expon;             // linear iff 1.0, non-linear iff > 1.0
    cfe->gw_reservoir.storage_threshold_primary_m = 0.0;    // 0.0 means no threshold applied

    cfe->gw_reservoir.storage_threshold_secondary_m = 0.0;  // 0.0 means no threshold applied
    cfe->gw_reservoir.coeff_secondary = 0.0;                // 0.0 means that secondary outlet is not applied
    cfe->gw_reservoir.exponent_secondary = 1.0;             // linear
    cfe->gw_reservoir.storage_m = init_reservoir_storage(is_storage_ratios, storage, max_storage);
}

extern void init_soil_reservoir(cfe_model* cfe, double alpha_fc, double max_storage, double storage,
                                int is_storage_ratios)
{
    double Klf = get_K_lf_for_time_step(cfe, 0);
    double water_specific_weight = 9810;
    double atm_pressure = cfe->forcings[0].surface_pressure_Pa;

    double trigger_z_m = 0.5;   // distance from bottom of soil column to the center of the lowest discretization

    // calculate the activation storage for the secondary lateral flow outlet in the soil nonlinear reservoir.
    // following the method in the NWM/t-shirt parameter equivalence document, assuming field capacity soil
    // suction pressure = 1/3 atm= field_capacity_atm_press_fraction * atm_press_Pa.

    // equation 3 from NWM/t-shirt parameter equivalence document
    double H_water_table_m = alpha_fc * atm_pressure / water_specific_weight;


    // solve the integral given by Eqn. 5 in the parameter equivalence document.
    // this equation calculates the amount of water stored in the 2 m thick soil column when the water content
    // at the center of the bottom discretization (trigger_z_m) is at field capacity
    double Omega = H_water_table_m - trigger_z_m;
    double lower_lim = pow(Omega, (1.0 - 1.0 / cfe->NWM_soil_params.bb)) / (1.0 - 1.0 / cfe->NWM_soil_params.bb);
    double upper_lim = pow(Omega + cfe->NWM_soil_params.D, (1.0 - 1.0 / cfe->NWM_soil_params.bb)) /
                       (1.0 - 1.0 / cfe->NWM_soil_params.bb);

    // initialize lateral flow function parameters
    //---------------------------------------------
    double field_capacity_storage_threshold_m =
            cfe->NWM_soil_params.smcmax * pow(1.0 / cfe->NWM_soil_params.satpsi, (-1.0 / cfe->NWM_soil_params.bb)) *
            (upper_lim - lower_lim);
    double lateral_flow_threshold_storage_m = field_capacity_storage_threshold_m;  // making them the same, but they don't have 2B

    // Initialize the soil conceptual reservoir data structure.  Indented here to highlight different purposes
    //-------------------------------------------------------------------------------------------------------------
    // soil conceptual reservoir first, two outlets, two thresholds, linear (exponent=1.0).
    cfe->soil_reservoir.is_exponential = FALSE;  // set this true TRUE to use the exponential form of the discharge equation
    // this should NEVER be set to true in the soil reservoir.
    cfe->soil_reservoir.storage_max_m = cfe->NWM_soil_params.smcmax * cfe->NWM_soil_params.D;
    //  vertical percolation parameters------------------------------------------------
    cfe->soil_reservoir.coeff_primary = cfe->NWM_soil_params.satdk * cfe->NWM_soil_params.slop * cfe->time_step_sizes[0]; // m per ts
    cfe->soil_reservoir.exponent_primary = 1.0;      // 1.0=linear
    cfe->soil_reservoir.storage_threshold_primary_m = field_capacity_storage_threshold_m;
    // lateral flow parameters --------------------------------------------------------
    cfe->soil_reservoir.coeff_secondary = Klf;  // 0.0 to deactiv. else =lateral_flow_linear_reservoir_constant;   // m per ts
    cfe->soil_reservoir.exponent_secondary = 1.0;   // 1.0=linear
    cfe->soil_reservoir.storage_threshold_secondary_m = lateral_flow_threshold_storage_m;

    cfe->soil_reservoir.storage_m = init_reservoir_storage(is_storage_ratios, storage, max_storage);
}

extern double init_reservoir_storage(int is_ratio, double amount, double max_amount) {
    // Negative amounts are always ignored and just considered emtpy
    if (amount < 0.0) {
        return 0.0;
    }
    // When not a ratio (and positive), just return the literal amount
    if (is_ratio == FALSE) {
        return amount;
    }
    // When between 0 and 1, return the simple ratio computation
    if (amount <= 1.0) {
        return max_amount * amount;
    }
    // Otherwise, just return the literal amount, and assume the is_ratio value was invalid
    // TODO: is this the best way to handle this?
    else {
        return amount;
    }
}

extern int run(cfe_model* model) {
    double vol_sch_runoff = 0;
    double vol_sch_infilt = 0;
    double vol_to_soil = 0;
    double vol_to_gw = 0;
    double vol_from_gw = 0;
    double vol_soil_to_gw = 0;
    double vol_soil_to_lat_flow = 0;
    double volout = 0;
    double vol_out_giuh = 0;
    double vol_in_nash = 0;
    double vol_out_nash = 0;

    int t_index = model->current_time_step;

    double time_step_size_s = model->time_step_sizes[t_index];

    // TODO: this is probably not correct and needs some conversion
    double timestep_rainfall_input_m = model->forcings[t_index].precip_kg_per_m2;

    //##################################################
    // partition rainfall using Schaake function
    //##################################################
    double soil_reservoir_storage_deficit_m = (model->NWM_soil_params.smcmax * model->NWM_soil_params.D -
                                               model->soil_reservoir.storage_m);
    double timestep_h = time_step_size_s / 3600;
    double Schaake_output_runoff_m, infiltration_depth_m;
    Schaake_partitioning_scheme(timestep_h, model->Schaake_adjusted_magic_constant_by_soil_type,
                                soil_reservoir_storage_deficit_m, timestep_rainfall_input_m,
                                &Schaake_output_runoff_m, &infiltration_depth_m);

    // check to make sure that there is storage available in soil to hold the water that does not runoff
    //--------------------------------------------------------------------------------------------------
    if (soil_reservoir_storage_deficit_m < infiltration_depth_m) {
        Schaake_output_runoff_m += (infiltration_depth_m -
                                    soil_reservoir_storage_deficit_m);  // put won't fit back into runoff
        infiltration_depth_m = soil_reservoir_storage_deficit_m;
        model->soil_reservoir.storage_m = model->soil_reservoir.storage_max_m;
    }

    double flux_overland_m = Schaake_output_runoff_m;

    vol_sch_runoff += flux_overland_m;
    vol_sch_infilt += infiltration_depth_m;

    // Doing things this way because the first time, for first time step, this will be 0.0, and subsequent times it will
    // be whatever was set in previous time step flux calculation.
    double previous_flux_perc_m = t_index == 0 ? 0.0 : model->fluxes[t_index].flux_perc_m;

    if (previous_flux_perc_m > soil_reservoir_storage_deficit_m) {
        double diff = previous_flux_perc_m - soil_reservoir_storage_deficit_m;  // the amount that there is not capacity for
        infiltration_depth_m = soil_reservoir_storage_deficit_m;
        vol_sch_runoff += diff;  // send excess water back to GIUH runoff
        vol_sch_infilt -= diff;  // correct overprediction of infilt.
        flux_overland_m += diff; // bug found by Nels.  This was missing and fixes it.
    }

    vol_to_soil += infiltration_depth_m;
    model->soil_reservoir.storage_m += infiltration_depth_m;  // put the infiltrated water in the soil.

    // calculate fluxes from the soil storage into the deep groundwater (percolation) and to lateral subsurface flow
    //--------------------------------------------------------------------------------------------------------------
    conceptual_reservoir_flux_calc(&(model->soil_reservoir), &(model->fluxes[t_index].flux_perc_m),
                                   &(model->fluxes[t_index].flux_lat_m));

    // calculate flux of base flow from deep groundwater reservoir to channel
    //--------------------------------------------------------------------------
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    double gw_reservoir_storage_deficit_m = model->gw_reservoir.storage_max_m - model->gw_reservoir.storage_m;

    // limit amount transferred to deficit iff there is insufficient avail. storage
    if (model->fluxes[t_index].flux_perc_m > gw_reservoir_storage_deficit_m) {
        double diff = model->fluxes[t_index].flux_perc_m - gw_reservoir_storage_deficit_m;
        model->fluxes[t_index].flux_perc_m = gw_reservoir_storage_deficit_m;
        vol_sch_runoff += diff;  // send excess water back to GIUH runoff
        vol_sch_infilt -= diff;  // correct overprediction of infilt.
    }

    vol_to_gw += model->fluxes[t_index].flux_perc_m;
    vol_soil_to_gw += model->fluxes[t_index].flux_perc_m;

    model->gw_reservoir.storage_m += model->fluxes[t_index].flux_perc_m;
    model->soil_reservoir.storage_m -= model->fluxes[t_index].flux_perc_m;
    model->soil_reservoir.storage_m -= model->fluxes[t_index].flux_lat_m;
    vol_soil_to_lat_flow += model->fluxes[t_index].flux_lat_m;
    volout = volout + model->fluxes[t_index].flux_lat_m;

    double secondary_flux; // Don't think this is actually used for GW reservoir
    conceptual_reservoir_flux_calc(&(model->gw_reservoir), &(model->fluxes[t_index].flux_from_deep_gw_to_chan_m),
                                   &secondary_flux);

    vol_from_gw += model->fluxes[t_index].flux_from_deep_gw_to_chan_m;

    // in the instance of calling the gw reservoir the secondary flux should be zero- verify
    if (is_fabs_less_than_epsilon(secondary_flux, 1.0e-09) == FALSE)
        printf("problem with nonzero flux point 1\n");


    // adjust state of deep groundwater conceptual nonlinear reservoir
    //-----------------------------------------------------------------
    model->gw_reservoir.storage_m -= model->fluxes[t_index].flux_from_deep_gw_to_chan_m;


    // Solve the convolution integral for this time step
    model->fluxes[t_index].giuh_runoff_m = convolution_integral(Schaake_output_runoff_m, model->num_giuh_ordinates,
                                           model->giuh_ordinates, model->runoff_queue_m_per_timestep);
    vol_out_giuh += model->fluxes[t_index].giuh_runoff_m;

    volout += model->fluxes[t_index].giuh_runoff_m;
    volout += model->fluxes[t_index].flux_from_deep_gw_to_chan_m;

    // Route lateral flow through the Nash cascade.
    model->fluxes[t_index].nash_lateral_runoff_m = nash_cascade(model->fluxes[t_index].flux_lat_m,
                                                                model->num_lateral_flow_nash_reservoirs,
                                                                model->K_nash, model->nash_storage);
    vol_in_nash += model->fluxes[t_index].flux_lat_m;
    vol_out_nash += model->fluxes[t_index].nash_lateral_runoff_m;

    #ifdef DEBUG
    //fprintf(out_debug_fptr,"%d %lf %lf\n",tstep,flux_lat_m,nash_lateral_runoff_m);
    #endif

    model->fluxes[t_index].Qout_m =
            model->fluxes[t_index].giuh_runoff_m + model->fluxes[t_index].nash_lateral_runoff_m +
            model->fluxes[t_index].flux_from_deep_gw_to_chan_m;

    // Increment time step count
    model->current_time_step++;

    return 0;
}

//##############################################################
//###################   NASH CASCADE   #########################
//##############################################################
extern double nash_cascade(double flux_lat_m,int num_lateral_flow_nash_reservoirs,
                           double K_nash,double *nash_storage)
{
//##############################################################
// Solve for the flow through the Nash cascade to delay the 
// arrival of the lateral flow into the channel
//##############################################################
// local vars
int i;
double outflow_m;
static double Q[MAX_NUM_NASH_CASCADE];

//Loop through reservoirs
for(i = 0; i < num_lateral_flow_nash_reservoirs; i++)
  {
  Q[i] = K_nash*nash_storage[i];
  nash_storage[i]  -= Q[i];

  if (i==0) nash_storage[i] += flux_lat_m; 
  else      nash_storage[i] +=  Q[i-1];
  }

/*  Get Qout */
outflow_m = Q[num_lateral_flow_nash_reservoirs-1];

//Return the flow output
return (outflow_m);

}

//##############################################################
//############### GIUH CONVOLUTION INTEGRAL   ##################
//##############################################################
extern double convolution_integral(double runoff_m,int num_giuh_ordinates, 
                                   double *giuh_ordinates, double *runoff_queue_m_per_timestep)
{
//##############################################################
// This function solves the convolution integral involving N
//  GIUH ordinates.
//##############################################################
double runoff_m_now;
int N,i;

N=num_giuh_ordinates;
runoff_queue_m_per_timestep[N]=0.0;

for(i=0;i<N;i++)
  {
  runoff_queue_m_per_timestep[i]+=giuh_ordinates[i]*runoff_m;
  }
runoff_m_now=runoff_queue_m_per_timestep[0];

for(i=0;i<N;i++)  // shift all the entries in preperation ffor the next timestep
  {
  runoff_queue_m_per_timestep[i]=runoff_queue_m_per_timestep[i+1];
  }

return(runoff_m_now);
}

//##############################################################
//########## SINGLE OUTLET EXPONENTIAL RESERVOIR ###############
//##########                -or-                 ###############
//##########    TWO OUTLET NONLINEAR RESERVOIR   ###############
//################################################################
// This function calculates the flux from a linear, or nonlinear 
// conceptual reservoir with one or two outlets, or from an
// exponential nonlinear conceptual reservoir with only one outlet.
// In the non-exponential instance, each outlet can have its own
// activation storage threshold.  Flow from the second outlet is 
// turned off by setting the discharge coeff. to 0.0.
//################################################################
extern void conceptual_reservoir_flux_calc(struct conceptual_reservoir *reservoir,
                                           double *primary_flux_m,double *secondary_flux_m)
{
//struct conceptual_reservoir  <<<<INCLUDED HERE FOR REFERENCE.>>>>
//{
// int    is_exponential;  // set this true TRUE to use the exponential form of the discharge equation
// double storage_max_m;
// double storage_m;
// double coeff_primary;
// double exponent_secondary;
// double storage_threshold_primary_m;
// double storage_threshold_secondary_m;
// double coeff_secondary;
// double exponent_secondary;
// };
// THIS FUNCTION CALCULATES THE FLUXES FROM A CONCEPTUAL NON-LINEAR (OR LINEAR) RESERVOIR WITH TWO OUTLETS
// all fluxes calculated by this routine are instantaneous with units of the coefficient.

//local variables
double storage_above_threshold_m;

if(reservoir->is_exponential==TRUE)  // single outlet reservoir like the NWM V1.2 exponential conceptual gw reservoir
  {
  // calculate the one flux and return.
  *primary_flux_m=reservoir->coeff_primary*
                    (exp(reservoir->exponent_primary*reservoir->storage_m/reservoir->storage_max_m)-1.0);
  *secondary_flux_m=0.0;
  return;
  }
// code goes past here iff it is not a single outlet exponential deep groundwater reservoir of the NWM variety
// The vertical outlet is assumed to be primary and satisfied first.

*primary_flux_m=0.0;
storage_above_threshold_m=reservoir->storage_m-reservoir->storage_threshold_primary_m;
if(storage_above_threshold_m>0.0)
  {
  // flow is possible from the primary outlet
  *primary_flux_m=reservoir->coeff_primary*
                pow(storage_above_threshold_m/(reservoir->storage_max_m-reservoir->storage_threshold_primary_m),
                    reservoir->exponent_primary);
  if(*primary_flux_m > storage_above_threshold_m) 
                    *primary_flux_m=storage_above_threshold_m;  // limit to max. available
  }
*secondary_flux_m=0.0;
storage_above_threshold_m=reservoir->storage_m-reservoir->storage_threshold_secondary_m;
if(storage_above_threshold_m>0.0)
  {
  // flow is possible from the secondary outlet
  *secondary_flux_m=reservoir->coeff_secondary*
                  pow(storage_above_threshold_m/(reservoir->storage_max_m-reservoir->storage_threshold_secondary_m),
                      reservoir->exponent_secondary);
  if(*secondary_flux_m > (storage_above_threshold_m-(*primary_flux_m))) 
                    *secondary_flux_m=storage_above_threshold_m-(*primary_flux_m);  // limit to max. available
  }
return;
}


//##############################################################
//#########   SCHAAKE RUNOFF PARTITIONING SCHEME   #############
//##############################################################
void Schaake_partitioning_scheme(double timestep_h, double Schaake_adjusted_magic_constant_by_soil_type, 
           double column_total_soil_moisture_deficit_m,
           double water_input_depth_m,double *surface_runoff_depth_m,double *infiltration_depth_m)
{


/*! ===============================================================================
  This subtroutine takes water_input_depth_m and partitions it into surface_runoff_depth_m and
  infiltration_depth_m using the scheme from Schaake et al. 1996. 
! --------------------------------------------------------------------------------
! ! modified by FLO April 2020 to eliminate reference to ice processes, 
! ! and to de-obfuscate and use descriptive and dimensionally consistent variable names.
! --------------------------------------------------------------------------------
    IMPLICIT NONE
! --------------------------------------------------------------------------------
! inputs
  double timestep_h
  double Schaake_adjusted_magic_constant_by_soil_type = C*Ks(soiltype)/Ks_ref, where C=3, and Ks_ref=2.0E-06 m/s
  double column_total_soil_moisture_deficit_m
  double water_input_depth_m  amount of water input to soil surface this time step [m]

! outputs
  double surface_runoff_depth_m      amount of water partitioned to surface water this time step [m]


--------------------------------------------------------------------------------*/
int k;
double timestep_d,Schaake_parenthetical_term,Ic,Px,infilt_dep_m;


if(0.0 < water_input_depth_m) 
  {
  if (0.0 > column_total_soil_moisture_deficit_m)
    {
    *surface_runoff_depth_m=water_input_depth_m;
    *infiltration_depth_m=0.0;
    }
  else
    {
    // partition time-step total applied water as per Schaake et al. 1996.
                                         // change from dt in [s] to dt1 in [d] because kdt has units of [d^(-1)] 
    timestep_d = timestep_h /24.0;    // timestep_d is the time step in days.
      
    // calculate the parenthetical part of Eqn. 34 from Schaake et al. Note the magic constant has units of [d^(-1)]
    
    Schaake_parenthetical_term = (1.0 - exp ( - Schaake_adjusted_magic_constant_by_soil_type * timestep_d));      
    
    // From Schaake et al. Eqn. 2., using the column total moisture deficit 
    // BUT the way it is used here, it is the cumulative soil moisture deficit in the entire soil profile. 
    // "Layer" info not used in this subroutine in noah-mp, except to sum up the total soil moisture storage.
    // NOTE: when column_total_soil_moisture_deficit_m becomes zero, which occurs when the soil column is saturated, 
    // then Ic=0, where Ic in the Schaake paper is called the "spatially averaged infiltration capacity", 
    // and is defined in Eqn. 12. 

    Ic = column_total_soil_moisture_deficit_m * Schaake_parenthetical_term; 
                                     
    Px=water_input_depth_m;   // Total water input to partitioning scheme this time step [m]
  
    // This is eqn 24 from Schaake et al.  NOTE: this is 0 in the case of a saturated soil column, when Ic=0.  
    // Physically happens only if soil has no-flow lower b.c.
    
    *infiltration_depth_m = (Px * (Ic / (Px + Ic)));  


    if( 0.0 < (water_input_depth_m-(*infiltration_depth_m)) )
      {
      *surface_runoff_depth_m = water_input_depth_m - (*infiltration_depth_m);
      }
    else  *surface_runoff_depth_m=0.0;
    *infiltration_depth_m =  water_input_depth_m - (*surface_runoff_depth_m);
    }
  }
else
  {
  *surface_runoff_depth_m = 0.0;
  *infiltration_depth_m = 0.0;
  }
return;
}

extern int is_fabs_less_than_epsilon(double a,double epsilon)  // returns true if fabs(a)<epsilon
{
if(fabs(a)<epsilon) return(TRUE);
else                return(FALSE);
}



/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/* ALL THE STUFF BELOW HERE IS JUST UTILITY MEMORY AND TIME FUNCTION CODE */
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/


/*####################################################################*/
/*########################### PARSE LINE #############################*/
/*####################################################################*/
void parse_aorc_line(char *theString, long *year, long *month, long *day, long *hour, long *minute, double *second,
                     struct aorc_forcing_data *aorc) {
    char str[20];
    long yr, mo, da, hr, mi;
    double mm, julian, se;
    float val;
    int i, start, end, len;
    int yes_pm, wordlen;
    char theWord[150];

    len = strlen(theString);

    start = 0; /* begin at the beginning of theString */
    get_word(theString, &start, &end, theWord, &wordlen);
    *year = atol(theWord);

    get_word(theString, &start, &end, theWord, &wordlen);
    *month = atol(theWord);

    get_word(theString, &start, &end, theWord, &wordlen);
    *day = atol(theWord);

    get_word(theString, &start, &end, theWord, &wordlen);
    *hour = atol(theWord);

    get_word(theString, &start, &end, theWord, &wordlen);
    *minute = atol(theWord);

    get_word(theString, &start, &end, theWord, &wordlen);
    *second = (double) atof(theWord);

    get_word(theString, &start, &end, theWord, &wordlen);
    aorc->precip_kg_per_m2 = atof(theWord);

    get_word(theString, &start, &end, theWord, &wordlen);
    aorc->incoming_longwave_W_per_m2 = atof(theWord);

    get_word(theString, &start, &end, theWord, &wordlen);
    aorc->incoming_shortwave_W_per_m2 = atof(theWord);

    get_word(theString, &start, &end, theWord, &wordlen);
    aorc->surface_pressure_Pa = atof(theWord);

    get_word(theString, &start, &end, theWord, &wordlen);
    aorc->specific_humidity_2m_kg_per_kg = atof(theWord);

    get_word(theString, &start, &end, theWord, &wordlen);
    aorc->air_temperature_2m_K = atof(theWord);

    get_word(theString, &start, &end, theWord, &wordlen);
    aorc->u_wind_speed_10m_m_per_s = atof(theWord);

    get_word(theString, &start, &end, theWord, &wordlen);
    aorc->v_wind_speed_10m_m_per_s = atof(theWord);


    return;
}

/*####################################################################*/
/*############################## GET WORD ############################*/
/*####################################################################*/
void get_word(char *theString, int *start, int *end, char *theWord, int *wordlen) {
    int i, lenny, j;
    lenny = strlen(theString);

    while (theString[*start] == ' ' || theString[*start] == '\t') {
        (*start)++;
    };

    j = 0;
    for (i = (*start); i < lenny; i++) {
        if (theString[i] != ' ' && theString[i] != '\t' && theString[i] != ',' && theString[i] != ':' &&
            theString[i] != '/') {
            theWord[j] = theString[i];
            j++;
        } else {
            break;
        }
    }
    theWord[j] = '\0';
    *start = i + 1;
    *wordlen = strlen(theWord);
    return;
}

/****************************************/
void itwo_alloc(int ***array, int rows, int cols) {
    int i, frows, fcols, numgood = 0;
    int error = 0;

    if ((rows == 0) || (cols == 0)) {
        printf("Error: Attempting to allocate array of size 0\n");
        exit;
    }

    frows = rows + 1;  /* added one for FORTRAN numbering */
    fcols = cols + 1;  /* added one for FORTRAN numbering */

    *array = (int **) malloc(frows * sizeof(int *));
    if (*array) {
        memset((*array), 0, frows * sizeof(int *));
        for (i = 0; i < frows; i++) {
            (*array)[i] = (int *) malloc(fcols * sizeof(int));
            if ((*array)[i] == NULL) {
                error = 1;
                numgood = i;
                i = frows;
            } else
                memset((*array)[i], 0, fcols * sizeof(int));
        }
    }
    return;
}


void dtwo_alloc(double ***array, int rows, int cols) {
    int i, frows, fcols, numgood = 0;
    int error = 0;

    if ((rows == 0) || (cols == 0)) {
        printf("Error: Attempting to allocate array of size 0\n");
        exit;
    }

    frows = rows + 1;  /* added one for FORTRAN numbering */
    fcols = cols + 1;  /* added one for FORTRAN numbering */

    *array = (double **) malloc(frows * sizeof(double *));
    if (*array) {
        memset((*array), 0, frows * sizeof(double *));
        for (i = 0; i < frows; i++) {
            (*array)[i] = (double *) malloc(fcols * sizeof(double));
            if ((*array)[i] == NULL) {
                error = 1;
                numgood = i;
                i = frows;
            } else
                memset((*array)[i], 0, fcols * sizeof(double));
        }
    }
    return;
}


void d_alloc(double **var, int size) {
    size++;  /* just for safety */

    *var = (double *) malloc(size * sizeof(double));
    if (*var == NULL) {
        printf("Problem allocating memory for array in d_alloc.\n");
        return;
    } else
        memset(*var, 0, size * sizeof(double));
    return;
}

void i_alloc(int **var, int size) {
    size++;  /* just for safety */

    *var = (int *) malloc(size * sizeof(int));
    if (*var == NULL) {
        printf("Problem allocating memory in i_alloc\n");
        return;
    } else
        memset(*var, 0, size * sizeof(int));
    return;
}

/*
 * convert Gregorian days to Julian date
 *
 * Modify as needed for your application.
 *
 * The Julian day starts at noon of the Gregorian day and extends
 * to noon the next Gregorian day.
 *
 */
/*
** Takes a date, and returns a Julian day. A Julian day is the number of
** days since some base date  (in the very distant past).
** Handy for getting date of x number of days after a given Julian date
** (use jdate to get that from the Gregorian date).
** Author: Robert G. Tantzen, translator: Nat Howard
** Translated from the algol original in Collected Algorithms of CACM
** (This and jdate are algorithm 199).
*/


double greg_2_jul(
        long year,
        long mon,
        long day,
        long h,
        long mi,
        double se) {
    long m = mon, d = day, y = year;
    long c, ya, j;
    double seconds = h * 3600.0 + mi * 60 + se;

    if (m > 2)
        m -= 3;
    else {
        m += 9;
        --y;
    }
    c = y / 100L;
    ya = y - (100L * c);
    j = (146097L * c) / 4L + (1461L * ya) / 4L + (153L * m + 2L) / 5L + d + 1721119L;
    if (seconds < 12 * 3600.0) {
        j--;
        seconds += 12.0 * 3600.0;
    } else {
        seconds = seconds - 12.0 * 3600.0;
    }
    return (j + (seconds / 3600.0) / 24.0);
}

/* Julian date converter. Takes a julian date (the number of days since
** some distant epoch or other), and returns an int pointer to static space.
** ip[0] = month;
** ip[1] = day of month;
** ip[2] = year (actual year, like 1977, not 77 unless it was  77 a.d.);
** ip[3] = day of week (0->Sunday to 6->Saturday)
** These are Gregorian.
** Copied from Algorithm 199 in Collected algorithms of the CACM
** Author: Robert G. Tantzen, Translator: Nat Howard
**
** Modified by FLO 4/99 to account for nagging round off error 
**
*/
void calc_date(double jd, long *y, long *m, long *d, long *h, long *mi,
               double *sec) {
    static int ret[4];

    long j;
    double tmp;
    double frac;

    j = (long) jd;
    frac = jd - j;

    if (frac >= 0.5) {
        frac = frac - 0.5;
        j++;
    } else {
        frac = frac + 0.5;
    }

    ret[3] = (j + 1L) % 7L;
    j -= 1721119L;
    *y = (4L * j - 1L) / 146097L;
    j = 4L * j - 1L - 146097L * *y;
    *d = j / 4L;
    j = (4L * *d + 3L) / 1461L;
    *d = 4L * *d + 3L - 1461L * j;
    *d = (*d + 4L) / 4L;
    *m = (5L * *d - 3L) / 153L;
    *d = 5L * *d - 3 - 153L * *m;
    *d = (*d + 5L) / 5L;
    *y = 100L * *y + j;
    if (*m < 10)
        *m += 3;
    else {
        *m -= 9;
        *y = *y + 1; /* Invalid use: *y++. Modified by Tony */
    }

    /* if (*m < 3) *y++; */
    /* incorrectly repeated the above if-else statement. Deleted by Tony.*/

    tmp = 3600.0 * (frac * 24.0);
    *h = (long) (tmp / 3600.0);
    tmp = tmp - *h * 3600.0;
    *mi = (long) (tmp / 60.0);
    *sec = tmp - *mi * 60.0;
}

int dayofweek(double j) {
    j += 0.5;
    return (int) (j + 1) % 7;
}

