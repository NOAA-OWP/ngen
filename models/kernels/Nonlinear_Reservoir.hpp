#include <vector>
#include <cmath>
#include <algorithm>

using namespace std;

struct reservoir_parameters
{
    double minimum_storage_meters; 
    double maximum_storage_meters;
};

struct reservoir_state
{
    double current_storage_height_meters;  
};


//This class is for a single reservior outlet that holds the parameters a, b, and the activation threhold, which is
//the height in meters the bottom of the outlet is above the bottom of the reservoir. This class also contains a 
//function to return the velocity in meters per second of the discharge through the outlet. This function will only
//run if the reservoir storage passed in is above the activation threshold.
class Reservoir_Outlet
{
    public:

    //Default Constructor
    Reservoir_Outlet(): activation_threshold_meters(0.0), a(0.0), b(0.0)
    {
		
    }	
   
    //Parameterized Constructor
    Reservoir_Outlet(double a, double b, double activation_threshold_meters): activation_threshold_meters(activation_threshold_meters), a(a), b(b)
    {

    }    
  
    //Function to return the velocity in meters per second of the discharge through the outlet
    double velocity_meters_per_second(reservoir_parameters &parameters_struct, reservoir_state &storage_struct)
    {
        //Return velocity of 0.0 if the storage passed in is less than the activation threshold
        if (storage_struct.current_storage_height_meters <= activation_threshold_meters)
            return 0.0;  
	
        //Return the velocity in meters per second of the discharge through the outlet   
        return a * std::pow((storage_struct.current_storage_height_meters - activation_threshold_meters)
               / (parameters_struct.maximum_storage_meters - activation_threshold_meters), b);
    };

    //Accessor to return activation_threshold_meters
    inline double get_activation_threshold_meters() const
    {
        return activation_threshold_meters;
    };

    private:
    double a;
    double b;
    double activation_threshold_meters;
};

 
//This class is for a nonlinear reservoir that has zero, one, or multiple outlets. The nonlinear reservoir has parameters of minimum
//and maximum storage height in meters and a state variable of current storage height in meters. A vector will be created that
//stores pointers to the reservoir outlet objects. This class will also sort multiple outlets from lowest to highest activation
//thresholds in the vector. The response_storage_meters function takes an inflow and cycles through the outlets from lowest to highest
//activation thresholds and calls the outlet's function to return the discharge velocity in meters per second through the outlet.
//The reservoir's storage is updated from velocities of each outlet and the delta time for the given timestep.
class Nonlinear_Reservoir
{
    public:

    //Constructor for a reservoir with no outlets.
    Nonlinear_Reservoir(double minimum_storage_meters = 0.0, double maximum_storage_meters = 1.0, double current_storage_height_meters = 0.0)
    {
        parameters.minimum_storage_meters = minimum_storage_meters;
        parameters.maximum_storage_meters = maximum_storage_meters;
        state.current_storage_height_meters = current_storage_height_meters;
    }

    //Constructor for a reservoir with only one outlet.
    Nonlinear_Reservoir(double minimum_storage_meters, double maximum_storage_meters, double current_storage_height_meters, double a, 
    double b, double activation_threshold_meters) : Nonlinear_Reservoir(minimum_storage_meters, maximum_storage_meters, current_storage_height_meters)
    {
        //Ensure that the activation threshold is less than the maximum storage
        //if (activation_threshold_meters > maximum_storage_meters)
        //TODO: Return appropriate error

        this->outlets.push_back(Reservoir_Outlet(a, b, activation_threshold_meters));
    }

    //Constructor for a reservoir with multiple outlets.
    Nonlinear_Reservoir(double minimum_storage_meters, double maximum_storage_meters, double current_storage_height_meters, std::vector <Reservoir_Outlet> &outlets) : Nonlinear_Reservoir(minimum_storage_meters, maximum_storage_meters, current_storage_height_meters)
    {
        //Assign outlets vector to class outlets vector variable
        this->outlets = outlets;
	
        //Call fuction to ensure that reservoir outlet activation thresholds are sorted from least to greatest height
        sort_outlets();

        //Ensure that the activation threshold is less than the maximum storage by checking the highest outlet
        if (outlets.back().get_activation_threshold_meters() > maximum_storage_meters)
        {
            //TODO: Return appropriate error
        }    
    }	

    //Function to update the response of storage in meters to an influx and timestep.
    double response_storage_meters(double in_flux_meters_per_second, int delta_time_seconds, double &excess_water_meters)
    {	
        double outlet_velocity_meters_per_second = 0;
        double sum_of_outlet_velocities_meters_per_second = 0;

        //Update current storage from influx multiplied by delta time.
        state.current_storage_height_meters += in_flux_meters_per_second * delta_time_seconds;
	
        //Loop through reservoir outlets.
        for (auto& outlet : this->outlets) //pointer to outlets
        {
            //Calculate outlet velocity.
            outlet_velocity_meters_per_second = outlet.velocity_meters_per_second(parameters, state);

            //Update storage from outlet velocity multiplied by delta time.
            state.current_storage_height_meters -= outlet_velocity_meters_per_second * delta_time_seconds;

            //Add outlet velocity to the sum of velocities.
            sum_of_outlet_velocities_meters_per_second += outlet_velocity_meters_per_second;
        }

        //If storage is less than minimum storage, set to minimum storage and 
        //return negative excess water to be substracted from the input flux.
        if (state.current_storage_height_meters < parameters.minimum_storage_meters)
        {
            state.current_storage_height_meters = parameters.minimum_storage_meters;
	    
            excess_water_meters = (state.current_storage_height_meters - parameters.minimum_storage_meters);
        }

        //If storage is greater than maximum storage, set to maximum storage and 
        //return excess water to be added to the input flux.
        else if (state.current_storage_height_meters > parameters.maximum_storage_meters)
        {
            state.current_storage_height_meters = parameters.maximum_storage_meters;   
             
	    excess_water_meters = (state.current_storage_height_meters - parameters.maximum_storage_meters);
        }

        //If storage remains in bounds of the reservoir, return zero excess water to the input flux.
        else
            excess_water_meters = 0.0;

        return sum_of_outlet_velocities_meters_per_second;
    }

    //Ensure that reservoir outlet activation thresholds are sorted from least to greatest height
    void sort_outlets()
    {
        sort(this->outlets.begin(), this->outlets.end(), [](const Reservoir_Outlet &left, const Reservoir_Outlet &right)
        {              
            return left.get_activation_threshold_meters() < right.get_activation_threshold_meters();
        });
     }

    //Accessor to return storage
    double get_storage_height_meters()
    {
        return state.current_storage_height_meters;
    }

    //TODO: Potential Additions
    //1. Local sub-timestep to see if storage level crosses threshold below activation height of an outlet
    //   or above top of outlet.
    //2. Correct for water level falling below an activation threshold and return a flag to possibly call another
    //   function that calls a partial timestep.
    //3. Return total velocity or individual outlet velocities and these would be multiplied by a watershed area
    //   to get discharge in cubic meters per second.

    private:
    reservoir_parameters parameters;
    reservoir_state state;
    std::vector <Reservoir_Outlet> outlets;
};
