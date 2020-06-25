#ifndef NGEN_RESERVOIR_EXPONENTIAL_OUTLET_H
#define NGEN_RESERVOIR_EXPONENTIAL_OUTLET_H

#include "Reservoir_Outlet.hpp"

/**
 * @brief Single Exponential Reservior Outlet Class
 * This class is for a single exponential reservoir outlet that is derived from the base Reservoir_Outlet class. It
 * derived class holds extra parameters for c and expon and overrides the calc_velocity_meters_per_second_local
 * function with an exponential equation.
 */
class Reservoir_Exponential_Outlet: public Reservoir_Outlet
{
    public:

    /**
     * @brief Default Constructor building an empty Reservoir Exponential Outlet Object.
     */
    Reservoir_Exponential_Outlet(): Reservoir_Outlet(), c(0.0), expon(0.0)
    {

    }

    /**
     * @brief Parameterized Constructor that builds a Reservoir Outlet object.
     * @param c outlet velocity calculation coefficient
     * @param expon outlet velocity calculation exponential coefficient
     * @param activation_threshold_meters meters from the bottom of the reservoir to the bottom of the outlet
     * @param max_velocity_meters_per_second max outlet velocity in meters per second
     */
    Reservoir_Exponential_Outlet(double c, double expon, double activation_threshold_meters,
                                 double max_velocity_meters_per_second) :
            Reservoir_Outlet(-999.0, -999.0, activation_threshold_meters, max_velocity_meters_per_second),
            c(c),
            expon(expon)
    {

    }

protected:

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
     * @param max_velocity_meters_per_second max outlet velocity in meters per second
     * @return velocity_meters_per_second_local the velocity in meters per second of the discharge through the outlet
     */
    double calc_velocity_meters_per_second_local(reservoir_parameters &parameters_struct,
                                                 reservoir_state &storage_struct) override
    {
        return c * (exp(expon * storage_struct.current_storage_height_meters /
                        parameters_struct.maximum_storage_meters) - 1);
    }

    private:
    double c;
    double expon;
};

#endif //NGEN_RESERVOIR_EXPONENTIAL_OUTLET_H
