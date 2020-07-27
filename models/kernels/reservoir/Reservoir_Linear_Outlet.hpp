#ifndef NGEN_RESERVOIR_LINEAR_OUTLET_H
#define NGEN_RESERVOIR_LINEAR_OUTLET_H

#include "Reservoir_Outlet.hpp"

/**
 * @brief Single Linear Reservior Outlet Class
 * This class is for a single linear reservoir outlet that is derived from the base Reservoir_Outlet class.
 */
class Reservoir_Linear_Outlet: public Reservoir_Outlet
{
    public:

    /**
     * @brief Default Constructor building a Linear Reservoir Outlet Object
     */
    Reservoir_Linear_Outlet();


    /**
     * @brief Parameterized Constructor that builds a Linear Reservoir Outlet object.
     * @param a outlet velocity calculation coefficient
     * @param activation_threshold_meters meters from the bottom of the reservoir to the bottom of the outlet
     * @param max_velocity_meters_per_second max outlet velocity in meters per second
     */
    Reservoir_Linear_Outlet(double a, double activation_threshold_meters,
                                 double max_velocity_meters_per_second);


protected:

    /**
     * @brief Calculate outlet discharge velocity in meters per second.
     *
     * Perform appropriate calculations to return the velocity, in meters per second, of the discharge through this
     * linear outlet.
     *
     * Typically this is only used by the velocity_meters_per_second function.
     *
     * This function overrides the base nonlinear outlet and adjusts the behavior of how the discharge velocity
     * is calculated, assuming the reservoir storage is above the activation threshold.  However, it is left to the
     * velocity_meters_per_second function to actually update the object's state.
     *
     * @param parameters_struct reservoir parameters struct
     * @param storage_struct reservoir state storage struct
     */
     double calc_velocity_meters_per_second_local(reservoir_parameters &parameters_struct,
                                                 reservoir_state &storage_struct) override;

};

#endif //NGEN_RESERVOIR_LINEAR_OUTLET_H
