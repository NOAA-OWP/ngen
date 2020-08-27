#include "tshirt_c.h"
#include "Constants.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// TODO: remove this ASAP
void d_alloc(double **var, int size) {
    size++;  /* just for safety */

    *var = (double *) malloc(size * sizeof(double));
    if (*var == NULL) {
        printf("Problem allocating memory for array in d_alloc.\n");
        return;
    } else memset(*var, 0, size * sizeof(double));
}

/**
 * Solve for the flow through a Nash Cascade to delay the arrival of some flow.
 *
 * @param flux_lat_m Input to the cascade in meters.
 * @param num_lateral_flow_nash_reservoirs The number of internal reservoirs for the Nash Cascade.
 * @param K_nash The 'K' parameter for the cascade reservoirs.
 * @param nash_storage Pointer to head of storage array for cascade reservoirs.
 * @return The outflow from the cascade, in meters.
 */
 // TODO: update with more agnostic parameter names once fully tested and testable in the future.
extern double nash_cascade(double flux_lat_m, int num_lateral_flow_nash_reservoirs,
                           double K_nash, double *nash_storage) {
    // TODO: update this with more modern features once fully tested
    int i;
    double outflow_m;
    static double Q[MAX_NUM_NASH_CASCADE];

    // Loop through reservoirs
    for (i = 0; i < num_lateral_flow_nash_reservoirs; i++) {
        Q[i] = K_nash * nash_storage[i];
        nash_storage[i] -= Q[i];

        if (i == 0) nash_storage[i] += flux_lat_m;
        else nash_storage[i] += Q[i - 1];
    }

    /* Get Qout */
    outflow_m = Q[num_lateral_flow_nash_reservoirs - 1];

    // Return the flow output
    return (outflow_m);

}

/**
 * Perform GIUH convolution integral solver calculation for an array of ordinates.
 *
 * @param runoff_m Input runoff in meters.
 * @param num_giuh_ordinates The number of GIUH ordinates.
 * @param giuh_ordinates Pointer to the head of an array of GUIH ordinates.
 * @param runoff_queue_m_per_timestep Pointer to array of calculated convolution queue runoff values for the ordinates.
 * @return The calculated amount for current GIUH runoff in meters.
 */
extern double convolution_integral(double runoff_m, int num_giuh_ordinates,
                                   double *giuh_ordinates, double *runoff_queue_m_per_timestep) {
    //##############################################################
    // This function solves the convolution integral involving N
    //  GIUH ordinates.
    //##############################################################
    double runoff_m_now;
    int N, i;

    N = num_giuh_ordinates;
    runoff_queue_m_per_timestep[N] = 0.0;

    for (i = 0; i < N; i++) {
        runoff_queue_m_per_timestep[i] += giuh_ordinates[i] * runoff_m;
    }
    runoff_m_now = runoff_queue_m_per_timestep[0];

    // shift all the entries in preparation for the next time step
    for (i = 0; i < N; i++) {
        runoff_queue_m_per_timestep[i] = runoff_queue_m_per_timestep[i + 1];
    }

    return (runoff_m_now);
}

/**
 * Perform calculations for the flux for a conceptual reservoir with one or two outlet
 *
 * Perform calculations for the flux for a conceptual reservoir (linear or nonlinear) with one or two outlets, or an
 * exponential nonlinear conceptual reservoir with a single outlet.  In the case of non-exponential reservoir, each
 * outlet can have its own activation storage threshold.
 *
 * Flow for the second outlet is turned off in the conceptual reservoir by setting the discharge coefficient to 0.0.
 *
 * All fluxes calculated by this routine are instantaneous with units of the coefficient.
 *
 * Flux values are returned by reference pointers passed in as arguments.
 *
 * @param reservoir The #conceptual_reservoir structure.
 * @param primary_flux_m Pointer to location for holding calculated flux (in meters) for primary outlet.
 * @param secondary_flux_m Pointer to location for holding calculated flux (in meters) for secondary outlet.
 */
extern void conceptual_reservoir_flux_calc(struct conceptual_reservoir *reservoir,
                                           double *primary_flux_m, double *secondary_flux_m)
{
    /*
     * <<<<INCLUDED HERE FOR REFERENCE.>>>>
     *
    struct conceptual_reservoir
    {
        int    is_exponential;  // set this true TRUE to use the exponential form of the discharge equation
        double storage_max_m;
        double storage_m;
        double coeff_primary;
        double exponent_secondary;
        double storage_threshold_primary_m;
        double storage_threshold_secondary_m;
        double coeff_secondary;
        double exponent_secondary;
    };

    */

    // local variables
    double storage_above_threshold_m;

    if (reservoir->is_exponential ==
        TRUE)  // single outlet reservoir like the NWM V1.2 exponential conceptual gw reservoir
    {
        // calculate the one flux and return.
        *primary_flux_m = reservoir->coeff_primary *
                          (exp(reservoir->exponent_primary * reservoir->storage_m / reservoir->storage_max_m) - 1.0);
        *secondary_flux_m = 0.0;
        return;
    }
    // code goes past here iff it is not a single outlet exponential deep groundwater reservoir of the NWM variety
    // The vertical outlet is assumed to be primary and satisfied first.

    *primary_flux_m = 0.0;
    storage_above_threshold_m = reservoir->storage_m - reservoir->storage_threshold_primary_m;
    if (storage_above_threshold_m > 0.0) {
        // flow is possible from the primary outlet
        *primary_flux_m = reservoir->coeff_primary *
                          pow(storage_above_threshold_m /
                              (reservoir->storage_max_m - reservoir->storage_threshold_primary_m),
                              reservoir->exponent_primary);
        if (*primary_flux_m > storage_above_threshold_m)
            *primary_flux_m = storage_above_threshold_m;  // limit to max. available
    }
    *secondary_flux_m = 0.0;
    storage_above_threshold_m = reservoir->storage_m - reservoir->storage_threshold_secondary_m;
    if (storage_above_threshold_m > 0.0) {
        // flow is possible from the secondary outlet
        *secondary_flux_m = reservoir->coeff_secondary *
                            pow(storage_above_threshold_m /
                                (reservoir->storage_max_m - reservoir->storage_threshold_secondary_m),
                                reservoir->exponent_secondary);
        if (*secondary_flux_m > (storage_above_threshold_m - (*primary_flux_m)))
            *secondary_flux_m = storage_above_threshold_m - (*primary_flux_m);  // limit to max. available
    }
    return;
}

/**
 * Perform Schaake Runoff partitioning.
 *
 * This function takes water_input_depth_m and partitions it into surface_runoff_depth_m and infiltration_depth_m using
 * the scheme from Schaake et al. 1996.  The logic has been modified to eliminate reference to ice processes, and to
 * and to de-obfuscate and use descriptive and dimensionally consistent variable names.
 *
 * Function "returns" the surface runoff and infiltration amounts via passed in reference pointer parameters.
 *
 * Note the folloing general formula for the Schaake magic constant:
 *      Schaake_adjusted_magic_constant_by_soil_type = C*Ks(soiltype)/Ks_ref, where C=3, and Ks_ref=2.0E-06 m/s
 *
 * @param timestep_h Time step size in hours
 * @param Schaake_adjusted_magic_constant_by_soil_type Magic constant value for soil type.
 * @param column_total_soil_moisture_deficit_m Column moisture deficit in meters.
 * @param water_input_depth_m Amount of input water to soil surface this time step, in meters.
 * @param surface_runoff_depth_m Pointer for partitioned surface runoff for this time step, in meters.
 * @param infiltration_depth_m Pointer for partitioned infiltration depth for this time step, in meters.
 */
void Schaake_partitioning_scheme(double timestep_h, double Schaake_adjusted_magic_constant_by_soil_type,
                                 double column_total_soil_moisture_deficit_m, double water_input_depth_m,
                                 double *surface_runoff_depth_m, double *infiltration_depth_m) {
    int k;
    double timestep_d, Schaake_parenthetical_term, Ic, Px, infilt_dep_m;


    if (0.0 < water_input_depth_m) {
        if (0.0 > column_total_soil_moisture_deficit_m) {
            *surface_runoff_depth_m = water_input_depth_m;
            *infiltration_depth_m = 0.0;
        }
        else {
            // partition time-step total applied water as per Schaake et al. 1996.
            // change from dt in [s] to dt1 in [d] because kdt has units of [d^(-1)]
            timestep_d = timestep_h / 24.0;    // timestep_d is the time step in days.

            // calculate the parenthetical part of Eqn. 34 from Schaake et al. Note the magic constant has units of [d^(-1)]

            Schaake_parenthetical_term = (1.0 - exp(-Schaake_adjusted_magic_constant_by_soil_type * timestep_d));

            // From Schaake et al. Eqn. 2., using the column total moisture deficit
            // BUT the way it is used here, it is the cumulative soil moisture deficit in the entire soil profile.
            // "Layer" info not used in this subroutine in noah-mp, except to sum up the total soil moisture storage.
            // NOTE: when column_total_soil_moisture_deficit_m becomes zero, which occurs when the soil column is saturated,
            // then Ic=0, where Ic in the Schaake paper is called the "spatially averaged infiltration capacity",
            // and is defined in Eqn. 12.

            Ic = column_total_soil_moisture_deficit_m * Schaake_parenthetical_term;

            Px = water_input_depth_m;   // Total water input to partitioning scheme this time step [m]

            // This is eqn 24 from Schaake et al.  NOTE: this is 0 in the case of a saturated soil column, when Ic=0.
            // Physically happens only if soil has no-flow lower b.c.

            *infiltration_depth_m = (Px * (Ic / (Px + Ic)));


            if (0.0 < (water_input_depth_m - (*infiltration_depth_m))) {
                *surface_runoff_depth_m = water_input_depth_m - (*infiltration_depth_m);
            } else *surface_runoff_depth_m = 0.0;

            *infiltration_depth_m = water_input_depth_m - (*surface_runoff_depth_m);
        }
    }
    else {
        *surface_runoff_depth_m = 0.0;
        *infiltration_depth_m = 0.0;
    }
    return;
}

/**
 * Test if ``fabs(a)`` is less than epsilon.
 *
 * @param a The fabs function argument value.
 * @param epsilon The epsilon value.
 * @return Whether ``fabs(a)`` is less than epsilon.
 */
// TODO: adjust to using bool type once verified and testing is in place
extern int is_fabs_less_than_epsilon(double a, double epsilon) {
    if (fabs(a) < epsilon) return (TRUE);
    else return (FALSE);
}

 // TODO: probably need to produce a "generic" parameters struct or object somehow
 // TODO: convert from tshirt_params to NWM_soil_parameters struct via separate function in realization and test.
/**
 * Run the model, recording fluxes in one or more flux structs at a specified location.
 *
 * @param NWM_soil_params Struct containing soil model params
 * @param gw_reservoir Struct containing state for ground water reservoir
 * @param soil_reservoir Struct containing state for soil reservoir
 * @param num_timesteps The number of time steps, which PROBABLY should always be 1
 * @param giuh_ordinates A pointer to the head of an array of GIUH ordinate values
 * @param num_giuh_ordinates The number of elements in the array pointed to by ``giuh_ordinates``
 * @param field_capacity_atm_press_fraction Fraction of atmospheric pressure for field capacity suction pressure (also alpha_fc)
 * @param water_table_slope Assumed near channel water table slope lateral flow parameter
 * @param Schaake_adjusted_magic_constant_by_soil_type Schaake magic constant for soil type
 * @param lateral_flow_linear_reservoir_constant Later flow reservoir constant (Klf)
 * @param K_nash Nash reservoir flow constant
 * @param num_lateral_flow_nash_reservoirs The number of reservoirs in the cascade for the lateral flow
 * @param nash_storage Pointer to head of array of already-set Nash Cascade reservoir storage values
 * @param yes_aorc Whether
 * @param aorc_data Pointer to head of array of per-time-step AORC data structs, when yes_aorc is TRUE (array of size num_timesteps)
 * @param rain_rate Pointer to head of array of per-time-step simple rain rate data (in mm/h), when yes_aorc is FALSE (array of size num_timesteps)
 * @param num_added_fluxes Reference for number of fluxes added to 'fluxes' parameter array; initialized to 0.
 * @param fluxes Pointer to head of array of array per-time-step resulting flux values (array of size num_timesteps)
 * @return
 */
extern int run(NWM_soil_parameters& NWM_soil_params,
               conceptual_reservoir& gw_reservoir,
               conceptual_reservoir& soil_reservoir,
               int num_timesteps,
               double* giuh_ordinates,
               int num_giuh_ordinates,
               double field_capacity_atm_press_fraction,
               double water_table_slope,
               double Schaake_adjusted_magic_constant_by_soil_type,
               double lateral_flow_linear_reservoir_constant,
               double K_nash,
               int num_lateral_flow_nash_reservoirs,
               double* nash_storage,
               int yes_aorc,
               aorc_forcing_data* aorc_data,
               double* rain_rate,
               int& num_added_fluxes,
               tshirt_c_result_fluxes* fluxes)
{
    // Do this to be safe ...
    num_added_fluxes = 0;

    /* Commenting out, since this will need to return values rather than write results to a file.
    FILE *in_fptr;
    FILE *out_fptr;
    FILE *out_debug_fptr;
    */

    // local variables
    //-------------------------------------------------------------------
    int i;
    int tstep;
    double upper_lim;
    double lower_lim;
    double diff;
    char theString[513];   // dangerously hard coded string size... TODO fixme.
    long year, month, day, hour, minute;
    double dsec;
    double jdate_start = 0.0;

    double catchment_area_km2 = 5.0;            // in the range of our desired size
    double drainage_density_km_per_km2 = 3.5;   // this is approx. the average blue line drainage density for CONUS

    //int num_timesteps;
    int num_rain_dat;
    double timestep_h;

    // forcing
    //double *rain_rate = NULL;
    double timestep_rainfall_input_m;
    //int yes_aorc = TRUE;                  // change to TRUE to read in entire AORC precip/met. forcing data set.

    //Schaake partitioning function parameters

    //double Schaake_adjusted_magic_constant_by_soil_type;
    double Schaake_output_runoff_m;
    double infiltration_depth_m;

    // GIUH state & parameters
    // The GIUH ordinates (and ordinates count) now has to be passed in, rather than hard coded
    //double *giuh_ordinates = NULL;                  // assumed GIUH hydrograph ordinates
    double *runoff_queue_m_per_timestep = NULL;
    //int num_giuh_ordinates;
    double giuh_runoff_m;
    double soil_reservoir_storage_deficit_m;        // the available space in the soil conceptual reservoir

    // groundwater storage parameters and state

    // lateral flow function parameters
    double lateral_flow_threshold_storage_m;
    double field_capacity_storage_threshold_m;
    //double lateral_flow_linear_reservoir_constant;
    //int num_lateral_flow_nash_reservoirs;
    //double K_nash;  // lateral_flow_nash_cascade_reservoir_constant;
    //double *nash_storage = NULL;
    double assumed_near_channel_water_table_slope; // [L/L]
    double nash_lateral_runoff_m;

    // calculated flux variables
    double flux_overland_m;                // flux of surface runoff that goes through the GIUH convolution process
    double flux_perc_m = 0.0;                // flux from soil to deeper groundwater reservoir
    double flux_lat_m;                     // lateral flux in the subsurface to the Nash cascade
    double flux_from_deep_gw_to_chan_m;    // flux from the deep reservoir into the channels
    double gw_reservoir_storage_deficit_m; // the available space in the conceptual groundwater reservoir
    double primary_flux, secondary_flux;    // temporary vars.

    //double field_capacity_atm_press_fraction; // [-]
    double soil_water_content_at_field_capacity; // [V/V]

    double atm_press_Pa = STANDARD_ATMOSPHERIC_PRESSURE_PASCALS;
    double unit_weight_water_N_per_m3 = WATER_SPECIFIC_WEIGHT;

    double H_water_table_m;  // Hwt in NWM/t-shirt parameter equiv. doc  discussed between Eqn's 4 and 5.
    // this is the distance down to a fictitious water table from the point there the
    // soil water content is assumed equal to field capacity at the middle of lowest discretization
    double trigger_z_m;      // this is the distance up from the bottom of the soil that triggers lateral flow when
    // the soil water content equals field capacity.   0.5 for center of bottom discretization
    double Omega;            // The limits of integration used in Eqn. 5 of the parameter equivalence document.

    double Qout_m;           // the sum of giuh, nash-cascade, and base flow outputs m per timestep

    //struct conceptual_reservoir soil_reservoir;
    //struct conceptual_reservoir gw_reservoir;
    //struct NWM_soil_parameters NWM_soil_params;
    //struct aorc_forcing_data aorc_data;

    // mass balance variables.  These all store cumulative discharges per unit catchment area [m3/m2] or [m]
    //-----------------------

    // TODO: probably need to eventually move these out of here to realization/formulation, since now this only handles one time step at a time

    double volstart = 0.0;
    double vol_sch_runoff = 0.0;
    double vol_sch_infilt = 0.0;

    double vol_out_giuh = 0.0;
    double vol_end_giuh = 0.0;

    double vol_to_gw = 0.0;
    double vol_in_gw_start = 0.0;
    double vol_in_gw_end = 0.0;
    double vol_from_gw = 0.0;

    double vol_in_nash = 0.0;
    double vol_in_nash_end = 0.0;  // note the nash cascade is empty at start of simulation.
    double vol_out_nash = 0.0;

    double vol_soil_start = 0.0;
    double vol_to_soil = 0.0;
    double vol_soil_to_lat_flow = 0.0;
    double vol_soil_to_gw = 0.0;  // this should equal vol_to_gw
    double vol_soil_end = 0.0;
    // note, vol_from_soil_to_gw is same as vol_to_gw.

    double volin = 0.0;
    double volout = 0.0;
    double volend = 0.0;

    /* Commenting out, since this will need to return values rather than write results to a file.
    if ((out_fptr = fopen("test.out", "w")) == NULL) {
        printf("Can't open output file\n");
        return -1;
    }
     */

    #ifdef DEBUG
    /* Commenting out, since this will need to return values rather than write results to a file.
    if ((out_debug_fptr = fopen("debug.out", "w")) == NULL) {
        printf("Can't open output file\n");
        return -1;
    }
     */
    #endif

    //d_alloc(&giuh_ordinates, MAX_NUM_GIUH_ORDINATES);  // allocate memory to store the GIUH ordinates
    // TODO: come back and try to avoid using d_alloc (or, implicitly, malloc), so use array in stack to get a pointer
    d_alloc(&runoff_queue_m_per_timestep, MAX_NUM_GIUH_ORDINATES+1); // allocate memory to store convolution queue
    //double runoff_queue_array[MAX_NUM_GIUH_ORDINATES+1];
    //runoff_queue_m_per_timestep = &runoff_queue_array[0];

    // catchment properties
    //------------------------
    catchment_area_km2 = 5.0;

    //initialize simulation constants
    //---------------------------
    //num_timesteps = 10000;
    // setting to 1 for this, since this should only be concerned with a single time step
    num_timesteps = 1;
    timestep_h = 1.0;

    // This is now passed in directly
    //initialize NWM soil parameters, using LOAM soils from SOILPARM.TBL.
    //--------------------------------------------------------------------
    /*NWM_soil_params.smcmax=0.439;   // [V/V]
    NWM_soil_params.wltsmc=0.066;   // [V/V]
    NWM_soil_params.satdk=3.38e-06; // [m per sec.]
    NWM_soil_params.satpsi=0.355;   // [m]
    NWM_soil_params.bb=4.05;        // [-]    -- called "bexp" in NWM
    NWM_soil_params.slop=1.0;    // calibration factor that varies from 0 (no flow) to 1 (free drainage) at soil bottom
    NWM_soil_params.D=2.0;       // [m] soil thickness assumed in the NWM not from SOILPARM.TBL
    NWM_soil_params.mult=1000.0;    // not from SOILPARM.TBL, This is actually calibration parameter: LKSATFAC*/

    trigger_z_m = 0.5;   // distance from the bottom of the soil column to the center of the lowest discretization

    // calculate the activation storage ffor the secondary lateral flow outlet in the soil nonlinear reservoir.
    // following the method in the NWM/t-shirt parameter equivalence document, assuming field capacity soil
    // suction pressure = 1/3 atm= field_capacity_atm_press_fraction * atm_press_Pa.

    //field_capacity_atm_press_fraction = 0.33;  //alpha in Eqn. 3.

    // equation 3 from NWM/t-shirt parameter equivalence document
    H_water_table_m = field_capacity_atm_press_fraction * atm_press_Pa / unit_weight_water_N_per_m3;
    soil_water_content_at_field_capacity=NWM_soil_params.smcmax *
                                         pow(H_water_table_m / NWM_soil_params.satpsi, (1.0 / NWM_soil_params.bb));

    // solve the integral given by Eqn. 5 in the parameter equivalence document.
    // this equation calculates the amount of water stored in the 2 m thick soil column when the water content
    // at the center of the bottom discretization (trigger_z_m) is at field capacity
    Omega = H_water_table_m - trigger_z_m;
    lower_lim = pow(Omega, (1.0 - (1.0 / NWM_soil_params.bb))) / (1.0 - (1.0 / NWM_soil_params.bb));
    upper_lim = pow(Omega + NWM_soil_params.D, (1.0 - (1.0 / NWM_soil_params.bb))) / (1.0 - (1.0 / NWM_soil_params.bb));
    field_capacity_storage_threshold_m = NWM_soil_params.smcmax
            * pow((1.0 / NWM_soil_params.satpsi), (-1.0 / NWM_soil_params.bb))
            * (upper_lim - lower_lim);

    //printf("field capacity storage threshold = %lf m\n", field_capacity_storage_threshold_m);

    // initialize giuh parameters.  Can no longer hard code these though.
    //------------------------------------------------------------------
    /*
    num_giuh_ordinates=5;
    */
    // We can still do this part though, but for now leaving it out also
    /*
    if(num_giuh_ordinates > MAX_NUM_GIUH_ORDINATES) {
        fprintf(stderr, "Big problem, number of giuh ordinates greater than MAX_NUM_GIUH_ORDINATES.\n");
        return -1;
    }
    */
    /*
    giuh_ordinates[0]=0.06;  // note these sum to 1.0.  If we have N ordinates, we need a queue sized N+1 to perform
    giuh_ordinates[1]=0.51;  // the convolution.
    giuh_ordinates[2]=0.28;
    giuh_ordinates[3]=0.12;
    giuh_ordinates[4]=0.03;
    */

    // initialize Schaake parameters
    //--------------------------------
    // in noah-mp refkdt=3.0.

    // This also is now coming from Tshirt params, and so passed in directly
    //Schaake_adjusted_magic_constant_by_soil_type=refkdt*NWM_soil_params.satdk/2.0e-06; // 2.0e-06 is used in noah-mp

    // initialize lateral flow function parameters
    //---------------------------------------------
    // TODO: think this is slope from params (and tshirt params), but verify
    //assumed_near_channel_water_table_slope=0.01; // [L/L]
    assumed_near_channel_water_table_slope = water_table_slope;
    lateral_flow_threshold_storage_m = field_capacity_storage_threshold_m;  // making them the same, but they don't have 2B

    // Equation 10 in parameter equivalence document.
    // TODO: this should be Klf, which is in tshirt params, so now passed in (though again, verify and verify init
    //  calculations)
    //lateral_flow_linear_reservoir_constant=2.0*assumed_near_channel_water_table_slope*NWM_soil_params.mult*
    //                                       NWM_soil_params.satdk*NWM_soil_params.D*drainage_density_km_per_km2;   // m/s
    //lateral_flow_linear_reservoir_constant*=3600.0;  // convert to m/h

    // Also now in tshirt params, so passed into function as param
    //num_lateral_flow_nash_reservoirs=2;

    if (num_lateral_flow_nash_reservoirs > MAX_NUM_NASH_CASCADE) {
        fprintf(stdout, "Number of Nash Cascade linear reservoirs greater than MAX_NUM_NASH_CASCADE.\n");
        return -2;
    }

    // TODO: initialize the ground water and soil conceptual reservoirs outside of function now (i.e., in realization
    //  class) and pass into function as parameters

    volstart += gw_reservoir.storage_m;    // initial mass balance checks in g.w. reservoir
    vol_in_gw_start = gw_reservoir.storage_m;

    volstart += soil_reservoir.storage_m;    // initial mass balance checks in soil reservoir
    vol_soil_start = soil_reservoir.storage_m;

    //d_alloc(&rain_rate, MAX_NUM_RAIN_DATA);

    // Rather than read in forcing or rainfall data from file (as in original) function receives directly as param
    num_rain_dat = num_timesteps;
    if(yes_aorc == TRUE) {
        for (i = 0; i < num_rain_dat; i++) {
            //fgets(theString, 512, in_fptr);  // read in a line of AORC data.
            //parse_aorc_line(theString, &year, &month, &day, &hour, &minute, &dsec, &aorc_data);
            //if (i == 0)
            //    jdate_start = greg_2_jul(year, month, day, hour, minute,
            //                             dsec);      // calc & save the starting julian date of the rainfall data
            // saving only the rainfall data ffor now.
            rain_rate[i] = (double) aorc_data[i].precip_kg_per_m2;  // assumed 1000 kg/m3 density of water.  This result is mm/h;
        }
    }

    // run the model for num_timesteps
    //---------------------------------------

    double lateral_flux;      // flux from soil to lateral flow Nash cascade +to cascade
    double percolation_flux;  // flux from soil to gw nonlinear researvoir, +downward
    double oldval;

    //###################################################################################
    //############################      TIME LOOP       #################################
    //###################################################################################

    // For this, just do a single time step, though leaving coding structure in place for loop (for now) to minimize changes
    //num_timesteps=num_rain_dat+279;  // run a little bit beyond the rain data to see what happens.
    for(tstep = 0; tstep < num_timesteps; tstep++) {
        if (tstep < num_rain_dat)
            timestep_rainfall_input_m = rain_rate[tstep] / 1000.0;  // convert from mm/h to m w/ 1h timestep
        else
            timestep_rainfall_input_m = 0.0;

        volin += timestep_rainfall_input_m;

        //##################################################
        // partition rainfall using Schaake function
        //##################################################

        soil_reservoir_storage_deficit_m=(NWM_soil_params.smcmax * NWM_soil_params.D - soil_reservoir.storage_m);

        Schaake_partitioning_scheme(timestep_h, Schaake_adjusted_magic_constant_by_soil_type,
                                    soil_reservoir_storage_deficit_m, timestep_rainfall_input_m,
                                    &Schaake_output_runoff_m, &infiltration_depth_m);

        // check to make sure that there is storage available in soil to hold the water that does not runoff
        //--------------------------------------------------------------------------------------------------
        if (soil_reservoir_storage_deficit_m < infiltration_depth_m) {
            Schaake_output_runoff_m += (infiltration_depth_m -
                                        soil_reservoir_storage_deficit_m);  // put won't fit back into runoff
            infiltration_depth_m = soil_reservoir_storage_deficit_m;
            soil_reservoir.storage_m = soil_reservoir.storage_max_m;
        }
        //printf("After Schaake function: rain:%8.5lf mm  runoff:%8.5lf mm  infiltration:%8.5lf mm  residual:%e m\n",
        //       rain_rate[tstep], Schaake_output_runoff_m * 1000.0, infiltration_depth_m * 1000.0,
        //       timestep_rainfall_input_m - Schaake_output_runoff_m - infiltration_depth_m);

        flux_overland_m = Schaake_output_runoff_m;

        vol_sch_runoff += flux_overland_m;
        vol_sch_infilt += infiltration_depth_m;

        // put infiltration flux into soil conceptual reservoir.  If not enough room
        // limit amount transferred to deficit
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        //  REDUNDANT soil_reservoir_storage_deficit_m=soil_reservoir.storage_max_m-soil_reservoir.storage_m;  <- commented out by FLO based on comments from Bartel

        if (flux_perc_m > soil_reservoir_storage_deficit_m) {
            diff = flux_perc_m - soil_reservoir_storage_deficit_m;  // the amount that there is not capacity ffor
            infiltration_depth_m = soil_reservoir_storage_deficit_m;
            vol_sch_runoff += diff;  // send excess water back to GIUH runoff
            vol_sch_infilt -= diff;  // correct overprediction of infilt.
            flux_overland_m += diff; // bug found by Nels.  This was missing and fixes it.
        }

        vol_to_soil += infiltration_depth_m;
        soil_reservoir.storage_m += infiltration_depth_m;  // put the infiltrated water in the soil.

        // calculate fluxes from the soil storage into the deep groundwater (percolation) and to lateral subsurface flow
        //--------------------------------------------------------------------------------------------------------------
        conceptual_reservoir_flux_calc(&soil_reservoir, &percolation_flux, &lateral_flux);

        flux_perc_m = percolation_flux;  // m/h   <<<<<<<<<<<  flux of percolation from soil to g.w. reservoir >>>>>>>>>

        flux_lat_m = lateral_flux;  // m/h        <<<<<<<<<<<  flux into the lateral flow Nash cascade >>>>>>>>

        // calculate flux of base flow from deep groundwater reservoir to channel
        //--------------------------------------------------------------------------
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        gw_reservoir_storage_deficit_m = gw_reservoir.storage_max_m - gw_reservoir.storage_m;

        // limit amount transferred to deficit iff there is insufficient avail. storage
        if (flux_perc_m > gw_reservoir_storage_deficit_m) {
            diff = flux_perc_m - gw_reservoir_storage_deficit_m;
            flux_perc_m = gw_reservoir_storage_deficit_m;
            vol_sch_runoff += diff;  // send excess water back to GIUH runoff
            vol_sch_infilt -= diff;  // correct overprediction of infilt.
        }

        vol_to_gw += flux_perc_m;
        vol_soil_to_gw += flux_perc_m;

        gw_reservoir.storage_m += flux_perc_m;
        soil_reservoir.storage_m -= flux_perc_m;
        soil_reservoir.storage_m -= flux_lat_m;
        vol_soil_to_lat_flow += flux_lat_m;  //TODO add this to nash cascade as input
        volout = volout + flux_lat_m;

        conceptual_reservoir_flux_calc(&gw_reservoir, &primary_flux, &secondary_flux);

        flux_from_deep_gw_to_chan_m = primary_flux;  // m/h   <<<<<<<<<< BASE FLOW FLUX >>>>>>>>>
        vol_from_gw += flux_from_deep_gw_to_chan_m;

        // in the instance of calling the gw reservoir the secondary flux should be zero- verify
        if (is_fabs_less_than_epsilon(secondary_flux, 1.0e-09) == FALSE) printf("problem with nonzero flux point 1\n");


        // adjust state of deep groundwater conceptual nonlinear reservoir
        //-----------------------------------------------------------------

        gw_reservoir.storage_m -= flux_from_deep_gw_to_chan_m;


        // Solve the convolution integral ffor this time step

        giuh_runoff_m = convolution_integral(Schaake_output_runoff_m, num_giuh_ordinates,
                                             giuh_ordinates, runoff_queue_m_per_timestep);
        vol_out_giuh += giuh_runoff_m;

        volout += giuh_runoff_m;
        volout += flux_from_deep_gw_to_chan_m;

        // Route lateral flow through the Nash cascade.
        nash_lateral_runoff_m = nash_cascade(flux_lat_m, num_lateral_flow_nash_reservoirs,
                                             K_nash, nash_storage);
        vol_in_nash += flux_lat_m;
        vol_out_nash += nash_lateral_runoff_m;

        #ifdef DEBUG
        //fprintf(out_debug_fptr,"%d %lf %lf\n",tstep,flux_lat_m,nash_lateral_runoff_m);
        #endif

        Qout_m = giuh_runoff_m + nash_lateral_runoff_m + flux_from_deep_gw_to_chan_m;

        // <<<<<<<<NOTE>>>>>>>>>
        // These fluxs are all in units of meters per time step.   Multiply them by the "catchment_area_km2" variable
        // to convert them into cubic meters per time step.

        // Not doing this in this implementation
        // WRITE OUTPUTS IN mm/h ffor aiding in interpretation
        //fprintf(out_fptr,"%16.8lf %lf %lf %lf %lf %lf %lf\n",jdate_start+(double)tstep*1.0/24.0,
        //        timestep_rainfall_input_m*1000.0,
        //        Schaake_output_runoff_m*1000.0,
        //        giuh_runoff_m*1000.0,
        //        nash_lateral_runoff_m*1000.0,
        //        flux_from_deep_gw_to_chan_m*1000.0,
        //        Qout_m*1000.0 );

        fluxes[tstep].timestep_rainfall_input_m = timestep_rainfall_input_m;
        fluxes[tstep].Schaake_output_runoff_m = Schaake_output_runoff_m;
        fluxes[tstep].giuh_runoff_m = giuh_runoff_m;
        fluxes[tstep].nash_lateral_runoff_m = nash_lateral_runoff_m;
        fluxes[tstep].flux_from_deep_gw_to_chan_m = flux_from_deep_gw_to_chan_m;
        fluxes[tstep].Qout_m = Qout_m;

        num_added_fluxes++;

    }

    //
    // PERFORM MASS BALANCE CHECKS AND WRITE RESULTS TO stderr.
    //----------------------------------------------------------

    volend = soil_reservoir.storage_m + gw_reservoir.storage_m;
    vol_in_gw_end = gw_reservoir.storage_m;

    #ifdef DEBUG
    //fclose(out_debug_fptr);
    #endif

    // the GIUH queue might have water in it at the end of the simulation, so sum it up.
    for(i = 0; i < num_giuh_ordinates; i++)
        vol_end_giuh += runoff_queue_m_per_timestep[i];

    for(i = 0; i < num_lateral_flow_nash_reservoirs; i++)
        vol_in_nash_end += nash_storage[i];

    vol_soil_end = soil_reservoir.storage_m;

    // Release the things we malloc-ed
    free(runoff_queue_m_per_timestep);

    // TODO: look at whether the rest of this logic needs to be removed or moved, since we don't need to output to file here.

    /*
    double global_residual;
    double schaake_residual;
    double giuh_residual;
    double soil_residual;
    double nash_residual;
    double gw_residual;

    global_residual = volstart + volin - volout - volend;

    fprintf(stderr,"GLOBAL MASS BALANCE\n");
    fprintf(stderr,"  initial volume: %8.4lf m\n",volstart);
    fprintf(stderr,"    volume input: %8.4lf m\n",volin);
    fprintf(stderr,"   volume output: %8.4lf m\n",volout);
    fprintf(stderr,"    final volume: %8.4lf m\n",volend);
    fprintf(stderr,"        residual: %6.4e m\n",global_residual);
    if(volin>0.0) fprintf(stderr,"global pct. err: %6.4e percent of inputs\n",global_residual/volin*100.0);
    else          fprintf(stderr,"global pct. err: %6.4e percent of initial\n",global_residual/volstart*100.0);
    if(!is_fabs_less_than_epsilon(global_residual,1.0e-12))
        fprintf(stderr, "WARNING: GLOBAL MASS BALANCE CHECK FAILED\n");

    schaake_residual = volin - vol_sch_runoff - vol_sch_infilt;

    fprintf(stderr," SCHAAKE MASS BALANCE\n");
    fprintf(stderr,"  surface runoff: %8.4lf m\n",vol_sch_runoff);
    fprintf(stderr,"    infiltration: %8.4lf m\n",vol_sch_infilt);
    fprintf(stderr,"schaake residual: %6.4e m\n",schaake_residual);  // should equal 0.0
    if(!is_fabs_less_than_epsilon(schaake_residual,1.0e-12))
        fprintf(stderr,"WARNING: SCHAAKE PARTITIONING MASS BALANCE CHECK FAILED\n");

    giuh_residual = vol_out_giuh - vol_sch_runoff - vol_end_giuh;

    fprintf(stderr," GIUH MASS BALANCE\n");
    fprintf(stderr,"  vol. into giuh: %8.4lf m\n",vol_sch_runoff);
    fprintf(stderr,"   vol. out giuh: %8.4lf m\n",vol_out_giuh);
    fprintf(stderr," vol. end giuh q: %8.4lf m\n",vol_end_giuh);
    fprintf(stderr,"   giuh residual: %6.4e m\n",giuh_residual);  // should equal zero
    if(!is_fabs_less_than_epsilon(giuh_residual,1.0e-12))
        fprintf(stderr,"WARNING: GIUH MASS BALANCE CHECK FAILED\n");

    soil_residual = vol_soil_start + vol_sch_infilt - vol_soil_to_lat_flow - vol_soil_end - vol_to_gw;

    fprintf(stderr," SOIL WATER CONCEPTUAL RESERVOIR MASS BALANCE\n");
    fprintf(stderr,"   init soil vol: %8.4lf m\n",vol_soil_start);
    fprintf(stderr,"  vol. into soil: %8.4lf m\n",vol_sch_infilt);
    fprintf(stderr,"vol.soil2latflow: %8.4lf m\n",vol_soil_to_lat_flow);
    fprintf(stderr," vol. soil to gw: %8.4lf m\n",vol_soil_to_gw);
    fprintf(stderr," final vol. soil: %8.4lf m\n",vol_soil_end);
    fprintf(stderr,"vol. soil resid.: %6.4e m\n",soil_residual);
    if(!is_fabs_less_than_epsilon(soil_residual,1.0e-12))
        fprintf(stderr,"WARNING: SOIL CONCEPTUAL RESERVOIR MASS BALANCE CHECK FAILED\n");

    nash_residual = vol_in_nash - vol_out_nash - vol_in_nash_end;

    fprintf(stderr," NASH CASCADE CONCEPTUAL RESERVOIR MASS BALANCE\n");
    fprintf(stderr,"    vol. to nash: %8.4lf m\n",vol_in_nash);
    fprintf(stderr,"  vol. from nash: %8.4lf m\n",vol_out_nash);
    fprintf(stderr," final vol. nash: %8.4lf m\n",vol_in_nash_end);
    fprintf(stderr,"nash casc resid.: %6.4e m\n",nash_residual);
    if(!is_fabs_less_than_epsilon(nash_residual,1.0e-12))
        fprintf(stderr,"WARNING: NASH CASCADE CONCEPTUAL RESERVOIR MASS BALANCE CHECK FAILED\n");

    gw_residual = vol_in_gw_start + vol_to_gw - vol_from_gw - vol_in_gw_end;

    fprintf(stderr," GROUNDWATER CONCEPTUAL RESERVOIR MASS BALANCE\n");
    fprintf(stderr,"init gw. storage: %8.4lf m\n",vol_in_gw_start);
    fprintf(stderr,"       vol to gw: %8.4lf m\n",vol_to_gw);
    fprintf(stderr,"     vol from gw: %8.4lf m\n",vol_from_gw);
    fprintf(stderr,"final gw.storage: %8.4lf m\n",vol_in_gw_end);
    fprintf(stderr,"    gw. residual: %6.4e m\n",gw_residual);
    if(!is_fabs_less_than_epsilon(gw_residual,1.0e-12))
        fprintf(stderr,"WARNING: GROUNDWATER CONCEPTUAL RESERVOIR MASS BALANCE CHECK FAILED\n");

    */

    //fclose(out_fptr);

}
