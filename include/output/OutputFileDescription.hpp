#ifndef __OUTPUT_FILE_DESCRIPTION_H__
#define __OUTPUT_FILE_DESCRIPTION_H__

#include <string>
#include <vector>

#include "output/NetcdfOutputWriter.hpp"

namespace data_output
{
    enum FileOutputType { NetCDF4 = 1, CSV =2/*,JSON=3*/};

    struct NetCDFFileInfo
    {
        public:

        std::vector<NetcdfDimensionDiscription> dimensions;
        std::vector<NetcdfVariableDiscription> variables;
    };

    struct CSVFileInfo
    {
        public:

        std::vector<std::string> column_names;
        std::vector<std::string> column_types;
    };
    
    struct OutputFileDescription
    {
        public:

        std::string file_name;
        FileOutputType file_type;
        NetCDFFileInfo nc_info;
        CSVFileInfo csv_info;      
    };
}

#endif // __OUTPUT_FILE_DESCRIPTION_H__