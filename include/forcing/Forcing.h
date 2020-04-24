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
#include <memory>

using namespace std;

class Forcing
{
    public:

    typedef struct tm time_type;


    //Default Constructor
    Forcing(): air_temperature_fahrenheit(0.0), basin_id(0), forcing_file_name("")
    {

    }

    //Parameterized Constuctor
    Forcing(double air_temperature_fahrenheit, double basin_latitude, string forcing_file_name, std::shared_ptr<time_type>  start_date_time, std::shared_ptr<time_type> end_date_time): air_temperature_fahrenheit(air_temperature_fahrenheit), basin_id(basin_id), forcing_file_name(forcing_file_name), start_date_time(start_date_time), end_date_time(end_date_time)
    {
        //Convert start and end time structs to epoch time
        start_date_time_epoch = mktime(start_date_time.get());

        end_date_time_epoch =  mktime(end_date_time.get());

        current_date_time_epoch = start_date_time_epoch;

        //Call read_forcing function
        read_forcing(forcing_file_name); //also pass in start and end times

        //Initialize forcing vector index to 0;
        *forcing_vector_index_ptr = 0;
    }


    //Precipitation frequency is assumed to be hourly for now.
    //TODO: Add input for dt (delta time) for different frequencies in the data than the model frequency.
    double get_current_hourly_precipitation_meters_per_second()
    { 
        return precipitation_meters_per_second_vector[*forcing_vector_index_ptr];
    }


    //Precipitation frequency is assumed to be hourly for now.
    //TODO: Add input for dt (delta time) for different frequencies in the data than the model frequency.
    double get_next_hourly_precipitation_meters_per_second()
    {
        //Increment forcing index
        *forcing_vector_index_ptr = *forcing_vector_index_ptr + 1;

        //Increment current time by 1 hour
        current_date_time_epoch = current_date_time_epoch + 3600;

        return get_current_hourly_precipitation_meters_per_second();
    }

    //Get day of year
    int get_day_of_year()
    {
        int current_day_of_year;

        struct tm *current_date_time;

        current_date_time = localtime(&current_date_time_epoch);
                
        current_day_of_year = current_date_time->tm_yday;    

        return current_day_of_year;
    }

    private:

    //Read Forcing Data from CSV
    void read_forcing(string file_name)
    { 
        //Call CSVReader constuctor
        CSVReader reader(file_name);

	//Get the data from CSV File
	std::vector<std::vector<std::string> > data_list = reader.getData();

        //Iterate through CSV starting on the third row
        for (int i = 2; i < data_list.size(); i++)
        {
                //Row vector
                std::vector<std::string>& vec = data_list[i];
               
                //Declare pointer to struct for the current row date-time
                struct tm *current_row_date_time;
               
                //Allocate memory to struct for the current row date-time
                current_row_date_time = new tm();

                //Year
                string year_str = vec[0];
                int year = stoi(year_str);
                current_row_date_time->tm_year = year - 1900;

                //Month
                string month_str = vec[1];
                int month = stoi(month_str);              
                current_row_date_time->tm_mon = month - 1;

                //Day
                string day_str = vec[2];
                int day = stoi(day_str);
                current_row_date_time->tm_mday = day;

                //Hour
                string hour_str = vec[3];
                int hour = stoi(hour_str);
                current_row_date_time->tm_hour = hour;

                //Convert current row date-time to epoch time
                time_t current_row_date_time_epoch = mktime(current_row_date_time);

                //If the current row date-time is within the model date-time range, then add precipitation to vector
                if (start_date_time_epoch <= current_row_date_time_epoch &&  current_row_date_time_epoch <= end_date_time_epoch)
                {
                    //Precipitation
                    string precip_str = vec[5];
    
                    //Convert from string to double and from mm/hr to m/s
                    double precip = atof(precip_str.c_str()) / (1000 * 3600);

                    //Add precip to vector
                    precipitation_meters_per_second_vector.push_back(precip);
                }

                //Free memory from struct
                delete current_row_date_time;
        }        
    }


    vector<double> precipitation_meters_per_second_vector;
    int *forcing_vector_index_ptr; 
    double precipitation_rate_meters_per_second;
    double air_temperature_fahrenheit;
    int basin_id;
    int day_of_year;
    string forcing_file_name;

    std::shared_ptr<time_type> start_date_time;
    std::shared_ptr<time_type> end_date_time;

    time_t start_date_time_epoch;
    time_t end_date_time_epoch;
    time_t current_date_time_epoch;
};

//TODO: Consider aggregating precipiation data
//TODO: Make CSV forcing a subclass
//TODO: Consider passing grid to class
//TODO: Consider following GDAL API functionality

