#ifndef SCHAAKE_PARTITIONING_SCHEME_H
#define SCHAAKE_PARTITIONING_SCHEME_H

#include <math.h>


void schaake_partitioning_scheme(double timestep_s, double schaake_adjusted_magic_constant_by_soil_type,
                                 double column_total_soil_moisture_deficit_m, double water_input_depth_m,
                                 double *surface_runoff_depth_m, double *infiltration_depth_m)
{
  int k;
  double timestep_d,schaake_parenthetical_term,ic,px,infilt_dep_m;

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
        schaake_parenthetical_term = (1.0 - exp ( - schaake_adjusted_magic_constant_by_soil_type * timestep_d));
        // From Schaake et al. Eqn. 2., using the column total moisture deficit
        // BUT the way it is used here, it is the cumulative soil moisture deficit in the entire soil profile.
        // "Layer" info not used in this subroutine.
        // NOTE: when column_total_soil_moisture_deficit_m becomes zero, which occurs when the soil column is saturated,
        // then ic=0 Where ic in the Schaake paper is called the
        // "spatially averaged infiltration capacity", and is defined in Eqn. 12.
        ic = column_total_soil_moisture_deficit_m * schaake_parenthetical_term;

        // Total water input to partitioning scheme this time step [m]
        px=water_input_depth_m;

        // This is eqn 24 from Schaake et al.
        // NOTE: this is 0 in the case of a saturated soil column, when ic=0.
        // Physically happens only if soil has no-flow lower b.c.
        *infiltration_depth_m = (px * (ic / (px + ic)));


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
