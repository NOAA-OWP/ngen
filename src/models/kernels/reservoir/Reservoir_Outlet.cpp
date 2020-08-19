#include <iostream>
#include "Reservoir_Outlet.hpp"

namespace Reservoir{
    namespace Explicit_Time{
                
        /**
         * @brief Default Constructor building an empty Reservoir Outlet Object.
         */
        Reservoir_Outlet::Reservoir_Outlet() : a(0.0), b(0.0), activation_threshold_meters(0.0), max_velocity_meters_per_second(0.0),
                            velocity_meters_per_second_local(0.0)
        {

        }

        /**
         * @brief Parameterized Constuctor that builds a Reservoir Outlet object.
         *
         * @param a outlet velocity calculation coefficient
         * @param b outlet velocity calculation exponent
         * @param activation_threshold_meters meters from the bottom of the reservoir to the bottom of the outlet
         * @param max_velocity_meters_per_second max outlet velocity in meters per second
         */
        Reservoir_Outlet::Reservoir_Outlet(double a, double b, double activation_threshold_meters, double max_velocity_meters_per_second):
                a(a), b(b), activation_threshold_meters(activation_threshold_meters),
                max_velocity_meters_per_second(max_velocity_meters_per_second), velocity_meters_per_second_local(0.0)
        {

        }

        /**
         * Set the locally maintained velocity state variable value.
         *
         * @param velocity_meters_per_second The velocity value to set, in meters per second.
         */
        void Reservoir_Outlet::adjust_velocity(double velocity_meters_per_second)
        {
            velocity_meters_per_second_local = velocity_meters_per_second;
        }

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
         */
        double Reservoir_Outlet::calc_velocity_meters_per_second_local(reservoir_parameters &parameters_struct,
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
         * @brief Accessor to return velocity_meters_per_second_local that is previously calculated.
         *
         * @return velocity_meters_per_second_local
         */
        double Reservoir_Outlet::get_previously_calculated_velocity_meters_per_second()
        {
            return velocity_meters_per_second_local;
        };

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
         * @return velocity_meters_per_second_local the velocity in meters per second of the discharge through the outlet
         * @see calc_velocity_meters_per_second_local
         */
        double Reservoir_Outlet::velocity_meters_per_second(reservoir_parameters &parameters_struct, reservoir_state &storage_struct)
        {
            // Return velocity of 0.0 if the storage passed in is less than the activation threshold
            if (storage_struct.current_storage_height_meters <= activation_threshold_meters) {
                velocity_meters_per_second_local = 0.0;
                return velocity_meters_per_second_local;
            }

            // Calculate the velocity in meters per second of the discharge through the outlet
            velocity_meters_per_second_local = calc_velocity_meters_per_second_local(parameters_struct, storage_struct);

            // If calculated outlet velocity is greater than max velocity, then set to max velocity and return a warning.
            if (velocity_meters_per_second_local > max_velocity_meters_per_second) {
                velocity_meters_per_second_local = max_velocity_meters_per_second;

                //TODO: Return appropriate warning
                std::cout
                        << "WARNING: Reservoir calculated an outlet velocity over max velocity, and therefore "
                        << "set the outlet velocity to max velocity."
                        << std::endl;
            }

            //Return the velocity in meters per second of the discharge through the outlet
            return velocity_meters_per_second_local;
        };
    }
}
