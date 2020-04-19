#include <vector>
#include <cmath>
#include <algorithm>
#include <string>
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
    //void read_forcing(std::string file_name)
    void read_forcing(string file_name)    //builds fine
    { 
        //CSVReader(filename = file_name);

        //CSVReader("file_name");  //Builds fine

        CSVReader reader(file_name);

	// Get the data from CSV File
	std::vector<std::vector<std::string> > data_list = reader.getData();


	// Print the content of row by row on screen
	for(std::vector<std::string> vec : data_list)
	{
		for(std::string data : vec)
		{
			std::cout<<data << " , ";
		}
		std::cout<<std::endl;
	}

        cout << "hi";


    }


    private:
    double precipitation_rate_meters_per_second;
    double air_temperature_fahrenheit;
    double basin_latitude;
    int day_of_year;
};
