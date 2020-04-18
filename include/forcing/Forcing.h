#include <vector>
#include <cmath>
#include <algorithm>
//#include <string>
#include <fstream>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include "CSV_Reader.h"

using namespace std;
//using namespace boost;


class Forcing
{
    public:

    //Default Constructor
    Forcing(): air_temperature_fahrenheit(0.0), basin_latitude(0.0), day_of_year(0)
    {

    }

    //Parameterized Constructor
    Forcing(double air_temperature_fahrenheit, double basin_latitude, int day_of_year): air_temperature_fahrenheit(air_temperature_fahrenheit), basin_latitude(basin_latitude), day_of_year(day_of_year)
    {

    }

    //Read Forcing Data from CSV
    void read_forcing(std::string file_name) 
    { 
        CSVReader("name.csv");


    }


    private:
    double precipitation_rate_meters_per_second;
    double air_temperature_fahrenheit;
    double basin_latitude;
    int day_of_year;
};
