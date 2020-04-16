#include <vector>
#include <cmath>
#include <algorithm>
//#include <string>
#include <fstream>
#include <iostream>

using namespace std;


class Forcing
{
    //public:

/*
    void read_forcing(std::string file_name) 
    { 
        // File pointer 
        ifstream file_in; 

        vector<std::vector<std::string> > data_list;

        string line = "";
  
        // Open an existing file 
        file_in.open(file_name, ios::in); 

        while(!file_in.eof())
        {



        }

    }
*/

    private:
    double precipitation_rate_meters_per_second;
    double air_temperature_fahrenheit;
    double basin_latitude;
    int day_of_year;
};
