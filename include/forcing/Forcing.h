#include <vector>
#include <cmath>
#include <algorithm>
#include <string>
#include <fstream>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include "CSV_Reader.h"
#include <ctime>
#include <time.h>
#include <memory>

using namespace std;

/**
 * @brief Forcing class providing time-series precipiation forcing data to the model.
 */
class Forcing
{
    public:

    typedef struct tm time_type;

    /**
     * Default Constructor building an empty Forcing object
     */
    Forcing(): air_temperature_fahrenheit(0.0), basin_id(0), forcing_file_name("")
    {

    }

    /**
     * @brief Parameterized Constuctor that builds a Forcing object and reads an input forcing CSV into a vector. 
     * @param air_temperature_fahrenheit Air temperatrure in Fahrenheit
     * @param basin_latitude Basin Latitude
     * @param forcing_file_name Forcing file name
     * @param start_date_time Start date-time of model to select start of forcing time window of data
     * @param end_date_time End date-time of model to select end of forcing time window of data
     */
    Forcing(double air_temperature_fahrenheit, double basin_latitude, string forcing_file_name, std::shared_ptr<time_type>  start_date_time, std::shared_ptr<time_type> end_date_time): air_temperature_fahrenheit(air_temperature_fahrenheit), basin_id(basin_id), forcing_file_name(forcing_file_name), start_date_time(start_date_time), end_date_time(end_date_time)
    {

        //Convert start and end time structs to epoch time
        start_date_time_epoch = mktime(start_date_time.get());

        end_date_time_epoch =  mktime(end_date_time.get());

        current_date_time_epoch = start_date_time_epoch;

        //Call read_forcing function
        //read_forcing(forcing_file_name);
        read_forcing_aorc(forcing_file_name);

        //Initialize forcing vector index to 0;
        forcing_vector_index_ptr = 0;
    }


    /**
     * @brief Gets current hourly precipitation in meters per second
     * Precipitation frequency is assumed to be hourly for now.
     * TODO: Add input for dt (delta time) for different frequencies in the data than the model frequency.
     * @return the current hourly precipitation in meters per second
     */
    double get_current_hourly_precipitation_meters_per_second()
    { 
        return precipitation_rate_meters_per_second_vector[forcing_vector_index_ptr];
    }

    /**
     * @brief Gets next hourly precipitation in meters per second
     * Increments pointer in forcing vector by one timestep
     * Precipitation frequency is assumed to be hourly for now.
     * TODO: Add input for dt (delta time) for different frequencies in the data than the model frequency.
     * @return the current hourly precipitation in meters per second after pointer is incremented by one timestep
     */
    double get_next_hourly_precipitation_meters_per_second()
    {
        //Increment forcing index
        forcing_vector_index_ptr = forcing_vector_index_ptr + 1;

        //Increment current time by 1 hour
        current_date_time_epoch = current_date_time_epoch + 3600;

        return get_current_hourly_precipitation_meters_per_second();
    }

    /**
     * @brief Gets day of year integer
     * @return day of year integer
     */
    int get_day_of_year()
    {
        int current_day_of_year;

        struct tm *current_date_time;

        current_date_time = localtime(&current_date_time_epoch);
                
        current_day_of_year = current_date_time->tm_yday;    

        return current_day_of_year;
    }

    private:

    /**
     * @brief Read Forcing Data from CSV
     * Reads only data within the specified model start and end date-times and adds to precipiation vector
     * @param file_name Forcing file name
     */
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
                if (start_date_time_epoch <= current_row_date_time_epoch && current_row_date_time_epoch <= end_date_time_epoch)
                {
                    //Precipitation
                    string precip_str = vec[5];
    
                    //Convert from string to double and from mm/hr to m/s
                    double precip = atof(precip_str.c_str()) / (1000 * 3600);

                    //Add precip to vector
                    precipitation_rate_meters_per_second_vector.push_back(precip);
                }

                //Free memory from struct
                delete current_row_date_time;
        }        
    }


    /**
     * @brief Read Forcing Data from AORC CSV
     * Reads only data within the specified model start and end date-times and adds to precipiation vector
     * @param file_name Forcing file name
     */
    void read_forcing_aorc(string file_name)
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
               
                //Declare struct for the current row date-time
                struct tm current_row_date_time;
               
                //Allocate memory to struct for the current row date-time
                current_row_date_time = tm();

                //Grab time string from first column
                string time_str = vec[0];

                //Convert time string to time struct
                strptime(time_str.c_str(), "%Y-%m-%d %H:%M:%S", &current_row_date_time);

                //Convert current row date-time to epoch time
                time_t current_row_date_time_epoch = mktime(&current_row_date_time);

                //If the current row date-time is within the model date-time range, then add precipitation to vector
                if (start_date_time_epoch <= current_row_date_time_epoch && current_row_date_time_epoch <= end_date_time_epoch)
                {
                    //Grab data from columns
                    string APCP_surface_str = vec[1];
                    string DLWRF_surface_str = vec[2];
                    string DSWRF_surface_str = vec[3];
                    string PRES_surface_str = vec[4];
                    string SPFH_2maboveground_str = vec[5];
                    string TMP_2maboveground_str = vec[6];
                    string UGRD_10maboveground_str = vec[7];
                    string VGRD_10maboveground_str = vec[8];
                    string precip_rate_str = vec[9];
    
                    //Convert from strings to doubles
                    double APCP_surface = atof(APCP_surface_str.c_str());
                    double DLWRF_surface = atof(DLWRF_surface_str.c_str());
                    double DSWRF_surface = atof(DSWRF_surface_str.c_str());
                    double PRES_surface = atof(PRES_surface_str.c_str());
                    double SPFH_2maboveground = atof(SPFH_2maboveground_str.c_str());
                    double TMP_2maboveground = atof(TMP_2maboveground_str.c_str());
                    double UGRD_10maboveground = atof(UGRD_10maboveground_str.c_str());
                    double VGRD_10maboveground = atof(VGRD_10maboveground_str.c_str());
                    double precip_rate = atof(precip_rate_str.c_str());

                    //Add data to vectors
                    APCP_surface_kg_per_meters_squared_vector.push_back(APCP_surface);
                    DLWRF_surface_W_per_meters_squared_vector.push_back(DLWRF_surface);
                    DSWRF_surface_W_per_meters_squared_vector.push_back(DSWRF_surface);
                    PRES_surface_Pa_vector.push_back(PRES_surface);
                    SPFH_2maboveground_kg_per_kg_vector.push_back(SPFH_2maboveground);
                    TMP_2maboveground_K_vector.push_back(TMP_2maboveground);
                    UGRD_10maboveground_meters_per_second_vector.push_back(UGRD_10maboveground);
                    VGRD_10maboveground_meters_per_second_vector.push_back(VGRD_10maboveground);
                    precipitation_rate_meters_per_second_vector.push_back(precip_rate);
                }

                //Free memory from struct
                //delete current_row_date_time;
        }    
    }

    vector<double> APCP_surface_kg_per_meters_squared_vector;
    vector<double> DLWRF_surface_W_per_meters_squared_vector;
    vector<double> DSWRF_surface_W_per_meters_squared_vector;
    vector<double> PRES_surface_Pa_vector;
    vector<double> SPFH_2maboveground_kg_per_kg_vector;
    vector<double> TMP_2maboveground_K_vector;
    vector<double> UGRD_10maboveground_meters_per_second_vector;
    vector<double> VGRD_10maboveground_meters_per_second_vector;
    vector<double> precipitation_rate_meters_per_second_vector;
    int forcing_vector_index_ptr; 
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

    /*
    //AORC Forcing Variables
    double APCP_surface; //Total Precipitation (kg/m^2)
    double DLWRF_surface; //Downward Long-Wave Rad. (Flux W/m^2)
    double DSWRF_surface; //Downward Short-Wave Radiation (Flux W/m^2)
    double PRES_surface; //Pressure (Pa)
    double SPFH_2maboveground; //Specific Humidity (kg/kg)
    double TMP_2maboveground; //Temperature (K)
    double UGRD_10maboveground; //U-Component of Wind (m/s)
    double VGRD_10maboveground; //V-Component of Wind (m/s)
    double latitude; //latitude (degrees_north)
    double longitude; //longitude (degrees_east)
    double time; //verification time generated by wgrib2 function verftime() (seconds since 1970-01-01 00:00:00.0 0:00)
    */
};

/// \todo Consider aggregating precipiation data
/// \todo Make CSV forcing a subclass
/// \todo Consider passing grid to class
/// \todo Consider following GDAL API functionality

