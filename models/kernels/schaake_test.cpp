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
double *depth_to_bottom_of_soil_discretization;         // *zsoil;
double surface_input_flux_m_per_s;                      // qinsur;
double surface_runoff_flux_m_per_s;                     // runsrf;
double infiltration_rate_m_per_s;                       // pddum;
double soil_effective_porosity;                         // smcmax;
double soil_wilting_point_moisture_content;             // smcwlt;
double *soil_max_water_storage_per_discretization_m;    // dmax;
double soil_max_moisture_capacity;                      // smcmax;
double column_total_soil_moisture_deficit_m;            // NEW variable by FLO to pass to infil() because that is what it really wants.
double soil_max_avail_moisture_capacity;                // dimensionless soil water storage capacity given as (effective porosity-wilting point water content)
double soil_saturated_hydraulic_conductivity_m_per_s;   // dksat;  This is the value read in from the soil parameter table.
double Schaake_magic_constant_equal_to_three;           // refkdt from GENPARM.dat this is _really_ just a constant equal to 3.0.
double Schaake_reference_saturated_hydraulic_conductivity_m_per_s;   // refdk; the magical value of reference hydraulic conductivity identified as super cool by Schaake et al.  A CONSTANT = 2.0e-06 [m/s]
double Schaake_adjusted_magic_constant_by_soil_type;    // kdt, this is the constant given by 3*(dksat/


if((out_fptr=fopen("inf_vs_rainrate.out","w"))==NULL)
  {printf("Can't open output file\n");exit(0);}

num_soil_discretizations=4;

//ALLOCATE MEMORY FOR THESE FOUR ARRAYS
d_alloc(&soil_discretization_thickness_m,num_soil_discretizations);
d_alloc(&depth_to_bottom_of_soil_discretization,num_soil_discretizations);
d_alloc(&soil_volumetric_moisture_content,num_soil_discretizations);
d_alloc(&soil_max_water_storage_per_discretization_m,num_soil_discretizations);

soil_discretization_thickness_m[0]=0.0;     // NOTE: it is strange that they use negative signs here, normally in soils physics,
soil_discretization_thickness_m[1]=-0.10;   //       people create a coordinate system that is positive downward. -FLO
soil_discretization_thickness_m[2]=-0.30;   //       The end result is that the math below is weird in calculating the thickness
soil_discretization_thickness_m[3]=-0.60;   //       of the various discretizations, which are actually already known.
soil_discretization_thickness_m[4]=-1.00;

soil_effective_porosity = 0.463999987;
soil_wilting_point_moisture_content = 0.12;

depth_to_bottom_of_soil_discretization[0]=0.0;
for(k=1;k<=num_soil_discretizations;k++)
  {
  depth_to_bottom_of_soil_discretization[k]=depth_to_bottom_of_soil_discretization[k-1]+soil_discretization_thickness_m[k];
  }

soil_volumetric_moisture_content[0] = 0.0;
soil_volumetric_moisture_content[1] = 0.285762608;
soil_volumetric_moisture_content[2] = 0.292563140;
soil_volumetric_moisture_content[3] = 0.272871971;
soil_volumetric_moisture_content[4] = 0.306821257;

// jdate=greg_2_jul(year,month,day,hour,minute,dsec);

/*! ----------------------------------------------------------------------*/
/*! Set-up universal parameters (not dependent on SOILTYP, VEGTYP)        */
/*! ----------------------------------------------------------------------*/
/* DKSAT  = SATDK (SOILTYP) */
soil_saturated_hydraulic_conductivity_m_per_s=5.23E-6;                   // was dksat.  In noah-mp, this value from SOILPARM.TBL for a particular soil type

Schaake_reference_saturated_hydraulic_conductivity_m_per_s=2.0e-06;      // was refdk, the magical value of reference hydraulic conductivity identified as super cool by Schaake et al.
Schaake_magic_constant_equal_to_three=3.0;                               // refkdt=3.0;     from GENPARM.TBL.  A constant equal to three.

// kdt    = refkdt * dksat / refdk;  <- this is the magic.

Schaake_adjusted_magic_constant_by_soil_type = Schaake_magic_constant_equal_to_three*(soil_saturated_hydraulic_conductivity_m_per_s/Schaake_reference_saturated_hydraulic_conductivity_m_per_s); // was kdt

// dt=1800.0; //seconds
timestep_s = 900.0;
/*! ----------------------------------------------------------------------*/
/* soil type dependent params.                                            */
/*! ----------------------------------------------------------------------*/
// soil_effective_porosity=0.40;
soil_effective_porosity = 0.464;
// soil_wilting_point_moisture_content=0.26;
soil_wilting_point_moisture_content = 0.120;


surface_input_flux_m_per_s=25.4;  // rate of water applied to soil surface [mm/h]
surface_input_flux_m_per_s/=(double)3600000.0;  // rate of applied water in units of [m/s] */

// compute the average soil moisture deficit in all soil discretizations, and sum them up.
// NOTE: if used with a nonlinear reservoir, you wouldn't do this.  Rather it would just be (storage_max_m - current_storage_m)

soil_max_avail_moisture_capacity = soil_effective_porosity-soil_wilting_point_moisture_content;  // this is the volumetric pore space between effective saturation and wilting point.  Dimensionless.
                                                                                              // NOTE: this is constant for a given soil type in noah-mp.  Doesn't need to be calculated each time in infilt()

soil_max_water_storage_per_discretization_m[1]= -depth_to_bottom_of_soil_discretization[1] * soil_max_avail_moisture_capacity;                        // max.depth of water storage [m] in upper most soil discretization
soil_max_water_storage_per_discretization_m[1]= soil_max_water_storage_per_discretization_m[1]* (1.0-(soil_volumetric_moisture_content[1] - soil_wilting_point_moisture_content)/soil_max_avail_moisture_capacity);  // Actual water storage deficit [m] in uppermost soil discretization
                                                   //     Note in previous line (soil_volumetric_moisture_content[1]-soil_wilting_point_moisture_content)/soil_max_avail_moisture_capacity is relative storage (0 to 1)

column_total_soil_moisture_deficit_m = soil_max_water_storage_per_discretization_m[1];   // Soil moisture deficit (m) sum, initialized to start the summation in the next loop.

for(k = 2;k<=num_soil_discretizations;k++)
  {
  soil_max_water_storage_per_discretization_m[k] = (depth_to_bottom_of_soil_discretization[k-1] - depth_to_bottom_of_soil_discretization[k]) * soil_max_avail_moisture_capacity;          // max. depth of moisture storage in kth discretization
  soil_max_water_storage_per_discretization_m[k] = soil_max_water_storage_per_discretization_m[k] * (1.0-(soil_volumetric_moisture_content[k] - soil_wilting_point_moisture_content)/soil_max_avail_moisture_capacity); // Actual water storage deficit [m] in kth discretization, dmax[k]*(1-(a ratio from 0 to 1))
  column_total_soil_moisture_deficit_m  += soil_max_water_storage_per_discretization_m[k];        // Sum of available water storage in entire profile [m].
  }

// this function call would be within a time loop to partition water applied to the soil surface [m] into runoff [m] and soil moisture [m]

  printf("column_total_soil_moisture_deficit_m= %lf\n", column_total_soil_moisture_deficit_m);

  Schaake_partitioning_scheme(timestep_s,Schaake_adjusted_magic_constant_by_soil_type,column_total_soil_moisture_deficit_m,
        surface_input_flux_m_per_s,&surface_runoff_flux_m_per_s,&infiltration_rate_m_per_s);          // this is all that this function really needs.

fprintf(out_fptr,"%e %e %e\n",surface_input_flux_m_per_s,surface_runoff_flux_m_per_s,infiltration_rate_m_per_s);

fclose(out_fptr);

return(0);
}
