#ifndef RESERVOIR_HPP
#define RESERVOIR_HPP

#include <vector>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <memory>
#include "Reservoir_Outlet.hpp"
#include "Reservoir_Linear_Outlet.hpp"
#include "Reservoir_Exponential_Outlet.hpp"
#include "reservoir_parameters.h"
#include "reservoir_state.h"

using namespace std;

namespace Reservoir{
    namespace Explicit_Time{

        /**
         * @brief Reservoir that has zero, one, or multiple outlets.
         * The reservoir has parameters of minimum and maximum storage height in meters and a state variable of
         * current storage height in meters. A vector will be created that stores pointers to the reservoir outlet objects. This
         * class will also sort multiple outlets from lowest to highest activation thresholds in the vector. The
         * response_meters_per_second function takes an inflow and cycles through the outlets from lowest to highest activation
         * thresholds and calls the outlet's function to return the discharge velocity in meters per second through the outlet.
         * The reservoir's storage is updated from velocities of each outlet and the delta time for the given timestep.
         */
        class Reservoir
        {
            public:

            typedef std::vector <std::shared_ptr<Reservoir_Outlet>> outlet_vector_type;
            typedef std::shared_ptr<Reservoir_Outlet> outlet_type;

            /**
             * @brief Default Constructor building a reservoir with no outlets.
             */
            Reservoir(double minimum_storage_meters = 0.0, double maximum_storage_meters = 1.0,
                                double current_storage_height_meters = 0.0);

            /**
             * @brief Parameterized Constructor building a reservoir with only one standard base outlet.
             * @param minimum_storage_meters minimum storage in meters
             * @param maximum_storage_meters maximum storage in meters
             * @param current_storage_height_meters current storage height in meters
             * @param a outlet velocity calculation coefficient
             * @param b outlet velocity calculation exponent
             * @param activation_threshold_meters meters from the bottom of the reservoir to the bottom of the outlet
             * @param max_velocity_meters_per_second max outlet velocity in meters per second
             */
            Reservoir(double minimum_storage_meters, double maximum_storage_meters,
                                double current_storage_height_meters, double a,
                                double b, double activation_threshold_meters, double max_velocity_meters_per_second);

            /**
             * @brief Parameterized Constructor building a reservoir with only one linear outlet.
             * @param minimum_storage_meters minimum storage in meters
             * @param maximum_storage_meters maximum storage in meters
             * @param current_storage_height_meters current storage height in meters
             * @param a outlet velocity calculation coefficient
             * @param activation_threshold_meters meters from the bottom of the reservoir to the bottom of the outlet
             * @param max_velocity_meters_per_second max outlet velocity in meters per second
             */
            Reservoir(double minimum_storage_meters, double maximum_storage_meters,
                                double current_storage_height_meters, double a,
                                double activation_threshold_meters, double max_velocity_meters_per_second);

            /**
             * @brief Parameterized Constructor building a reservoir with one or multiple outlets of the base standard and/or
             * exponential type.
             * A reservoir with an exponential outlet can only be instantiated with this constructor.
             * @param minimum_storage_meters minimum storage in meters
             * @param maximum_storage_meters maximum storage in meters
             * @param current_storage_height_meters current storage height in meters
             * @param outlets vector of reservoir outlets
             */
            Reservoir(double minimum_storage_meters, double maximum_storage_meters,
                                double current_storage_height_meters, outlet_vector_type &outlets);

            /**
             * @brief Function to update the reservoir storage in meters and return a response in meters per second to
             * an influx and time step.
             *
             * Note that the function also takes a reference parameter for holding the amount of excess water for the reservoir
             * beyond its maximum storage after accounting for the input amount.  All calls to this function will initialize this
             * parameter, without accounting for any previous value, so any usage should ensure the passed reference does not
             * contain a value that is still needed but not stored elsewhere.
             *
             * @param in_flux_meters_per_second influx in meters per second
             * @param delta_time_seconds delta time in seconds
             * @param excess_water_meters Reference to an amount of excess water in meters, after considering input and max storage.
             * @return sum_of_outlet_velocities_meters_per_second sum of the outlet velocities in meters per second
             */
            double response_meters_per_second(double in_flux_meters_per_second, int delta_time_seconds,
                                              double &excess_water_meters);

            /**
             * @brief Adds a preconstructed outlet of any type to the reservoir
             * @param outlet single reservoir outlet
             */
            void add_outlet(outlet_type &outlet);

            /**
             * @brief Adds a parameterized standard nonlinear outlet to the reservoir
             * @param a outlet velocity calculation coefficient
             * @param b outlet velocity calculation exponent
             * @param activation_threshold_meters meters from the bottom of the reservoir to the bottom of the outlet
             * @param max_velocity_meters_per_second max outlet velocity in meters per second
             */
            void add_outlet(double a, double b, double activation_threshold_meters, double max_velocity_meters_per_second);

            /**
             * @brief Adds a parameterized linear outlet to the reservoir
             * @param a outlet velocity calculation coefficient
             * @param activation_threshold_meters meters from the bottom of the reservoir to the bottom of the outlet
             * @param max_velocity_meters_per_second max outlet velocity in meters per second
             */
            void add_outlet(double a, double activation_threshold_meters, double max_velocity_meters_per_second);

            /**
             * @brief Sorts the outlets from lowest to highest acitivation threshold height
             */
            void sort_outlets();

            /**
             * @brief Ensures that the activation threshold is less than the maximum storage by checking the highest outlet
             */
            void check_highest_outlet_against_max_storage();

            /**
             * @brief Accessor to return storage
             * @return state.current_storage_height_meters current storage height in meters
             */
            double get_storage_height_meters();

            /**
             * @brief Return velocity in meters per second of discharge through the specified outlet.
             * Essentially a wrapper for {@link Reservoir_Outlet#velocity_meters_per_second}, where the args for that come from
             * the reservoir anyway.
             * @param outlet_index The index of the desired outlet in this instance's vector of outlets
             * @return
             */
            double velocity_meters_per_second_for_outlet(int outlet_index);

            /**
             * /// \todo TODO: Potential Additions
             *1. Local sub-timestep to see if storage level crosses threshold below activation height of an outlet
             *   or above top of outlet.
             *2. Correct for water level falling below an activation threshold and return a flag to possibly call another
             *   function that calls a partial timestep.
             *3. Return total velocity or individual outlet velocities and these would be multiplied by a watershed area
             *   to get discharge in cubic meters per second.
             */

            private:
            reservoir_parameters parameters;
            reservoir_state state;
            outlet_vector_type outlets;
        };
    }
}

#endif  // RESERVOIR_HPP
