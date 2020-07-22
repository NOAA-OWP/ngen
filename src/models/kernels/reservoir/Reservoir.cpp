#include "Reservoir.hpp"

using namespace std;

/**
 * @brief Default Constructor building a reservoir with no outlets.
 *
 * @param minimum_storage_meters
 * @param maximum_storage_meters
 * @param current_storage_height_meters
 */
Reservoir::Reservoir(double minimum_storage_meters, double maximum_storage_meters,
                                         double current_storage_height_meters)
{
    parameters.minimum_storage_meters = minimum_storage_meters;
    parameters.maximum_storage_meters = maximum_storage_meters;
    state.current_storage_height_meters = current_storage_height_meters;
}

/**
 * @brief Parameterized Constructor building a reservoir with only one standard base outlet.
 *
 * @param minimum_storage_meters minimum storage in meters
 * @param maximum_storage_meters maximum storage in meters
 * @param current_storage_height_meters current storage height in meters
 * @param a outlet velocity calculation coefficient
 * @param b outlet velocity calculation exponent
 * @param activation_threshold_meters meters from the bottom of the reservoir to the bottom of the outlet
 * @param max_velocity_meters_per_second max outlet velocity in meters per second
 */
Reservoir::Reservoir(double minimum_storage_meters, double maximum_storage_meters,
                                         double current_storage_height_meters, double a, double b,
                                         double activation_threshold_meters, double max_velocity_meters_per_second)
        : Reservoir(minimum_storage_meters, maximum_storage_meters, current_storage_height_meters)
{
    //Ensure that the activation threshold is less than the maximum storage
    //if (activation_threshold_meters > maximum_storage_meters)
    /// \todo TODO: Return appropriate error
    //Add outlet to end of outlet vector
    this->outlets.push_back(
            std::make_shared<Reservoir_Outlet>(a, b, activation_threshold_meters, max_velocity_meters_per_second));
}

/**
 * @brief Parameterized Constructor building a reservoir with only one linear outlet.
 *
 * @param minimum_storage_meters minimum storage in meters
 * @param maximum_storage_meters maximum storage in meters
 * @param current_storage_height_meters current storage height in meters
 * @param a outlet velocity calculation coefficient
 * @param activation_threshold_meters meters from the bottom of the reservoir to the bottom of the outlet
 * @param max_velocity_meters_per_second max outlet velocity in meters per second
 */
Reservoir::Reservoir(double minimum_storage_meters, double maximum_storage_meters,
                                         double current_storage_height_meters, double a,
                                         double activation_threshold_meters, double max_velocity_meters_per_second)
        : Reservoir(minimum_storage_meters, maximum_storage_meters, current_storage_height_meters)
{
    //Ensure that the activation threshold is less than the maximum storage
    //if (activation_threshold_meters > maximum_storage_meters)
    /// \todo TODO: Return appropriate error
    //Add outlet to end of outlet vector
    this->outlets.push_back(
            std::make_shared<Reservoir_Linear_Outlet>(a, activation_threshold_meters, max_velocity_meters_per_second));
}

/**
 * @brief Parameterized Constructor building a reservoir with one or multiple outlets of the base standard and/or
 * exponential type.
 *
 * A reservoir with an exponential outlet can only be instantiated with this constructor.
 *
 * @param minimum_storage_meters minimum storage in meters
 * @param maximum_storage_meters maximum storage in meters
 * @param current_storage_height_meters current storage height in meters
 * @param outlets vector of reservoir outlets
 */
Reservoir::Reservoir(double minimum_storage_meters, double maximum_storage_meters,
                                         double current_storage_height_meters, outlet_vector_type &outlets)
        : Reservoir(minimum_storage_meters, maximum_storage_meters, current_storage_height_meters)
{
    //Assign outlets vector to class outlets vector variable
    this->outlets = outlets;

    //Call fuction to ensure that reservoir outlet activation thresholds are sorted from least to greatest height
    sort_outlets();

    //Call function to Ensure that the activation threshold is less than the max storage by checking the highest outlet
    check_highest_outlet_against_max_storage();
}

// TODO: expand docstring

/**
 * @brief Function to update the reservoir storage in meters and return a response in meters per second to
 * an influx and time step.
 *
 * @param in_flux_meters_per_second influx in meters per second
 * @param delta_time_seconds delta time in seconds
 * @param excess_water_meters excess water in meters
 * @return sum_of_outlet_velocities_meters_per_second sum of the outlet velocities in meters per second
 */
double Reservoir::response_meters_per_second(double in_flux_meters_per_second, int delta_time_seconds,
                                                       double &excess_water_meters)
{
    double outlet_velocity_meters_per_second = 0;
    double sum_of_outlet_velocities_meters_per_second = 0;
    excess_water_meters = 0;

    //Update current storage from influx multiplied by delta time.
    state.current_storage_height_meters += in_flux_meters_per_second * delta_time_seconds;

    //Loop through reservoir outlets.
    for (auto& outlet : this->outlets) //pointer to outlets
    {
        //Calculate outlet velocity.
        //outlet_velocity_meters_per_second = outlet.velocity_meters_per_second(parameters, state);
        outlet_velocity_meters_per_second = outlet->velocity_meters_per_second(parameters, state);

        //Update storage from outlet velocity multiplied by delta time.
        state.current_storage_height_meters -= outlet_velocity_meters_per_second * delta_time_seconds;

        //If storage is less than minimum storage.
        if (state.current_storage_height_meters < parameters.minimum_storage_meters)
        {
            /// \todo TODO: Return appropriate warning
            //cout << "WARNING: Reservoir calculated a storage below the minimum storage." << endl;

            //Return to storage before falling below minimum storage.
            state.current_storage_height_meters += outlet_velocity_meters_per_second * delta_time_seconds;

            //Outlet velocity is set to drain the reservoir to the minimum storage.
            outlet_velocity_meters_per_second =
                    (state.current_storage_height_meters - parameters.minimum_storage_meters) / delta_time_seconds;
            outlet->adjust_velocity(outlet_velocity_meters_per_second);
            //Set storage to minimum storage.
            state.current_storage_height_meters = parameters.minimum_storage_meters;

            excess_water_meters = 0.0;
        }

        //Add outlet velocity to the sum of velocities.
        sum_of_outlet_velocities_meters_per_second += outlet_velocity_meters_per_second;
    }

    //If storage is greater than maximum storage, set to maximum storage and return excess water.
    if (state.current_storage_height_meters > parameters.maximum_storage_meters) {
        /// \todo TODO: Return appropriate warning
        cout << "WARNING: Reservoir calculated a storage above the maximum storage."  << endl;
        excess_water_meters = state.current_storage_height_meters - parameters.maximum_storage_meters;
        state.current_storage_height_meters = parameters.maximum_storage_meters;
    }

    //Ensure that excess_water_meters is not negative
    if (excess_water_meters < 0.0)
    {
        /// \todo TODO: Return appropriate error
        cerr
            << "ERROR: excess_water_meters from the reservoir is calculated to be less than zero."
            << endl;
    }

    return sum_of_outlet_velocities_meters_per_second;
}

/**
 * @brief Adds a preconstructed outlet of any type to the reservoir
 * @param outlet single reservoir outlet
 */
void Reservoir::add_outlet(outlet_type &outlet)
{
    //Add outlet to end of outlet vector
    this->outlets.push_back(outlet);

    //Call fuction to ensure that reservoir outlet activation thresholds are sorted from least to greatest height
    sort_outlets();

    //Call function to ensure that the activation threshold is less than the max storage by checking the highest outlet
    check_highest_outlet_against_max_storage();
}

/**
 * @brief Adds a parameterized standard nonlinear outlet to the reservoir
 * @param a outlet velocity calculation coefficient
 * @param b outlet velocity calculation exponent
 * @param activation_threshold_meters meters from the bottom of the reservoir to the bottom of the outlet
 * @param max_velocity_meters_per_second max outlet velocity in meters per second
 */
void Reservoir::add_outlet(double a, double b, double activation_threshold_meters, double max_velocity_meters_per_second)
{
    //Add outlet to end of outlet vector
    this->outlets.push_back(
            std::make_shared<Reservoir_Outlet>(a, b, activation_threshold_meters, max_velocity_meters_per_second));

    //Call fuction to ensure that reservoir outlet activation thresholds are sorted from least to greatest height
    sort_outlets();

    //Call function to ensure that the activation threshold is less than the max storage by checking the highest outlet
    check_highest_outlet_against_max_storage();
}

/**
 * @brief Adds a parameterized linear outlet to the reservoir
 * @param a outlet velocity calculation coefficient
 * @param activation_threshold_meters meters from the bottom of the reservoir to the bottom of the outlet
 * @param max_velocity_meters_per_second max outlet velocity in meters per second
 */
void Reservoir::add_outlet(double a, double activation_threshold_meters, double max_velocity_meters_per_second)
{
    //Add outlet to end of outlet vector
    this->outlets.push_back(
            std::make_shared<Reservoir_Linear_Outlet>(a, activation_threshold_meters, max_velocity_meters_per_second));

    //Call fuction to ensure that reservoir outlet activation thresholds are sorted from least to greatest height
    sort_outlets();

    //Call function to ensure that the activation threshold is less than the max storage by checking the highest outlet
    check_highest_outlet_against_max_storage();
}

/**
 * @brief Sorts the outlets from lowest to highest activation threshold height.
 */
void Reservoir::sort_outlets()
{
    sort(this->outlets.begin(), this->outlets.end(),
         [](const std::shared_ptr<Reservoir_Outlet> &left, const std::shared_ptr<Reservoir_Outlet> &right)
         {
             return left->get_activation_threshold_meters() < right->get_activation_threshold_meters();
         }
    );
}

/**
 * @brief Ensures that the activation threshold is less than the maximum storage by checking the highest outlet
 */
void Reservoir::check_highest_outlet_against_max_storage()
{
    if (this->outlets.back()->get_activation_threshold_meters() > parameters.maximum_storage_meters)
    {
        /// \todo TODO: Return appropriate error
        cerr
            << "ERROR: The activation_threshold_meters is greater than the maximum_storage_meters of a "
            << "reservoir."
            << endl;
    }
}

/**
 * @brief Accessor to return storage.
 *
 * @return state.current_storage_height_meters current storage height in meters
 */
double Reservoir::get_storage_height_meters()
{
    return state.current_storage_height_meters;
}

/**
 * @brief Return velocity in meters per second of discharge through the specified outlet.
 *
 * Essentially a wrapper for {@link Reservoir_Outlet#velocity_meters_per_second}, where the args for that come from
 * the reservoir anyway.
 *
 * @param outlet_index The index of the desired outlet in this instance's vector of outlets
 * @return
 */
double Reservoir::velocity_meters_per_second_for_outlet(int outlet_index)
{
    //Check bounds on outlet vector
    /// \todo: Implement unordered_map outlet_map
    if (outlet_index >= 0 && outlet_index < outlets.size())
        return this->outlets.at(outlet_index)->get_previously_calculated_velocity_meters_per_second();

    else if (outlets.size() > 0)
    {
        cout
            << "Warning, reservoir outlet requested is not in the outlets vector. Returning the velocity of the "
            << "first outlet."
            << endl;
        return this->outlets.at(0)->get_previously_calculated_velocity_meters_per_second();
    }

    else
    {
        cout
            << "Warning, reservoir outlet requested of reservoir with no outlets. Returning a velocity of 0.0 "
            << "meters per second."
            << endl;
        return 0.0;
    }
}
