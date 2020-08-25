#include <iostream>
#include "Reservoir_Outlet_Timeless.hpp"

namespace Reservoir{
    namespace Implicit_Time{
        
        /**
         * @brief Default Constructor building an empty Reservoir Outlet Object.
         */
        Reservoir_Outlet::Reservoir_Outlet() : a(0.0), b(0.0), activation_threshold_meters(0.0), max_flux_meters(0.0),
                            flux_meters_local(0.0)
        {

        }

        /**
         * @brief Parameterized Constuctor that builds a Reservoir Outlet object.
         *
         * @param a outlet flux calculation coefficient
         * @param b outlet flux calculation exponent
         * @param activation_threshold_meters meters from the bottom of the reservoir to the bottom of the outlet
         * @param max_flux_meters max outlet flux in meters
         */
        Reservoir_Outlet::Reservoir_Outlet(double a, double b, double activation_threshold_meters, double max_flux_meters):
                a(a), b(b), activation_threshold_meters(activation_threshold_meters),
                max_flux_meters(max_flux_meters), flux_meters_local(0.0)
        {

        }

        /**
         * Set the locally maintained flux state variable value.
         *
         * @param flux_meters The flux value to set, in meters.
         */
        void Reservoir_Outlet::adjust_flux(double flux_meters)
        {
            flux_meters_local = flux_meters;
        }

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
         */
        double Reservoir_Outlet::calc_flux_meters_local(reservoir_parameters &parameters_struct,
                                                                       reservoir_state &storage_struct)
        {
            return a * std::pow(
                    (
                            (storage_struct.current_storage_height_meters - activation_threshold_meters)
                            /
                            (parameters_struct.maximum_storage_meters - activation_threshold_meters)
                    ), b);
        }

        /**
         * @brief Accessor to return activation_threshold_meters.
         *
         * @return activation_threshold_meters meters from the bottom of the reservoir to the bottom of the outlet
         */
        double Reservoir_Outlet::get_activation_threshold_meters() const
        {
            return activation_threshold_meters;
        };

        /**
         * @brief Accessor to return flux_meters_local that is previously calculated.
         *
         * @return flux_meters_local
         */
        double Reservoir_Outlet::get_previously_calculated_flux_meters()
        {
            return flux_meters_local;
        };

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
         * @return flux_meters_local the flux in meters of the discharge through the outlet
         * @see calc_flux_meters_local
         */
        double Reservoir_Outlet::flux_meters(reservoir_parameters &parameters_struct, reservoir_state &storage_struct)
        {
            // Return flux of 0.0 if the storage passed in is less than the activation threshold
            if (storage_struct.current_storage_height_meters <= activation_threshold_meters) {
                flux_meters_local = 0.0;
                return flux_meters_local;
            }

            // Calculate the flux in meters of the discharge through the outlet
            flux_meters_local = calc_flux_meters_local(parameters_struct, storage_struct);

            // If calculated outlet flux is greater than max flux, then set to max flux and return a warning.
            if (flux_meters_local > max_flux_meters) {
                flux_meters_local = max_flux_meters;

                //TODO: Return appropriate warning
                std::cout
                        << "WARNING: Reservoir calculated an outlet flux over max flux, and therefore "
                        << "set the outlet flux to max flux."
                        << std::endl;
            }

            //Return the flux in meters of the discharge through the outlet
            return flux_meters_local;
        };
    }
}
