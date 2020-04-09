#ifndef SCHAAKE_PARTITIONING_SCHEME_H
#define SCHAAKE_PARTITIONING_SCHEME_H

#include <math.h>

/*! ===============================================================================
  This subtroutine takes surface_input_flux_m_per_s and partitions it into surface_runoff_flux_m_per_sec and
  infiltration_rate_m_per_sec using the scheme from Schaake et al. 1996.
! --------------------------------------------------------------------------------
! ! modified by FLO April 2020 to eliminate reference to ice processes,
! ! and to use descriptive and dimensionally consistent variable names.
! --------------------------------------------------------------------------------
    IMPLICIT NONE
! --------------------------------------------------------------------------------
! inputs
  double timestep_s
  double Schaake_adjusted_magic_constant_by_soil_type
  double column_total_soil_moisture_deficit_m
  double surface_input_flux_m_per_s

! outputs
  double surface_runoff_flux_m_per_s
  double infiltration_rate_m_per_s

--------------------------------------------------------------------------------*/
void Schaake_partitioning_scheme(double timestep_s, double Schaake_adjusted_magic_constant_by_soil_type,
                                 double column_total_soil_moisture_deficit_m, double surface_input_flux_m_per_s,
                                 double *surface_runoff_flux_m_per_s,double *infiltration_rate_m_per_s)
{
  int k;
  double timestep_d,Schaake_parenthetical_term,Ic,Px,infmax;


  if (surface_input_flux_m_per_s >  0.0) {
    // maximum infiltration rate calculations as per Schaake et al. 1996.

    // timestep_d is the time step in days.
    // They switch from dt in [s] to dt1 in [d] because kdt has units of [d^(-1)]
    timestep_d = timestep_s /86400.0;

    // calculate the parenthetical part of Eqn. 34 from Schaake et al.
    // Note the magic constant has units of [d^(-1)]
    Schaake_parenthetical_term = (1.0 - exp ( - Schaake_adjusted_magic_constant_by_soil_type * timestep_d));

    // From Schaake et al. Eqn. 2., using the column total moisture deficit
    // BUT the way it is used here, it is the cumulative soil moisture deficit in the entire soil profile.
    // "Layer" info not used in this subroutine.
    // NOTE: when column_total_soil_moisture_deficit_m becomes zero, which occurs when the soil column is saturated,
    // then Ic=0 Where Ic in the Schaake paper is called the
    // "spatially averaged infiltration capacity", and is defined in Eqn. 12.
    Ic = column_total_soil_moisture_deficit_m * Schaake_parenthetical_term;

    // Total water input to soil surface this time step [m]
    Px=surface_input_flux_m_per_s * timestep_s;

    // NOTE: infmax=0 in the case of a saturated soil column, when Ic=0.
    // Physically happens only if soil has no-flow lower b.c.
    infmax = (Px * (Ic / (Px + Ic)))/ timestep_s ;

    if( (surface_input_flux_m_per_s-infmax) > 0.0) {
      *surface_runoff_flux_m_per_s= surface_input_flux_m_per_s - infmax;
    }
    else {
       *surface_runoff_flux_m_per_s=0.0;
    }
    *infiltration_rate_m_per_s = surface_input_flux_m_per_s - (*surface_runoff_flux_m_per_s);
  } // end if (surface_input_flux_m_per_s >  0.0)
  else {
    *surface_runoff_flux_m_per_s = 0.0;
    *infiltration_rate_m_per_s = 0.0;
  }
  return;
}

#endif // SCHAAKE_PARTITIONING_SCHEME_H
