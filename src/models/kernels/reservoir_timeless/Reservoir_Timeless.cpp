#include "Reservoir_Timeless.hpp"

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
 * @param a outlet flux calculation coefficient
 * @param b outlet flux calculation exponent
 * @param activation_threshold_meters meters from the bottom of the reservoir to the bottom of the outlet
 * @param max_flux_meters max outlet flux in meters
 */
Reservoir::Reservoir(double minimum_storage_meters, double maximum_storage_meters,
                                         double current_storage_height_meters, double a, double b,
                                         double activation_threshold_meters, double max_flux_meters)
        : Reservoir(minimum_storage_meters, maximum_storage_meters, current_storage_height_meters)
{
    //Ensure that the activation threshold is less than the maximum storage
    //if (activation_threshold_meters > maximum_storage_meters)
    /// \todo TODO: Return appropriate error
    //Add outlet to end of outlet vector
    this->outlets.push_back(
            std::make_shared<Reservoir_Outlet>(a, b, activation_threshold_meters, max_flux_meters));
}

/**
 * @brief Parameterized Constructor building a reservoir with only one linear outlet.
 *
 * @param minimum_storage_meters minimum storage in meters
 * @param maximum_storage_meters maximum storage in meters
 * @param current_storage_height_meters current storage height in meters
 * @param a outlet flux calculation coefficient
 * @param activation_threshold_meters meters from the bottom of the reservoir to the bottom of the outlet
 * @param max_flux_meters max outlet flux in meters
 */
Reservoir::Reservoir(double minimum_storage_meters, double maximum_storage_meters,
                                         double current_storage_height_meters, double a,
                                         double activation_threshold_meters, double max_flux_meters)
        : Reservoir(minimum_storage_meters, maximum_storage_meters, current_storage_height_meters)
{
    //Ensure that the activation threshold is less than the maximum storage
    //if (activation_threshold_meters > maximum_storage_meters)
    /// \todo TODO: Return appropriate error
    //Add outlet to end of outlet vector
    this->outlets.push_back(
            std::make_shared<Reservoir_Linear_Outlet>(a, activation_threshold_meters, max_flux_meters));
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
 * @brief Function to update the reservoir storage in meters and return a response in meters to
 * an influx.
 *
 * @param in_flux_meters influx in meters
 * @param excess_water_meters excess water in meters
 * @return sum_of_outlet_fluxes_meters sum of the outlet fluxes in meters
 */
double Reservoir::response_meters(double in_flux_meters, double &excess_water_meters)
{
    double outlet_flux_meters = 0;
    double sum_of_outlet_fluxes_meters = 0;

    //Update current storage from influx.
    state.current_storage_height_meters += in_flux_meters;

    //Loop through reservoir outlets.
    for (auto& outlet : this->outlets) //pointer to outlets
    {
        //Calculate outlet flux.
        //outlet_flux_meters = outlet.flux_meters(parameters, state);
        outlet_flux_meters = outlet->flux_meters(parameters, state);

        //Update storage from outlet flux.
        state.current_storage_height_meters -= outlet_flux_meters;

        //If storage is less than minimum storage.
        if (state.current_storage_height_meters < parameters.minimum_storage_meters)
        {
            /// \todo TODO: Return appropriate warning
            //cout << "WARNING: Reservoir calculated a storage below the minimum storage." << endl;

            //Return to storage before falling below minimum storage.
            state.current_storage_height_meters += outlet_flux_meters;

            //Outlet flux is set to drain the reservoir to the minimum storage.
            outlet_flux_meters =
                    (state.current_storage_height_meters - parameters.minimum_storage_meters);
            outlet->adjust_flux(outlet_flux_meters);
            //Set storage to minimum storage.
            state.current_storage_height_meters = parameters.minimum_storage_meters;

            excess_water_meters = 0.0;
        }

        //Add outlet flux to the sum of fluxes.
        sum_of_outlet_fluxes_meters += outlet_flux_meters;
    }

    //If storage is greater than maximum storage, set to maximum storage and return excess water.
    if (state.current_storage_height_meters > parameters.maximum_storage_meters)
    {
        /// \todo TODO: Return appropriate warning
        cout << "WARNING: Reservoir calculated a storage above the maximum storage."  << endl;

        state.current_storage_height_meters = parameters.maximum_storage_meters;

        excess_water_meters = (state.current_storage_height_meters - parameters.maximum_storage_meters);
    }

    //Ensure that excess_water_meters is not negative
    if (excess_water_meters < 0.0)
    {
        /// \todo TODO: Return appropriate error
        cerr
            << "ERROR: excess_water_meters from the reservoir is calculated to be less than zero."
            << endl;
    }
 
    return sum_of_outlet_fluxes_meters;
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
 * @param a outlet flux calculation coefficient
 * @param b outlet flux calculation exponent
 * @param activation_threshold_meters meters from the bottom of the reservoir to the bottom of the outlet
 * @param max_flux_meters max outlet flux in meters
 */
void Reservoir::add_outlet(double a, double b, double activation_threshold_meters, double max_flux_meters)
{
    //Add outlet to end of outlet vector
    this->outlets.push_back(
            std::make_shared<Reservoir_Outlet>(a, b, activation_threshold_meters, max_flux_meters));

    //Call fuction to ensure that reservoir outlet activation thresholds are sorted from least to greatest height
    sort_outlets();

    //Call function to ensure that the activation threshold is less than the max storage by checking the highest outlet
    check_highest_outlet_against_max_storage();
}

/**
 * @brief Adds a parameterized linear outlet to the reservoir
 * @param a outlet flux calculation coefficient
 * @param activation_threshold_meters meters from the bottom of the reservoir to the bottom of the outlet
 * @param max_flux_meters max outlet flux in meters
 */
void Reservoir::add_outlet(double a, double activation_threshold_meters, double max_flux_meters)
{
    //Add outlet to end of outlet vector
    this->outlets.push_back(
            std::make_shared<Reservoir_Linear_Outlet>(a, activation_threshold_meters, max_flux_meters));

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
 * @brief Return flux in meters of discharge through the specified outlet.
 *
 * Essentially a wrapper for {@link Reservoir_Outlet#flux_meters}, where the args for that come from
 * the reservoir anyway.
 *
 * @param outlet_index The index of the desired outlet in this instance's vector of outlets
 * @return
 */
double Reservoir::flux_meters_for_outlet(int outlet_index)
{
    //Check bounds on outlet vector
    /// \todo: Implement unordered_map outlet_map
    if (outlet_index >= 0 && outlet_index < outlets.size())
        return this->outlets.at(outlet_index)->get_previously_calculated_flux_meters();

    else if (outlets.size() > 0)
    {
        cout
            << "Warning, reservoir outlet requested is not in the outlets vector. Returning the flux of the "
            << "first outlet."
            << endl;
        return this->outlets.at(0)->get_previously_calculated_flux_meters();
    }

    else
    {
        cout
            << "Warning, reservoir outlet requested of reservoir with no outlets. Returning a flux of 0.0 "
            << "meters."
            << endl;
        return 0.0;
    }
}
