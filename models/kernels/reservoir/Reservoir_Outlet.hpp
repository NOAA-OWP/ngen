#ifndef NGEN_RESERVOIR_OUTLET_H
#define NGEN_RESERVOIR_OUTLET_H

#include <cmath>
#include "reservoir_parameters.h"
#include "reservoir_state.h"

/**
 * @brief Base Single Reservior Outlet Class
 *
 * This class is for a single reservior outlet that holds the parameters a, b, and the activation threshold, which is
 * the height in meters the bottom of the outlet is above the bottom of the reservoir. This class also contains a
 * function to return the velocity in meters per second of the discharge through the outlet. This function will only
 * run if the reservoir storage passed in is above the activation threshold.
 *
 * To modify the details of the calculations for discharge velocity, subclasses should typically override the
 * calc_velocity_meters_per_second_local function, rather than the velocity_meters_per_second function, which works at a
 * higher-level.
 */
class Reservoir_Outlet
{
public:

    /**
     * @brief Default Constructor building an empty Reservoir Outlet Object
     */
    Reservoir_Outlet();

    /**
     * @brief Parameterized Constuctor that builds a Reservoir Outlet object
     * @param a outlet velocity calculation coefficient
     * @param b outlet velocity calculation exponent
     * @param activation_threshold_meters meters from the bottom of the reservoir to the bottom of the outlet
     * @param max_velocity_meters_per_second max outlet velocity in meters per second
     */
    Reservoir_Outlet(double a, double b, double activation_threshold_meters, double max_velocity_meters_per_second);

    /**
     * Set the locally maintained velocity state variable value.
     *
     * @param velocity_meters_per_second The velocity value to set, in meters per second.
     */
    void adjust_velocity(double velocity_meters_per_second);

    /**
     * @brief Accessor to return activation_threshold_meters
     * @return activation_threshold_meters meters from the bottom of the reservoir to the bottom of the outlet
     */
    double get_activation_threshold_meters() const;

    /**
     * @brief Accessor to return velocity_meters_per_second_local that is previously calculated
     * @return velocity_meters_per_second_local
     */
    double get_previously_calculated_velocity_meters_per_second();

    /**
     * @brief Function to update and return the velocity in meters per second of the discharge through the outlet.
     *
     * Function determines, sets, and returns the value for discharge velocity of the object.  The value itself is
     * maintained within the velocity_meters_per_second_local member.
     *
     * Function performs sanity checking regarding whether storage is above the activation threshold before calculating
     * the updated velocity value with a call to calc_velocity_meters_per_second_local.  Additionally, it checks the
     * value returned by calc_velocity_meters_per_second_local, ensuring that velocity_meters_per_second_local does not
     * exceed the max allowed discharged velocity for the outlet set in max_velocity_meters_per_second member.
     *
     * @param parameters_struct reservoir parameters struct
     * @param storage_struct reservoir state storage struct
     * @see calc_velocity_meters_per_second_local
     */
    double velocity_meters_per_second(reservoir_parameters &parameters_struct, reservoir_state &storage_struct);

protected:
    double a;
    double b;
    double activation_threshold_meters;
    double max_velocity_meters_per_second;
    double velocity_meters_per_second_local;

    /**
     * @brief Calculate outlet discharge velocity in meters per second.
     *
     * Perform appropriate calculations to return the velocity, in meters per second, of the discharge through this
     * outlet.
     *
     * Typically this is only used by the velocity_meters_per_second function.
     *
     * This function should be overridden to adjust the behavior of subtypes with respect to how the discharge velocity
     * is calculated, assuming the reservoir storage is above the activation threshold.  However, it is left to the
     * velocity_meters_per_second function to actually update the object's state.
     *
     * @param parameters_struct reservoir parameters struct
     * @param storage_struct reservoir state storage struct
     * @param activation_threshold_meters meters from the bottom of the reservoir to the bottom of the outlet

     * @return velocity_meters_per_second_local the velocity in meters per second of the discharge through the outlet
     */
    virtual double calc_velocity_meters_per_second_local(reservoir_parameters &parameters_struct,
                                                         reservoir_state &storage_struct);

};

#endif //NGEN_RESERVOIR_OUTLET_H
