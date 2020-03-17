#include <vector>
#include <cmath>
#include <algorithm>


using namespace std;


struct reservoir_parameters
{
  double minimum_storage_meters; 
  double maximum_storage_meters;
};


//This struct is passed by reference
struct reservoir_state
{
  double current_storage_height_meters;  
};


class Reservoir_Outlet
{
  public:

    double activation_threshold_meters;       
   
    //Default Constructor
    Reservoir_Outlet(): activation_threshold_meters(0.0), a(0.0), b(0.0)
    {
		
    }	
   
    Reservoir_Outlet(double a, double b, double activation_threshold_meters): activation_threshold_meters(activation_threshold_meters), a(a), b(b)
    {

    }    
   
    //RETURN AS VELOCITY M/S. CONVERSION TO VELOCITY 
	//2 OPTIONS: PASS AREA OF WATERSHED TO FUNCTION AND RETURN DISCHAGRG CMS 
	//(IDEAL) OTHER OPTION: RETURN VELOCITY AND MULTIPLY BY WATERSHED AREA IN SUBROUTINE CALLING THIS IN HYMOD OR OTHER CALLING FUNCITON
    //MOCK UP WATERSHED AREA IN HYMOD.
	double velocity_meters_per_second(reservoir_parameters &parameters_struct, reservoir_state &storage_struct)
    {
      if (storage_struct.current_storage_height_meters <= activation_threshold_meters)
        return 0.0;  
	  
      return a * std::pow((storage_struct.current_storage_height_meters - activation_threshold_meters)
      / (parameters_struct.maximum_storage_meters - activation_threshold_meters), b);
    };

  private:
    double a;
    double b;
    //double activation_threshold_meters;   

};

//LOCAL SUB TIMESTEP TO SEE IF CROSS A THRESHOLD BELOW ACTIVATION OR ABOVE TOP OF ACTIVATION. 


//CORRECT FOR WATER LEVEL FALLING BELOW ACTIVATION THRESHOLD. RETURN FLAG AND MAYBE CALL ANOTHER FUNCTION THAT CALLS A PARTIAL TIMESTEP.

//TODO: ADD CHECK TO ENSURE ACTIVATION THRESHOLD IS LESS THAN MAX

//Vector of outlets. Each outlet has a Storage structure. Each outlet has a and b. 
class Nonlinear_Reservoir
{
  public:

    //TODO: DOC WHAT EACH OF THESE CONTRUCTORS DO
    //Nonlinear_Reservoir(double minimum_storage_meters, double maximum_storage_meters, double current_storage_height_meters) : outlets()
    //NO OUTLET
	Nonlinear_Reservoir(double minimum_storage_meters, double maximum_storage_meters, double current_storage_height_meters)
    {

      parameters.minimum_storage_meters = minimum_storage_meters;
      parameters.maximum_storage_meters = maximum_storage_meters;
      state.current_storage_height_meters = current_storage_height_meters;
    }

	//ONE OUTLET
    Nonlinear_Reservoir(double minimum_storage_meters, double maximum_storage_meters, double current_storage_height_meters, double a, double b, double activation_threshold_meters) : Nonlinear_Reservoir(minimum_storage_meters, maximum_storage_meters, current_storage_height_meters)
    {
      this->outlets.push_back(Reservoir_Outlet(a, b, activation_threshold_meters));
    }

    //MULTIPLE OUTLETS
    Nonlinear_Reservoir(double minimum_storage_meters, double maximum_storage_meters, double current_storage_height_meters, std::vector <Reservoir_Outlet> &outlets) : Nonlinear_Reservoir(minimum_storage_meters, maximum_storage_meters, current_storage_height_meters)
    {

      this->outlets = outlets;
	  
      //Ensure that reservoir outlet activation thresholds are sorted from least to greatest height
      (this->outlets.begin(), this->outlets.end(), [](const Reservoir_Outlet &left, const Reservoir_Outlet &right) //-> bool
      {
        return left.activation_threshold_meters < right.activation_threshold_meters;
      });
    }	

//current_storage_height_meters**********
//NOT GOOD TO UPDATE STATE AND FLUX CALCULATORS BACK TO THE CALLING FUNCTION. LET CALLING UPDATE ITSELF.

    //This function assumes that the outlets are sorted from lowest to highest activation thresholds
    double response_storage_meters(double &in_flux_meters_per_second, int delta_time_seconds)
    {	
      state.current_storage_height_meters = in_flux_meters_per_second * delta_time_seconds;
	
      //for (auto Reservoir_Outlet &outlet : this->outlets)
      //for (auto &outlet : this->outlets)
      for (auto outlet : this->outlets)
      {
        //Update storage  
        state.current_storage_height_meters -= outlet.velocity_meters_per_second(parameters, state) * delta_time_seconds;
      }

      //TODO: Return extra storage to caller
      //Is this being called with an in_flux rate or storage as inputs?
      //		
      //if (state.current_storage_height_meters > state.maximum_storage_meters)
      //  in_flux_meters_per_second += (maximum_storage_meters - current_storage_height_meters) / dt
      //  current_storage_height_meters = maximum_storage_meters
    }

    double get_storage_meters()
    {
      return state.current_storage_height_meters;
    }

  private:
    reservoir_parameters parameters;
    reservoir_state state; // State of storages for a given reservoir_parameters
 
    std::vector <Reservoir_Outlet> outlets;	

};



//int main()
//{
//  reservoir = Nonlinear_Reservoir(1.0, 1.0, 1.0);


//}

