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
 * @brief forcing_params providing configuration information for forcing time period and source.
 */
struct forcing_params
{
  std::string path;
  std::string start_time;
  std::string end_time;
  std::string date_format =  "%Y-%m-%d %H:%M:%S";
  time_t start_t;
  time_t end_t;
  /*
    Constructor for forcing_params
  */
  forcing_params(std::string path, std::string start_time, std::string end_time):
    path(path), start_time(start_time), end_time(end_time)
    {
      /// \todo converting to UTC can be tricky, especially if thread safety is a concern
      /* https://stackoverflow.com/questions/530519/stdmktime-and-timezone-info */
      struct tm tm;
      strptime(this->start_time.c_str(), this->date_format.c_str() , &tm);
      //mktime returns time in local time based on system timezone
      //FIXME use timegm (not standard)? or implement timegm (see above link)
      this->start_t = timegm( &tm );

      strptime(this->end_time.c_str(), this->date_format.c_str() , &tm);
      this->end_t = timegm( &tm );
    }
};

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

    Forcing(forcing_params forcing_config):start_date_time_epoch(forcing_config.start_t),
                                           end_date_time_epoch(forcing_config.end_t),
                                           current_date_time_epoch(forcing_config.start_t),
                                           forcing_vector_index(-1)
    {
        read_forcing_aorc(forcing_config.path);
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
        forcing_vector_index = 0;
    }

    /**
     * @brief Checks forcing vector index bounds and adjusts index if out of vector bounds
     */
    void check_forcing_vector_index_bounds()
    {
        //Check if forcing index is less than zero and if so, set to zero.
        if (forcing_vector_index < 0)
        {
            forcing_vector_index = 0;
            /// \todo: Return appropriate warning
            cout << "WARNING: Forcing vector index is less than zero. Therefore, setting index to zero." << endl;
        }

        //Check if forcing index is greater than or equal to the size of the size of the precipiation vector and if so, set to zero.
        else if (forcing_vector_index >= precipitation_rate_meters_per_second_vector.size())
        {
            forcing_vector_index = precipitation_rate_meters_per_second_vector.size() - 1;
            /// \todo: Return appropriate warning
            cout << "WARNING: Reached beyond the size of the forcing vector. Therefore, setting index to last value of the vector." << endl;
        }

        return;
    }

    /**
     * @brief Gets current hourly precipitation in meters per second
     * Precipitation frequency is assumed to be hourly for now.
     * TODO: Add input for dt (delta time) for different frequencies in the data than the model frequency.
     * @return the current hourly precipitation in meters per second
     */
    double get_current_hourly_precipitation_meters_per_second()
    {
        check_forcing_vector_index_bounds();

        return precipitation_rate_meters_per_second_vector.at(forcing_vector_index);
    }

    /**
     * @brief Gets next hourly precipitation in meters per second
     * Increments pointer in forcing vector by one timestep
     * Precipitation frequency is assumed to be hourly for now.
     * /// \todo: Add input for dt (delta time) for different frequencies in the data than the model frequency.
     * /// \todo: Reconsider incrementing the forcing_vector_index because other functions rely on this, and it
     *            could have side effects
     * @return the current hourly precipitation in meters per second after pointer is incremented by one timestep
     */
    double get_next_hourly_precipitation_meters_per_second()
    {
        //Check forcing vector bounds before incrementing forcing index
        if (forcing_vector_index < precipitation_rate_meters_per_second_vector.size() - 1)
            //Increment forcing index
            forcing_vector_index = forcing_vector_index + 1;

        else
            /// \todo: Return appropriate warning
            cout << "WARNING: Reached beyond the size of the forcing precipitation vector. Therefore, returning the last precipitation value of the vector." << endl;

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

        check_forcing_vector_index_bounds();

        current_date_time_epoch = time_epoch_vector.at(forcing_vector_index);

        /// \todo: Sort out using local versus UTC time
        current_date_time = localtime(&current_date_time_epoch);

        current_day_of_year = current_date_time->tm_yday;

        return current_day_of_year;
    }

    /**
     * @brief Accessor to time epoch
     * @return current_date_time_epoch
     */
    double get_time_epoch()
    {
        check_forcing_vector_index_bounds();

        return current_date_time_epoch = time_epoch_vector.at(forcing_vector_index);
    };

    /**
     * @brief Accessor to AORC struct
     * @return AORC_data
     */
    AORC_data get_AORC_data()
    {
        check_forcing_vector_index_bounds();

        return AORC_vector.at(forcing_vector_index);
    };

    /**
     * @brief Accessor to AORC APCP_surface_kg_per_meters_squared
     * @return APCP_surface_kg_per_meters_squared
     */
    double get_AORC_APCP_surface_kg_per_meters_squared()
    {
        check_forcing_vector_index_bounds();

        return AORC_vector.at(forcing_vector_index).APCP_surface_kg_per_meters_squared;
    };

    /**
     * @brief Accessor to AORC DLWRF_surface_W_per_meters_squared
     * @return DLWRF_surface_W_per_meters_squared
     */
    double get_AORC_DLWRF_surface_W_per_meters_squared()
    {
        check_forcing_vector_index_bounds();

        return AORC_vector.at(forcing_vector_index).DLWRF_surface_W_per_meters_squared;
    };

    /**
     * @brief Accessor to AORC DSWRF_surface_W_per_meters_squared
     * @return DSWRF_surface_W_per_meters_squared
     */
    double get_AORC_DSWRF_surface_W_per_meters_squared()
    {
        check_forcing_vector_index_bounds();

        return AORC_vector.at(forcing_vector_index).DSWRF_surface_W_per_meters_squared;
    };

    /**
     * @brief Accessor to AORC PRES_surface_Pa
     * @return PRES_surface_Pa
     */
    double get_AORC_PRES_surface_Pa()
    {
        check_forcing_vector_index_bounds();

        return AORC_vector.at(forcing_vector_index).PRES_surface_Pa;
    };

    /**
     * @brief Accessor to AORC SPFH_2maboveground_kg_per_kg
     * @return SPFH_2maboveground_kg_per_kg
     */
    double get_AORC_SPFH_2maboveground_kg_per_kg()
    {
        check_forcing_vector_index_bounds();

        return AORC_vector.at(forcing_vector_index).SPFH_2maboveground_kg_per_kg;
    };

    /**
     * @brief Accessor to AORC TMP_2maboveground_K
     * @return TMP_2maboveground_K
     */
    double get_AORC_()
    {
        check_forcing_vector_index_bounds();

        return AORC_vector.at(forcing_vector_index).TMP_2maboveground_K;
    };

    /**
     * @brief Accessor to AORC UGRD_10maboveground_meters_per_second
     * @return UGRD_10maboveground_meters_per_second
     */
    double get_AORC_UGRD_10maboveground_meters_per_second()
    {
        check_forcing_vector_index_bounds();

        return AORC_vector.at(forcing_vector_index).UGRD_10maboveground_meters_per_second;
    };

    /**
     * @brief Accessor to AORC VGRD_10maboveground_meters_per_second
     * @return VGRD_10maboveground_meters_per_second
     */
    double get_AORC_VGRD_10maboveground_meters_per_second()
    {
        check_forcing_vector_index_bounds();

        return AORC_vector.at(forcing_vector_index).VGRD_10maboveground_meters_per_second;
    };


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

                //Declare pointer to struct for the current row date-time utc
                struct tm *current_row_date_time_utc;

                //Allocate memory to struct for the current row date-time utc
                current_row_date_time_utc = new tm();

                //Year
                string year_str = vec[0];
                int year = stoi(year_str);
                current_row_date_time_utc->tm_year = year - 1900;

                //Month
                string month_str = vec[1];
                int month = stoi(month_str);
                current_row_date_time_utc->tm_mon = month - 1;

                //Day
                string day_str = vec[2];
                int day = stoi(day_str);
                current_row_date_time_utc->tm_mday = day;

                //Hour
                string hour_str = vec[3];
                int hour = stoi(hour_str);
                current_row_date_time_utc->tm_hour = hour;

                //Convert current row date-time utc to epoch time
                time_t current_row_date_time_epoch = timegm(current_row_date_time_utc);

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
                delete current_row_date_time_utc;
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

        //Iterate through CSV starting on the second row
        for (int i = 1; i < data_list.size(); i++)
        {
                //Row vector
                std::vector<std::string>& vec = data_list[i];

                //Declare struct for the current row date-time 
                struct tm current_row_date_time_utc;

                //Allocate memory to struct for the current row date-time
                current_row_date_time_utc = tm();

                //Grab time string from first column
                string time_str = vec[0];

                //Convert time string to time struct
                strptime(time_str.c_str(), "%Y-%m-%d %H:%M:%S", &current_row_date_time_utc);

                //Convert current row date-time UTC to epoch time
                time_t current_row_date_time_epoch = timegm(&current_row_date_time_utc);

                //Ensure that forcing data covers the entire model period. Otherwise, throw an error.
                if (i == 1 && start_date_time_epoch < current_row_date_time_epoch)
                    /// \todo TODO: Return appropriate error
                    //cout << "WARNING: Forcing data begins after the model start time." << endl;
                    throw std::runtime_error("Error: Forcing data begins after the model start time.");


                else if (i == data_list.size() - 1 && current_row_date_time_epoch < end_date_time_epoch)
                    /// \todo TODO: Return appropriate error
                    cout << "WARNING: Forcing data ends before the model end time." << endl;
                    //throw std::runtime_error("Error: Forcing data ends before the model end time.");

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

                    //Declare AORC struct
                    AORC_data AORC;

                    //Convert from strings to doubles and add to AORC struct
                    AORC.APCP_surface_kg_per_meters_squared = atof(APCP_surface_str.c_str());
                    AORC.DLWRF_surface_W_per_meters_squared = atof(DLWRF_surface_str.c_str());
                    AORC.DSWRF_surface_W_per_meters_squared = atof(DSWRF_surface_str.c_str());
                    AORC.PRES_surface_Pa = atof(PRES_surface_str.c_str());
                    AORC.SPFH_2maboveground_kg_per_kg = atof(SPFH_2maboveground_str.c_str());
                    AORC.TMP_2maboveground_K = atof(TMP_2maboveground_str.c_str());
                    AORC.UGRD_10maboveground_meters_per_second = atof(UGRD_10maboveground_str.c_str());
                    AORC.VGRD_10maboveground_meters_per_second = atof(VGRD_10maboveground_str.c_str());
                  
                    //Add AORC struct to AORC vector
                    AORC_vector.push_back(AORC);
            
                    //Convert precip_rate from string to double
                    double precip_rate = atof(precip_rate_str.c_str());

                    //Add data to vectors
                    precipitation_rate_meters_per_second_vector.push_back(precip_rate);
                    time_epoch_vector.push_back(current_row_date_time_epoch);
                }

                //Free memory from struct
                //delete current_row_date_time_utc;
        }
    }

    //AORC Forcing Data Struct
    struct AORC_data
    {
        double APCP_surface_kg_per_meters_squared; //Total Precipitation (kg/m^2)
        double DLWRF_surface_W_per_meters_squared; //Downward Long-Wave Rad. (Flux W/m^2)
        double DSWRF_surface_W_per_meters_squared; //Downward Short-Wave Radiation (Flux W/m^2)
        double PRES_surface_Pa; //Pressure (Pa)
        double SPFH_2maboveground_kg_per_kg; //Specific Humidity (kg/kg)
        double TMP_2maboveground_K; //Temperature (K)
        double UGRD_10maboveground_meters_per_second; //U-Component of Wind (m/s)
        double VGRD_10maboveground_meters_per_second; //V-Component of Wind (m/s)
    };

    vector<AORC_data> AORC_vector;

    vector<double> precipitation_rate_meters_per_second_vector;

    /// \todo: Consider making epoch time the iterator
    vector<time_t> time_epoch_vector;     
    int forcing_vector_index;
    double precipitation_rate_meters_per_second;
    double air_temperature_fahrenheit;
    double latitude; //latitude (degrees_north)
    double longitude; //longitude (degrees_east)
    int basin_id;
    int day_of_year;
    string forcing_file_name;

    std::shared_ptr<time_type> start_date_time;
    std::shared_ptr<time_type> end_date_time;

    time_t start_date_time_epoch;
    time_t end_date_time_epoch;
    time_t current_date_time_epoch;
};

/// \todo Consider aggregating precipiation data
/// \todo Make CSV forcing a subclass
/// \todo Consider passing grid to class
/// \todo Consider following GDAL API functionality
