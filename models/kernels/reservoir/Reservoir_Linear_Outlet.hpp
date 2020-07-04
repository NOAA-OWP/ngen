#ifndef NGEN_RESERVOIR_LINEAR_OUTLET_H
#define NGEN_RESERVOIR_LINEAR_OUTLET_H

#include "Reservoir_Outlet.hpp"

/**
 * @brief Single Linear Reservior Outlet Class
 * This class is for a single linear reservoir outlet that is derived from the base Reservoir_Outlet class.
 */
class Reservoir_Linear_Outlet: public Reservoir_Outlet
{
    public:

    /**
     * @brief Default Constructor building a Linear Reservoir Outlet Object
     */
    Reservoir_Linear_Outlet();


    /**
     * @brief Parameterized Constructor that builds a Linear Reservoir Outlet object.
     * @param a outlet velocity calculation coefficient
     * @param activation_threshold_meters meters from the bottom of the reservoir to the bottom of the outlet
     * @param max_velocity_meters_per_second max outlet velocity in meters per second
     */
    Reservoir_Linear_Outlet(double a, double activation_threshold_meters,
                                 double max_velocity_meters_per_second);

};

#endif //NGEN_RESERVOIR_LINEAR_OUTLET_H
