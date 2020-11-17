#ifndef CFE_CFE_H
#define CFE_CFE_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>
#include <string.h>

#define TRUE 1
#define FALSE 0
#define DEBUG 1
#define MAX_NUM_GIUH_ORDINATES 10
#define MAX_NUM_NASH_CASCADE    3
#define MAX_NUM_RAIN_DATA 720

// t-shirt approximation of the hydrologic routing funtionality of the National Water Model v 1.2, 2.0, and 2.1
// This code was developed to test the hypothesis that the National Water Model runoff generation, vadose zone
// dynamics, and conceptual groundwater model can be greatly simplified by acknowledging that it is truly a
// conceptual model.  The hypothesis is supported by a number of observations made during a 2017-2018 deep dive
// into the NWM code.  Thesed are:
//
// 1. Rainfall/throughfall/melt partitioning in the NWM is based on a simple curve-number like approach that
//    was developed by Schaake et al. (1996) and which is very similar to the Probability Distributed Moisture (PDM)
//    function by Moore, 1985.   The Schaake function is a single valued function of soil moisture deficit,
//    predicts 100% runoff when the soil is saturated, like the curve-number method, and is fundamentally simple.
// 2. Run-on infiltration is strictly not calculated.  Overland flow routing applies the Schaake function repeatedly
//    to predict this phenomenon, which violates the underlying assumption of the PDM method that only rainfall
//    inputs affect soil moisture.
// 3. The water-content based Richards' equation, applied using a coarse-discretization, can be replaced with a simple
//    conceptual reservoir because it never allows saturation or infiltration-excess runoff unless deactivated by
//    assuming no-flow lower boundary condition.  Since this form of Richards' equation cannot simulate heterogeneous
//    soil layers, it can be replaced with a conceptual reservoir.
// 4. The lateral flow routing function in the NWM is purely conceptual.  It is activated whenever the soil water
//    content in one or more of the four Richards-equation discretizations reaches the wilting point water content.
//    This activation threshold is physically unrealistic, because in most soils lateral subsurface flow is not
//    active until pore water pressures become positive at some point in the soil profile.  Furthermore, the lateral
//    flow hydraulic conductivity is assumed to be the vertical hydraulic conductivity multiplied by a calibration
//    factor "LKSATFAC" which is allowed to vary between 10 and 10,000 during calibration, resulting in an anisotropy
//    ratio that varies over the same range, without correlation with physiographic characteristics or other support.
//
//    This code implements these assumptions using pure conceptualizations.  The formulation consists of the following:
//
//    1. Rainfall is partitioned into direct runoff and soil moisture using the Schaake function.
//    2. Rainfall that becomes direct runoff is routed to the catchment outlet using a geomorphological instantanteous
//       unit hydrograph (GIUH) approach, eliminating the 250 m NWM routing grid, and the incorrect use of the Schaake
//       function to simulate run-on infiltration.
//    3. Water partitioned by the Schaake function to be soil moisture is placed into a conceptual linear reservoir
//       that consists of two outlets that apply a minimum storage activation threshold.   This activation threshold
//       is identical for both outlets, and is based on an integral solution of the storage in the soil assuming
//       Clapp-Hornberger parameters equal to those used in the NWM to determine that storage corresponding to a
//       soil water content 0.5 m above the soil column bottom that produces a soil suction head equal to -1/3 atm,
//       which is a commonly applied assumption used to estimate the field capacity water content.
//       The first outlet calculates vertical percolation of water to deep groundwater using the saturated hydraulic
//       conductivity of the soil multiplied by the NWM "slope" parameter, which when 1.0 indicates free drainage and
//       when 0.0 indicates a no-flow lower boundary condition.   The second outlet is used to calculate the flux to
//       the soil lateral flow path, using a conceptual LKSATFAC-like calibration parameter.
//    4. The lateral flow is routed to the catchment outlet using a Nash-cascade of reservoirs to produce a mass-
//       conserving delayed response, and elminates the need for the 250 m lateral flow routing grid.
//    5. The groundwater contribution to base flow is modeled using either (a) an exponential nonlinear reservoir
//       identical to the one in the NWM formulation, or (b) a nonlinear reservoir forumulation, which can also be
//       made linear by assuming an exponent value equal to 1.0.
//
//    This code was written entirely by Fred L. Ogden, May 22-24, 2020, in the service of the NOAA-NWS Office of Water
//    Prediction, in Tuscaloosa, Alabama.

struct conceptual_reservoir
{
// this data structure describes a nonlinear reservoir having two outlets, one primary with an activation
// threshold that may be zero, and a secondary outlet with a threshold that may be zero
// this will also simulate a linear reservoir by setting the exponent parameter to 1.0 iff is_exponential==FALSE
// iff is_exponential==TRUE, then it uses the exponential discharge function from the NWM V2.0 forumulation
// as the primary discharge with a zero threshold, and does not calculate a secondary discharge.
//--------------------------------------------------------------------------------------------------
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
typedef struct conceptual_reservoir conceptual_reservoir;

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
typedef struct NWM_soil_parameters NWM_soil_parameters;

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
typedef struct aorc_forcing_data aorc_forcing_data;

typedef struct {
    double Schaake_output_runoff_m;
    double giuh_runoff_m;
    double nash_lateral_runoff_m;
    double flux_from_deep_gw_to_chan_m;
    // flux from soil to deeper groundwater reservoir
    double flux_perc_m;
    double flux_lat_m;
    double Qout_m;
} result_fluxes;

typedef struct {
    // ***********************************************************
    // *************** Non-dynamically allocations ***************
    // ***********************************************************
    struct conceptual_reservoir soil_reservoir;
    struct conceptual_reservoir gw_reservoir;
    struct NWM_soil_parameters NWM_soil_params;

    // Epoch-based start time
    double start_time;
    int num_timesteps;
    int current_time_step;

    double Schaake_adjusted_magic_constant_by_soil_type;

    int num_lateral_flow_nash_reservoirs;
    double K_nash;

    int num_giuh_ordinates;

    // ***********************************************************
    // ***************** Dynamically allocations *****************
    // ***********************************************************
    int* time_step_sizes;
    // In meters per time step
    double* rain_rates;
    double* surface_pressure;
    double* giuh_ordinates;
    result_fluxes* fluxes;
    double* nash_storage;
    double* runoff_queue_m_per_timestep;
} cfe_model;

extern void alloc_cfe_model(cfe_model *model);

extern void free_cfe_model(cfe_model *model);

extern int run(cfe_model* model);

extern void Schaake_partitioning_scheme(double dt, double magic_number, double deficit, double qinsur,
                                        double *runsrf, double *pddum);

extern void conceptual_reservoir_flux_calc(struct conceptual_reservoir *da_reservoir,
                                           double *primary_flux,double *secondary_flux);

extern double convolution_integral(double runoff_m, int num_giuh_ordinates,
                                   double *giuh_ordinates, double *runoff_queue_m_per_timestep);

extern double nash_cascade(double flux_lat_m,int num_lateral_flow_nash_reservoirs,
                           double K_nash,double *nash_storage);


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


#if defined(__cplusplus)
}
#endif

#endif //CFE_CFE_H
