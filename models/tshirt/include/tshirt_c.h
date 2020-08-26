#ifndef NGEN_TSHIRT_C_HPP
#define NGEN_TSHIRT_C_HPP

#define TRUE 1
#define FALSE 0
#define DEBUG 1
#define MAX_NUM_GIUH_ORDINATES 10
#define MAX_NUM_NASH_CASCADE    3
#define TSHIRT_C_FIXED_TIMESTEP_SIZE_S 3600
#define MAX_NUM_RAIN_DATA 720

// define data structures
//--------------------------

struct conceptual_reservoir
{
// this data structure describes a nonlinear reservoir having two outlets, one primary with an activation
// threshold that may be zero, and a secondary outlet with a threshold that may be zero
// this will also simulate a linear reservoir by setting the exponent parameter to 1.0 iff is_exponential==FALSE
// iff is_exponential==TRUE, then it uses the exponential discharge function from the NWM V2.0 forumulation
// as the primary discharge with a zero threshold, and does not calculate a secondary discharge.
//--------------------------------------------------------------------------------------------------
    // TODO: adjust this (and usages) to bool type once verfied and automated testing is in place.
    int    is_exponential;  // set this true TRUE to use the exponential form of the discharge equation
    double storage_max_m;   // maximum storage in this reservoir
    double storage_m;       // state variable.
    double coeff_primary;    // the primary outlet
    double exponent_primary;
    double storage_threshold_primary_m;
    double storage_threshold_secondary_m;
    double coeff_secondary;
    double exponent_secondary;
};

struct NWM_soil_parameters
{
// using same variable names as used in NWM.  <sorry>
    double smcmax;  // effective porosity [V/V]
    double wltsmc;  // wilting point soil moisture content [V/V]
    double satdk;   // saturated hydraulic conductivity [m s-1]
    double satpsi;	// saturated capillary head [m]
    double bb;      // beta exponent on Clapp-Hornberger (1978) soil water relations [-]
    double mult;    // the multiplier applied to satdk to route water rapidly downslope
    double slop;   // this factor (0-1) modifies the gradient of the hydraulic head at the soil bottom.  0=no-flow.
    double D;       // soil depth [m]
};

//DATA STRUCTURE TO HOLD AORC FORCING DATA
struct aorc_forcing_data
{
// struct NAME                          DESCRIPTION                                            ORIGINAL AORC NAME
//____________________________________________________________________________________________________________________
    float precip_kg_per_m2;                // Surface precipitation "kg/m^2"                         | APCP_surface
    float incoming_longwave_W_per_m2 ;     // Downward Long-Wave Rad. Flux at 0m height, W/m^2       | DLWRF_surface
    float incoming_shortwave_W_per_m2;     // Downward Short-Wave Radiation Flux at 0m height, W/m^2 | DSWRF_surface
    float surface_pressure_Pa;             // Surface atmospheric pressure, Pa                       | PRES_surface
    float specific_humidity_2m_kg_per_kg;  // Specific Humidity at 2m height, kg/kg                  | SPFH_2maboveground
    float air_temperature_2m_K;            // Air temparture at 2m height, K                         | TMP_2maboveground
    float u_wind_speed_10m_m_per_s;        // U-component of Wind at 10m height, m/s                 | UGRD_10maboveground
    float v_wind_speed_10m_m_per_s;        // V-component of Wind at 10m height, m/s                 | VGRD_10maboveground
    float latitude;                        // degrees north of the equator.  Negative south          | latitude
    float longitude;                       // degrees east of prime meridian. Negative west          | longitude
    long int time; //TODO: type?           // seconds since 1970-01-01 00:00:00.0 0:00               | time
} ;

struct tshirt_c_result_fluxes {
    double timestep_rainfall_input_m;
    double Schaake_output_runoff_m;
    double giuh_runoff_m;
    double nash_lateral_runoff_m;
    double flux_from_deep_gw_to_chan_m;
    double Qout_m;
};

// TODO: add converter helper functions for going between aorc_forcing_data and AORC_data from Forcing.h

// function prototypes
// --------------------------------
extern void Schaake_partitioning_scheme(double dt, double magic_number, double deficit, double qinsur,
                                        double *runsrf, double *pddum);

extern void conceptual_reservoir_flux_calc(struct conceptual_reservoir *da_reservoir,
                                           double *primary_flux,double *secondary_flux);

extern double convolution_integral(double runoff_m, int num_giuh_ordinates,
                                   double *giuh_ordinates, double *runoff_queue_m_per_timestep);

extern double nash_cascade(double flux_lat_m, int num_lateral_flow_nash_reservoirs,
                           double K_nash, double *nash_storage);

extern int is_fabs_less_than_epsilon(double a,double epsilon);  // returns TRUE iff fabs(a)<epsilon

extern double greg_2_jul(long year, long mon, long day, long h, long mi,
                         double se);
extern void calc_date(double jd, long *y, long *m, long *d, long *h, long *mi,
                      double *sec);

extern void itwo_alloc( int ***ptr, int x, int y);
extern void dtwo_alloc( double ***ptr, int x, int y);
extern void d_alloc(double **var,int size);
extern void i_alloc(int **var,int size);

extern void parse_aorc_line(char *theString,long *year,long *month, long *day,long *hour,
                            long *minute, double *dsec, struct aorc_forcing_data *aorc);

extern void get_word(char *theString,int *start,int *end,char *theWord,int *wordlen);

/**
 * Run the model, recording fluxes in one or more flux structs at a specified location.
 *
 * @param NWM_soil_params Struct containing soil model params
 * @param gw_reservoir Struct containing state for ground water reservoir
 * @param soil_reservoir Struct containing state for soil reservoir
 * @param num_timesteps The number of time steps, which PROBABLY should always be 1
 * @param giuh_ordinates A pointer to the head of an array of GIUH ordinate values
 * @param num_giuh_ordinates The number of elements in the array pointed to by ``giuh_ordinates``
 * @param water_table_slope Assumed near channel water table slope lateral flow parameter
 * @param Schaake_adjusted_magic_constant_by_soil_type Schaake magic constant for soil type
 * @param lateral_flow_linear_reservoir_constant Later flow reservoir constant (Klf)
 * @param K_nash Nash reservoir flow constant
 * @param num_lateral_flow_nash_reservoirs The number of reservoirs in the cascade for the lateral flow
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
               double water_table_slope,
               double Schaake_adjusted_magic_constant_by_soil_type,
               double lateral_flow_linear_reservoir_constant,
               double K_nash,
               int num_lateral_flow_nash_reservoirs,
               int yes_aorc,
               aorc_forcing_data* aorc_data,
               double* rain_rate,
               int& num_added_fluxes,
               tshirt_c_result_fluxes* fluxes);

#endif