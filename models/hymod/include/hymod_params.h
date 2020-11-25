#ifndef NGEN_HYMOD_PARAMS_H
#define NGEN_HYMOD_PARAMS_H

//! Hymod paramaters struct
/*!
    This structure provides storage for the parameters of the hymod hydrological model
*/
struct hymod_params
{
    double min_storage_meters; //!< minimum amount of water stored
    double gw_max_storage_meters; //!< maximum amount of water stored in groundwater
    double nash_max_storage_meters; //!< maximum amount of water stored in each nash cascade reservoir
    double activation_threshold_meters_nash_cascade_reservoir; //!< meters from the bottom of the reservoir to the bottom of the outlet
    double activation_threshold_meters_groundwater_reservoir; //!< meters from the bottom of the reservoir to the bottom of the outlet
    double reservoir_max_velocity_meters_per_second; //!<max outlet velocity in meters per second
    double a;               //!< coefficent for distributing runoff and slowflow
    double b;               //!< exponent for flux equation
    //Ks and Kq are coeeficint constants used by the non-linear reservoirs.  There is an implicit unit of time
    //in these parameters to make `a*(dS/S)^b` have approriate units of meters/second
    //this implies that for any given timstep, dt, the approriate coefficients may be different.
    double Ks;              //!< slow flow coefficent
    double Kq;              //!< quick flow coefficent
    int n;               //!< number of nash cascades
};

#endif //NGEN_HYMOD_PARAMS_H
