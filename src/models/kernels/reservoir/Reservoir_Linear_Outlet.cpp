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
