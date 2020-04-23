#include <vector>
#include <cmath>
#include <algorithm>
#include <string>
#include <fstream>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include "CSV_Reader.h"
#include <chrono>  // chrono::system_clock
#include <ctime>
#include <time.h>



/*
0. Pass in catchment ID?

01. Should preprocessing of CSV line up times to relevant start time and/or time length of simulation?

1. Set time frame?

2. Parse dates from file?

3. Passing current time, dt, and grid to forcing

4. Pass timestep size to forcing.

5. Realization will construct forcing object

6. Every realization will have its own forcing object.
. GDAL



*/

using namespace std;
//using namespace boost;


class Forcing
{
    public:

// Add start and end time to constructor

//Make forcing csv subclass.

//day_of_year would be a vector and not passed in. built alongside the precip vector
//could be vector circular of vector of 365. Look into date-time. Maybe in AD hydro code. 

    //Default Constructor
    Forcing(): air_temperature_fahrenheit(0.0), basin_latitude(0.0), day_of_year(0), forcing_file_name("")
    {

    }



    Forcing(double air_temperature_fahrenheit, double basin_latitude, int day_of_year, string forcing_file_name, tm *start_date_time, tm *end_date_time): air_temperature_fahrenheit(air_temperature_fahrenheit), basin_latitude(basin_latitude), day_of_year(day_of_year), forcing_file_name(forcing_file_name), start_date_time(*start_date_time), end_date_time(*end_date_time)
    {

        //Call read_forcing function
        read_forcing(forcing_file_name); //also pass in start and end times

        //Initialize forcing vector index to 0;
        *forcing_vector_index_ptr = 0;
    }




/*
template<typename Clock>
Forcing(double air_temperature_fahrenheit, double basin_latitude, int day_of_year, std::chrono::time_point<Clock> start_date_time, std::chrono::time_point<Clock> end_date_time): air_temperature_fahrenheit(air_temperature_fahrenheit), basin_latitude(basin_latitude), day_of_year(day_of_year), std::chrono::time_point<Clock>(start_date_time), std::chrono::time_point<Clock>(end_date_time)
    {

    //call read forcing


    }
*/

/*
template<typename Clock>
Forcing(double air_temperature_fahrenheit, double basin_latitude, int day_of_year, string forcing_file_name, std::chrono::time_point<Clock> start_date_time, std::chrono::time_point<Clock> end_date_time): air_temperature_fahrenheit(air_temperature_fahrenheit), basin_latitude(basin_latitude), day_of_year(day_of_year), forcing_file_name(forcing_file_name), start_date_time(start_date_time), end_date_time(end_date_time)
    {

        //Call read_forcing function
        read_forcing(forcing_file_name); //also pass in start and end times

        //Initialize forcing vector index to 0;
        *forcing_vector_index_ptr = 0;
    }
*/

/////////////PREVIOUS
//add file name
    //Parameterized Constructor
/*
    Forcing(double air_temperature_fahrenheit, double basin_latitude, int day_of_year): air_temperature_fahrenheit(air_temperature_fahrenheit), basin_latitude(basin_latitude), day_of_year(day_of_year)
    {

    //call read forcing
    }
*/


//builds w/ 2 lines below
//template<typename Clock>
//void my_function(std::chrono::time_point<Clock> time_point);


    private:

    //Make private and call this function from constuctor
    //Read Forcing Data from CSV
    //void read_forcing(std::string file_name)
    void read_forcing(string file_name)    //builds fine
    { 

        CSVReader reader(file_name);

	// Get the data from CSV File
	std::vector<std::vector<std::string> > data_list = reader.getData();

        //vector<double> precipitation_vector_meters_per_second_vector;

        std::string::size_type sz;     // alias of size_t

        // Print the content of row by row on screen
        for(std::vector<std::string> vec : data_list)
        {
                 
                //Year
                string year_str = vec[0];

                //Month
                string month_str = vec[1];
              
                if (sizeof(month_str) < 2)
                    month_str = "0" + month_str;
 
                //Day
                string day_str = vec[2];

                if (sizeof(day_str) < 2)
                    day_str = "0" + day_str;           

                //Hour
                string hour_str = vec[3];

                if (sizeof(hour_str) < 2)
                    hour_str = "0" + hour_str; 

                //Concatentate full date-time
                string date_time_str = year_str + month_str + day_str + hour_str;

                ///////////////////
                //if within date range, then add precip to precip vector

                //Precipitation
                string precip_str = vec[5];

                //Convert from string to double and from mm/hr to m/s
                double precip = atof(precip_str.c_str()) / (1000 * 3600);

                precipitation_vector_meters_per_second_vector.push_back(precip);


        }



        /*
	for (vector<double>::iterator i = precipitation_vector_meters_per_second.begin();
		                   i != precipitation_vector_meters_per_second.end();
		                   ++i)
	{
	    cout << *i << endl;
	}
        */

    }


//function for current (current time what forcing object is poitning at) no parameters, be clear of what frequency is

//documeent assumption that it is hourly for now
//function for get_next (inc ptr and then call return current function)  TODO: (could have dt for different freq data than model)

//future quesiton: how do I agg data?

    //Precipitation frequency is assumed to be hourly for now.
    //TODO: Add input for dt (delta time) for different frequencies in the data than the model frequency.
    double get_current_hourly_precipitation_meters_per_second()
    { 
        return precipitation_vector_meters_per_second_vector[*forcing_vector_index_ptr];

    }

    //Precipitation frequency is assumed to be hourly for now.
    //TODO: Add input for dt (delta time) for different frequencies in the data than the model frequency.
    double get_next_hourly_precipitation_meters_per_second()
    {
        //Increment forcing index
        //*forcing_vector_index = *forcing_vector_index + 1;
        *forcing_vector_index_ptr++;

        return get_current_hourly_precipitation_meters_per_second();

    }


//ptr for
    vector<double> precipitation_vector_meters_per_second_vector;
    int *forcing_vector_index_ptr; 
    double precipitation_rate_meters_per_second;
    double air_temperature_fahrenheit;
    double basin_latitude;
    int day_of_year;
    string forcing_file_name;
    tm start_date_time;
    tm end_date_time;


/*
    template<typename Clock>
    std::chrono::time_point<Clock> start_date_time;
    template<typename Clock>
    std::chrono::time_point<Clock> end_date_time;
*/
    /////////////
    //std::chrono::steady_clock::time_point start_date_time;
    //std::chrono::steady_clock::time_point end_date_time;
    //////////// 

    //std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();


    //template<typename Clock>
    //std::chrono::time_point<Clock> time_point;
};
