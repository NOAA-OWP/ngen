#include "Reservoir_Exponential_Outlet_Timeless.hpp"

namespace Reservoir{
    namespace Implicit_Time{

        /**
         * @brief Default Constructor building an empty Reservoir Exponential Outlet Object.
         */
        Reservoir_Exponential_Outlet::Reservoir_Exponential_Outlet() : Reservoir_Outlet(), c(0.0), expon(0.0)
        {

        }

        /**
         * @brief Parameterized Constructor that builds a Reservoir Outlet object.
         *
         * @param c outlet flux calculation coefficient
         * @param expon outlet flux calculation exponential coefficient
         * @param activation_threshold_meters meters from the bottom of the reservoir to the bottom of the outlet
         * @param max_flux_meters max outlet flux in meters per second
         */
        Reservoir_Exponential_Outlet::Reservoir_Exponential_Outlet(double c, double expon, double activation_threshold_meters,
                                     double max_flux_meters) :
                Reservoir_Outlet(-999.0, -999.0, activation_threshold_meters, max_flux_meters),
                c(c),
                expon(expon)
        {

        }

        /**
         * @brief Calculate outlet discharge flux in meters per second.
         *
         * Perform appropriate calculations to return the flux, in meters per second, of the discharge through this
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
        double Reservoir_Exponential_Outlet::calc_flux_meters_local(reservoir_parameters &parameters_struct,
                                                                                   reservoir_state &storage_struct)
        {
            return c * (exp(expon * storage_struct.current_storage_height_meters /
                            parameters_struct.maximum_storage_meters) - 1);
        }
    }
}
