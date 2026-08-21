#ifndef CSV_Reader_H
#define CSV_Reader_H
#include "Logger.hpp"

#include <fstream>
#include <vector>
#include <iterator>
#include <string>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <cerrno>

/*
 * @brief A class to read data from a csv file.
 */
class CSVReader
{
    std::string fileName;
    std::string delimeter;

public:
    CSVReader(std::string filename, std::string delm = ",") :
            fileName(filename), delimeter(delm)
    { }

    // Function to fetch data from a CSV File
    std::vector<std::vector<std::string> > getData();
};

/*
* Parses through csv file line by line and returns the data
* in vector of vector of strings.
*/
inline std::vector<std::vector<std::string> > CSVReader::getData()
{
    errno = 0;
    std::ifstream file(fileName);

    if (file.fail()) {
        std::string throw_msg; throw_msg.assign(
                errno == 0
                    ? "Error: failure opening " + fileName
                    : "Errno " + std::to_string(errno) + " (" + strerror(errno) + ") opening " + fileName
        );
        LOG(throw_msg, LogLevel::WARNING);
        throw std::runtime_error(throw_msg);

        /// \todo Potentially only output warning and fill array with sentinel values.
    }

    std::vector<std::vector<std::string> > dataList;

    std::string line = "";
    // Iterate through each line and split the content using delimeter
    while (getline(file, line))
    {
        // Consider more robust solution like https://stackoverflow.com/a/6089413/489116
        if ( line.size() && line[line.size()-1] == '\r' ) {
           line = line.substr( 0, line.size() - 1 );
        }

        std::vector<std::string> vec;

                /// \todo Look into replacement from STD for split to reduce dependency on Boost
        boost::algorithm::split(vec, line, boost::is_any_of(delimeter));
        dataList.push_back(vec);
    }
    // Close the File
    file.close();

    return dataList;
}

#endif //CSV_Reader_H
