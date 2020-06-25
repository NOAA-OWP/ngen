#ifndef NGEN_RESERVOIR_OUTLET_H
#define NGEN_RESERVOIR_OUTLET_H

#include <cmath>
#include "reservoir_parameters.h"
#include "reservoir_state.h"

/**
 * @brief Base Single Reservior Outlet Class
 * This class is for a single reservior outlet that holds the parameters a, b, and the activation threhold, which is
 * the height in meters the bottom of the outlet is above the bottom of the reservoir. This class also contains a
 * function to return the velocity in meters per second of the discharge through the outlet. This function will only
 * run if the reservoir storage passed in is above the activation threshold.
 */
class Reservoir_Outlet
{
public:

    /**
     * @brief Default Constructor building an empty Reservoir Outlet Object
     */
    Reservoir_Outlet(): a(0.0), b(0.0), activation_threshold_meters(0.0), max_velocity_meters_per_second(0.0),
                        velocity_meters_per_second_local(0.0)
    {

    }

    /**
     * @brief Parameterized Constuctor that builds a Reservoir Outlet object
     * @param a outlet velocity calculation coefficient
     * @param b outlet velocity calculation exponent
     * @param activation_threshold_meters meters from the bottom of the reservoir to the bottom of the outlet
     * @param max_velocity_meters_per_second max outlet velocity in meters per second
     */
    Reservoir_Outlet(double a, double b, double activation_threshold_meters, double max_velocity_meters_per_second):
            a(a), b(b), activation_threshold_meters(activation_threshold_meters),
            max_velocity_meters_per_second(max_velocity_meters_per_second), velocity_meters_per_second_local(0.0)
    {

    }

    /**
     * @brief Function to return the velocity in meters per second of the discharge through the outlet
     * @param parameters_struct reservoir parameters struct
     * @param storage_struct reservoir state storage struct
     * @param activation_threshold_meters meters from the bottom of the reservoir to the bottom of the outlet
     * @param max_velocity_meters_per_second max outlet velocity in meters per second
     * @return velocity_meters_per_second_local the velocity in meters per second of the discharge through the outlet
     */
    virtual double velocity_meters_per_second(reservoir_parameters &parameters_struct, reservoir_state &storage_struct)
    {

        // Return velocity of 0.0 if the storage passed in is less than the activation threshold
        if (storage_struct.current_storage_height_meters <= activation_threshold_meters) {
            velocity_meters_per_second_local = 0.0;
            return 0.0;
        }

        // Calculate the velocity in meters per second of the discharge through the outlet
        velocity_meters_per_second_local =
                a * std::pow(
                        (
                                (storage_struct.current_storage_height_meters - activation_threshold_meters)
                                /
                                (parameters_struct.maximum_storage_meters - activation_threshold_meters)
                        ), b);

        // If calculated outlet velocity is greater than max velocity, then set to max velocity and return a warning.
        if (velocity_meters_per_second_local > max_velocity_meters_per_second) {
            velocity_meters_per_second_local = max_velocity_meters_per_second;

            //TODO: Return appropriate warning
            std::__1::cout
                    << "WARNING: Nonlinear reservoir calculated an outlet velocity over max velocity, and therefore "
                    << "set the outlet velocity to max velocity."
                    << std::__1::endl;
        }

        //Return the velocity in meters per second of the discharge through the outlet
        return velocity_meters_per_second_local;
    };

    /**
     * @brief Accessor to return activation_threshold_meters
     * @return activation_threshold_meters meters from the bottom of the reservoir to the bottom of the outlet
     */
    inline double get_activation_threshold_meters() const
    {
        return activation_threshold_meters;
    };

    /**
     * @brief Accessor to return velocity_meters_per_second_local that is previously calculated
     * @return velocity_meters_per_second_local
     */
    double get_previously_calculated_velocity_meters_per_second()
    {
        return velocity_meters_per_second_local;
    };

    void adjust_velocity(double velocity_meters_per_second)
    {
        velocity_meters_per_second_local = velocity_meters_per_second;
    }

protected:
    double a;
    double b;
    double activation_threshold_meters;
    double max_velocity_meters_per_second;
    double velocity_meters_per_second_local;
};

#endif //NGEN_RESERVOIR_OUTLET_H
