#ifndef NGEN_RESERVOIR_OUTLET_H
#define NGEN_RESERVOIR_OUTLET_H

#include <cmath>
#include "reservoir_parameters_timeless.h"
#include "reservoir_state_timeless.h"

/**
 * @brief Base Single Reservior Outlet Class
 *
 * This class is for a single reservior outlet that holds the parameters a, b, and the activation threshold, which is
 * the height in meters the bottom of the outlet is above the bottom of the reservoir. This class also contains a
 * function to return the flux in meters of the discharge through the outlet. This function will only
 * run if the reservoir storage passed in is above the activation threshold.
 *
 * To modify the details of the calculations for discharge flux, subclasses should typically override the
 * calc_flux_meters_local function, rather than the flux_meters function, which works at a
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
     * @param a outlet flux calculation coefficient
     * @param b outlet flux calculation exponent
     * @param activation_threshold_meters meters from the bottom of the reservoir to the bottom of the outlet
     * @param max_flux_meters max outlet flux in meters
     */
    Reservoir_Outlet(double a, double b, double activation_threshold_meters, double max_flux_meters);

    /**
     * Set the locally maintained flux state variable value.
     *
     * @param flux_meters The flux value to set, in meters.
     */
    void adjust_flux(double flux_meters);

    /**
     * @brief Accessor to return activation_threshold_meters
     * @return activation_threshold_meters meters from the bottom of the reservoir to the bottom of the outlet
     */
    double get_activation_threshold_meters() const;

    /**
     * @brief Accessor to return flux_meters_local that is previously calculated
     * @return flux_meters_local
     */
    double get_previously_calculated_flux_meters();

    /**
     * @brief Function to update and return the flux in meters of the discharge through the outlet.
     *
     * Function determines, sets, and returns the value for discharge flux of the object.  The value itself is
     * maintained within the flux_meters_local member.
     *
     * Function performs sanity checking regarding whether storage is above the activation threshold before calculating
     * the updated flux value with a call to calc_flux_meters_local.  Additionally, it checks the
     * value returned by calc_flux_meters_local, ensuring that flux_meters_local does not
     * exceed the max allowed discharged flux for the outlet set in max_flux_meters member.
     *
     * @param parameters_struct reservoir parameters struct
     * @param storage_struct reservoir state storage struct
     * @see calc_flux_meters_local
     */
    double flux_meters(reservoir_parameters &parameters_struct, reservoir_state &storage_struct);

protected:
    double a;
    double b;
    double activation_threshold_meters;
    double max_flux_meters;
    double flux_meters_local;

    /**
     * @brief Calculate outlet discharge flux in meters.
     *
     * Perform appropriate calculations to return the flux, in meters, of the discharge through this
     * outlet.
     *
     * Typically this is only used by the flux_meters function.
     *
     * This function should be overridden to adjust the behavior of subtypes with respect to how the discharge flux
     * is calculated, assuming the reservoir storage is above the activation threshold.  However, it is left to the
     * flux_meters function to actually update the object's state.
     *
     * @param parameters_struct reservoir parameters struct
     * @param storage_struct reservoir state storage struct
     * @param activation_threshold_meters meters from the bottom of the reservoir to the bottom of the outlet

     * @return flux_meters_local the flux in meters of the discharge through the outlet
     */
    virtual double calc_flux_meters_local(reservoir_parameters &parameters_struct,
                                                         reservoir_state &storage_struct);

};

#endif //NGEN_RESERVOIR_OUTLET_H
