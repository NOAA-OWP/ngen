#ifndef SCHAAKE_PARTITIONING_SCHEME_H
#define SCHAAKE_PARTITIONING_SCHEME_H

#include <math.h>


static void Schaake_partitioning_scheme_cpp(double timestep_s, double Schaake_adjusted_magic_constant_by_soil_type,
                                 double column_total_soil_moisture_deficit_m, double water_input_depth_m,
                                 double *surface_runoff_depth_m, double *infiltration_depth_m)
{
  int k;
  double timestep_d,Schaake_parenthetical_term,Ic,Px,infilt_dep_m;

  if(0.0 < water_input_depth_m) {
    if (0.0 > column_total_soil_moisture_deficit_m) {
        *surface_runoff_depth_m=water_input_depth_m;
        *infiltration_depth_m=0.0;
    }
    else {
        // partition time-step total applied water as per Schaake et al. 1996.

        // timestep_d is the time step in days.   They switch from dt in [s] to dt1 in [d] because kdt has units of [d^(-1)]
        timestep_d = timestep_s /86400.0;
        // calculate the parenthetical part of Eqn. 34 from Schaake et al. Note the magic constant has units of [d^(-1)]
        Schaake_parenthetical_term = (1.0 - exp ( - Schaake_adjusted_magic_constant_by_soil_type * timestep_d));
        // From Schaake et al. Eqn. 2., using the column total moisture deficit
        // BUT the way it is used here, it is the cumulative soil moisture deficit in the entire soil profile.
        // "Layer" info not used in this subroutine.
        // NOTE: when column_total_soil_moisture_deficit_m becomes zero, which occurs when the soil column is saturated,
        // then Ic=0 Where Ic in the Schaake paper is called the
        // "spatially averaged infiltration capacity", and is defined in Eqn. 12.
        Ic = column_total_soil_moisture_deficit_m * Schaake_parenthetical_term;

        // Total water input to partitioning scheme this time step [m]
        Px=water_input_depth_m;

        // This is eqn 24 from Schaake et al.
        // NOTE: this is 0 in the case of a saturated soil column, when Ic=0.
        // Physically happens only if soil has no-flow lower b.c.
        *infiltration_depth_m = (Px * (Ic / (Px + Ic)));


        if( 0.0 < (water_input_depth_m-(*infiltration_depth_m)) ){
          *surface_runoff_depth_m = water_input_depth_m - (*infiltration_depth_m);
        }
        else{
          *surface_runoff_depth_m=0.0;
        }
        *infiltration_depth_m = water_input_depth_m - (*surface_runoff_depth_m);
      } // end else branch of if (0.0 > column_total_soil_moisture_deficit_m)
  } // end if(0.0 < water_input_depth_m)
  else {
    *surface_runoff_depth_m = 0.0;
    *infiltration_depth_m = 0.0;
  }
  return;
}

#endif // SCHAAKE_PARTITIONING_SCHEME_H
