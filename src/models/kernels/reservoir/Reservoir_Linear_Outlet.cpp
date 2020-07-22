#include "Reservoir_Linear_Outlet.hpp"

/**
 * @brief Default Constructor building an empty Reservoir Linear Outlet Object.
 */
Reservoir_Linear_Outlet::Reservoir_Linear_Outlet() : Reservoir_Outlet()
{

}

/**
 * @brief Parameterized Constructor that builds a Linear Reservoir Outlet object.
 * @param a outlet velocity calculation coefficient
 * @param activation_threshold_meters meters from the bottom of the reservoir to the bottom of the outlet
 * @param max_velocity_meters_per_second max outlet velocity in meters per second
 */
Reservoir_Linear_Outlet::Reservoir_Linear_Outlet(double a, double activation_threshold_meters,
                             double max_velocity_meters_per_second) :
        Reservoir_Outlet(a, 1.0, activation_threshold_meters, max_velocity_meters_per_second)
{

}

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
double Reservoir_Linear_Outlet::calc_velocity_meters_per_second_local(reservoir_parameters &parameters_struct,
                                                                           reservoir_state &storage_struct)
{
    return a * (storage_struct.current_storage_height_meters - activation_threshold_meters) 
           /
           (parameters_struct.maximum_storage_meters - activation_threshold_meters);
}
