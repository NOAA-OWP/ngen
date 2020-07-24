#ifndef NGEN_RESERVOIR_EXPONENTIAL_OUTLET_H
#define NGEN_RESERVOIR_EXPONENTIAL_OUTLET_H

#include "Reservoir_Outlet_Timeless.hpp"

/**
 * @brief Single Exponential Reservoir Outlet Class
 * This class is for a single exponential reservoir outlet that is derived from the base Reservoir_Outlet class. It
 * derived class holds extra parameters for c and expon and overrides the calc_flux_meters_local
 * function with an exponential equation.
 */
class Reservoir_Exponential_Outlet: public Reservoir_Outlet
{
    public:

    /**
     * @brief Default Constructor building an empty Reservoir Exponential Outlet Object.
     */
    Reservoir_Exponential_Outlet();

    /**
     * @brief Parameterized Constructor that builds a Reservoir Exponential Outlet object.
     * @param c outlet flux calculation coefficient
     * @param expon outlet flux calculation exponential coefficient
     * @param activation_threshold_meters meters from the bottom of the reservoir to the bottom of the outlet
     * @param max_flux_meters max outlet flux in meters
     */
    Reservoir_Exponential_Outlet(double c, double expon, double activation_threshold_meters,
                                 double max_flux_meters);

protected:

    /**
     * @brief Calculate outlet discharge flux in meters.
     *
     * Perform appropriate calculations to return the flux, in meters, of the discharge through this
     * exponential outlet.
     *
     * Typically this is only used by the flux_meters function.
     *
     * This function overrides the base nonlinear outlet and adjusts the behavior of how the discharge flux
     * is calculated, assuming the reservoir storage is above the activation threshold.  However, it is left to the
     * flux_meters function to actually update the object's state.
     *
     * @param parameters_struct reservoir parameters struct
     * @param storage_struct reservoir state storage struct
     */
    double calc_flux_meters_local(reservoir_parameters &parameters_struct,
                                                 reservoir_state &storage_struct) override;

    private:
    double c;
    double expon;
};

#endif //NGEN_RESERVOIR_EXPONENTIAL_OUTLET_H
