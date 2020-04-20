#include <vector>
#include <cmath>
#include <algorithm>
#include <string>
#include <fstream>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include "CSV_Reader.h"


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

    //Default Constructor
    Forcing(): air_temperature_fahrenheit(0.0), basin_latitude(0.0), day_of_year(0)
    {

    }

    //Parameterized Constructor
    Forcing(double air_temperature_fahrenheit, double basin_latitude, int day_of_year): air_temperature_fahrenheit(air_temperature_fahrenheit), basin_latitude(basin_latitude), day_of_year(day_of_year)
    {

    }

    //Read Forcing Data from CSV
    //void read_forcing(std::string file_name)
    void read_forcing(string file_name)    //builds fine
    { 

        CSVReader reader(file_name);

	// Get the data from CSV File
	std::vector<std::vector<std::string> > data_list = reader.getData();

        vector<double> precipitation_vector_meters_per_second;

        std::string::size_type sz;     // alias of size_t

        // Print the content of row by row on screen
        for(std::vector<std::string> vec : data_list)
        {

                string precip_str = vec[5];

                //Convert from string to double and from mm/hr to m/s
                double precip = atof(precip_str.c_str()) / (1000 * 3600);

                precipitation_vector_meters_per_second.push_back(precip);

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


    private:

    double precipitation_rate_meters_per_second;
    double air_temperature_fahrenheit;
    double basin_latitude;
    int day_of_year;
};
