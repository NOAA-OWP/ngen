#ifndef NGEN_RESERVOIR_EXPONENTIAL_OUTLET_H
#define NGEN_RESERVOIR_EXPONENTIAL_OUTLET_H

#include "Reservoir_Outlet.hpp"

/**
 * @brief Single Exponential Reservior Outlet Class
 * This class is for a single exponential reservoir outlet that is derived from the base Reservoir_Outlet class.
 * This derived class holds extra parameters for c and expon and overrides the base velocity_meters_per_second function
 * with an exponential equation.
 */
class Reservoir_Exponential_Outlet: public Reservoir_Outlet
{
    public:

    /**
     * @brief Default Constructor building an empty Reservoir Exponential Outlet Object
     */
    Reservoir_Exponential_Outlet(): Reservoir_Outlet(), c(0.0), expon(0.0)
    {

    }

    /**
     * @brief Parameterized Constuctor that builds a Reservoir Outlet object
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

    /**
     * @brief Inherited, overridden function to return discharge velocity through exponential outlet (meters per second)
     * @param parameters_struct reservoir parameters struct
     * @param storage_struct reservoir state storage struct
     * @param activation_threshold_meters meters from the bottom of the reservoir to the bottom of the outlet
     * @param max_velocity_meters_per_second max outlet velocity in meters per second
     * @return velocity_meters_per_second_local the velocity in meters per second of the discharge through the outlet
     */
    double velocity_meters_per_second(reservoir_parameters &parameters_struct, reservoir_state &storage_struct) override
    {

        //Return velocity of 0.0 if the storage passed in is less than the activation threshold
        if (storage_struct.current_storage_height_meters <= activation_threshold_meters) {
            return 0.0;
        }

        //Calculate the velocity in meters per second of the discharge through the outlet
        velocity_meters_per_second_local = c * (exp(expon * storage_struct.current_storage_height_meters /
                                                    parameters_struct.maximum_storage_meters) - 1);

        //If calculated oulet velocity is greater than max velocity, then set to max velocity and return a warning.
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

    private:
    double c;
    double expon;
};

#endif //NGEN_RESERVOIR_EXPONENTIAL_OUTLET_H
