#ifdef NETCDF_ACTIVE
#ifndef NGEN_NETCDF_CATCHMENT_OUTPUT_WRITER_HPP
#define NGEN_NETCDF_CATCHMENT_OUTPUT_WRITER_HPP

#include <string>
#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <sstream>
#include <exception>
#include <mutex>
#include "assert.h"
#include <iomanip>
#include <boost/compute/detail/lru_cache.hpp>
#include <boost/filesystem/path.hpp>
#include <iostream>

#include <UnitsHelper.hpp>
#include <StreamHandler.hpp>

#include <netcdf>


using namespace netCDF;
using namespace netCDF::exceptions;

using boost::filesystem::path;

namespace data_output
{

    class NetcdfOutputWriter
    {
        public:
        
        NetcdfOutputWriter(path output_file) : netcdfFile()
        {
            open(output_file);
        } 

        virtual ~NetcdfOutputWriter() {}

        virtual void open(path output_file)
        {
            std::cerr << "Creating output file " << output_file.string() << "\n";
            netcdfFile = std::make_shared<NcFile>(output_file.string(), NcFile::replace, NcFile::nc4);
        }

        protected:

        std::shared_ptr<NcFile> netcdfFile;

    };
    
    class NetcdfCatchmentOutputWriter : public NetcdfOutputWriter
    {
        public:
        
        NetcdfCatchmentOutputWriter(path output_file) : NetcdfOutputWriter(output_file)
        {

        }

        void open(path output_file)
        {
            NetcdfOutputWriter::open(output_file);

            netcdfFile->addDim("id");
            netcdfFile->addDim("time");
        }
    };

}

#endif  //NGEN_NETCDF_CATCHMENT_OUTPUT_WRITER_HPP

#endif // NETCDF_ACTIVE
