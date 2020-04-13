#include "utility.hpp"
#include "schaake_partitioning.hpp"
// int main(int argc, char argv[])
// int main(int argc, char** argv)
int main(int argc, char* argv[])
{
FILE *in_fptr;
FILE *out_fptr;

/* LOCAL VARIABLES AND TIME VARIABLES USED IN JULIAN DATE FUNCTIONS */
int k,i;
double jdate,dsec;
long year,month,day,hour,minute;

/* THE NEW, MORE MEANINGFUL VARIABLE NAME      -----     OLD VARIABLE NAME FROM FORTRAN */
/*______________________________________________________________________________________*/
int    num_soil_discretizations;                        // nsoil;
double timestep_s;                                      // dt;
double *soil_discretization_thickness_m;                // *dz;
double *soil_volumetric_moisture_content;               // *sh2o;
double surface_input_flux_m_per_s;                      // qinsur;
double water_input_depth_m;                             // partitioning is not rate based, so this depth is passed to Schaake function
double surface_runoff_depth_m;                          // depth of applied water partitioned to surface water this time step
double infiltration_depth_m;                            // depth of applied water partitioned to soil moisture this time step
double soil_max_water_content;                          // smcmax;
double soil_wilting_point_moisture_content;             // smcwlt;
double *soil_max_water_storage_per_discretization_m;    // dmax;
double soil_max_moisture_capacity;                      // smcmax;
double column_total_soil_moisture_deficit_m;            // passed to partitioning function because that is what it really wants.
double soil_max_avail_moisture_capacity;                // dimensionless soil water storage capacity given as (effective porosity-wilting point water content)
double soil_saturated_hydraulic_conductivity_m_per_s;   // dksat;  This is the value read in from the soil parameter table.
double schaake_magic_constant_equal_to_three;           // refkdt from GENPARM.dat this is _really_ just a constant equal to 3.0.
double schaake_reference_saturated_hydraulic_conductivity_m_per_s;   // refdk; the magical value of reference hydraulic conductivity identified as super cool by Schaake et al.  A CONSTANT = 2.0e-06 [m/s]
double schaake_adjusted_magic_constant_by_soil_type;    // kdt, this is the constant given by 3*(dksat/


if((out_fptr=fopen("inf_vs_rainrate.out","w"))==NULL)
  {printf("Can't open output file\n");exit(0);}

num_soil_discretizations=4;

//ALLOCATE MEMORY FOR THESE FOUR ARRAYS
d_alloc(&soil_discretization_thickness_m,num_soil_discretizations);
d_alloc(&soil_volumetric_moisture_content,num_soil_discretizations);
d_alloc(&soil_max_water_storage_per_discretization_m,num_soil_discretizations);

/* the thickness of the discretizations */
soil_discretization_thickness_m[0]=0.0;     // NOTE: it is strange that they used negative signs here, normally in soils physics,
soil_discretization_thickness_m[1]=0.10;   //       people create a coordinate system that is positive downward. -FLO
soil_discretization_thickness_m[2]=0.30;   //       The end result is that the math below is weird in calculating the thickness
soil_discretization_thickness_m[3]=0.60;   //       of the various discretizations, which are actually already known.
soil_discretization_thickness_m[4]=1.00;   //          I define these as POSITIVE real numbers.


soil_max_water_content = 0.463999987;
soil_wilting_point_moisture_content = 0.12;


/* the assumed initial soil moisture contents */
soil_volumetric_moisture_content[0] = 0.0;
soil_volumetric_moisture_content[1] = 0.285762608;
soil_volumetric_moisture_content[2] = 0.292563140;
soil_volumetric_moisture_content[3] = 0.272871971;
soil_volumetric_moisture_content[4] = 0.306821257;

// jdate=greg_2_jul(year,month,day,hour,minute,dsec);


// dt=1800.0; //seconds
timestep_s = 900.0;
/*! ----------------------------------------------------------------------*/
/* soil type dependent params.                                            */
/*! ----------------------------------------------------------------------*/
/* the uniform soil properties needed to calculate storage deficit in soil column */

soil_max_water_content = 0.464;                   // these would come from SOILPARM.TBL
soil_wilting_point_moisture_content = 0.120;

/* DKSAT  = SATDK (SOILTYP) */  // <- THIS IS AN EXCELLENT EXAMPLE OF OBFUSCATION!
soil_saturated_hydraulic_conductivity_m_per_s=5.23E-6;                   // was dksat.  In noah-mp, this value from SOILPARM.TBL for a particular soil type

/*! ------------------------------------------------------------------------------------------*/
/*! Set-up "universal" parameters <- a funny comment from the original noah-mp code...        */
/*! ------------------------------------------------------------------------------------------*/

schaake_reference_saturated_hydraulic_conductivity_m_per_s=2.0e-06;      // was refdk, the magical value of reference hydraulic conductivity identified as super cool by Schaake et al.
schaake_magic_constant_equal_to_three=3.0;                               // refkdt=3.0;     from GENPARM.TBL.  A constant equal to three.

// kdt    = refkdt * dksat / refdk;  <- this is the magic.

schaake_adjusted_magic_constant_by_soil_type = schaake_magic_constant_equal_to_three*(soil_saturated_hydraulic_conductivity_m_per_s/schaake_reference_saturated_hydraulic_conductivity_m_per_s); // was kdt


// forcing magnitude
surface_input_flux_m_per_s=25.4;                            // rate of water applied to soil surface [mm/h] (human readable)
surface_input_flux_m_per_s/=(double)3600000.0;              // rate of applied water converted into units of [m/s]
water_input_depth_m=surface_input_flux_m_per_s*timestep_s;  // this is the amount of applied water this time step.

// compute the average soil moisture deficit in all soil discretizations, and sum them up.
// NOTE: if used with a nonlinear reservoir, you wouldn't do this.  Rather it would just be (storage_max_m - current_storage_m)

soil_max_avail_moisture_capacity = soil_max_water_content-soil_wilting_point_moisture_content;  // this is the volumetric pore space between effective saturation and wilting point.  Dimensionless.
                                                                                                // NOTE: this is constant for a given soil type in noah-mp.  Doesn't need to be calculated each time in infilt()

soil_max_water_storage_per_discretization_m[1]= soil_discretization_thickness_m[1] * soil_max_avail_moisture_capacity;                        // max.depth of water storage [m] in upper most soil discretization
soil_max_water_storage_per_discretization_m[1]= soil_max_water_storage_per_discretization_m[1]* (1.0-(soil_volumetric_moisture_content[1] - soil_wilting_point_moisture_content)/soil_max_avail_moisture_capacity);  // Actual water storage deficit [m] in uppermost soil discretization
                                                   //     Note in previous line (soil_volumetric_moisture_content[1]-soil_wilting_point_moisture_content)/soil_max_avail_moisture_capacity is relative storage (0 to 1)

column_total_soil_moisture_deficit_m = soil_max_water_storage_per_discretization_m[1];   // Soil moisture deficit (m) sum, initialized to start the summation in the next loop.

for(k = 2;k<=num_soil_discretizations;k++)
  {
  soil_max_water_storage_per_discretization_m[k] = soil_discretization_thickness_m[k] * soil_max_avail_moisture_capacity;          // max. depth of moisture storage in kth discretization
  soil_max_water_storage_per_discretization_m[k] = soil_max_water_storage_per_discretization_m[k] * (1.0-(soil_volumetric_moisture_content[k] - soil_wilting_point_moisture_content)/soil_max_avail_moisture_capacity); // Actual water storage deficit [m] in kth discretization, dmax[k]*(1-(a ratio from 0 to 1))
  column_total_soil_moisture_deficit_m  += soil_max_water_storage_per_discretization_m[k];        // Sum of available water storage in entire profile [m].
  }

// this function call would be within a time loop to partition water applied to the soil surface [m] into runoff [m] and soil moisture [m]

  printf("column_total_soil_moisture_deficit_m= %lf\n", column_total_soil_moisture_deficit_m);

  schaake_partitioning_scheme(timestep_s,schaake_adjusted_magic_constant_by_soil_type,column_total_soil_moisture_deficit_m,
        water_input_depth_m,&surface_runoff_depth_m,&infiltration_depth_m);          // this is all that this function really needs.

fprintf(out_fptr,"input water this timestep: %e mm partitioned into:\n            surface water: %e mm\n               soil water: %e mm\n                 residual: %e mm\n",
         water_input_depth_m*1000.0,
         surface_runoff_depth_m*1000.0,
         infiltration_depth_m*1000.0,
         water_input_depth_m-surface_runoff_depth_m-infiltration_depth_m);

fclose(out_fptr);

return(0);
}
