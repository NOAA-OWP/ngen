#ifdef NETCDF_ACTIVE
#ifndef NGEN_NETCDF_OUTPUT_WRITER_HPP
#define NGEN_NETCDF_OUTPUT_WRITER_HPP

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
#include <unordered_map>

#include <UnitsHelper.hpp>
#include <StreamHandler.hpp>

#include <netcdf>


using namespace netCDF;
using namespace netCDF::exceptions;

using boost::filesystem::path;

namespace data_output
{

    struct NetcdfDimensionDiscription
    {
        NetcdfDimensionDiscription() {} 
        NetcdfDimensionDiscription(std::string n, long s = 0) : name(n), size(s) {}
        
        std::string name;
        unsigned long size;             // use size zero for scalar 
    };

    struct NetcdfVariableDiscription
    {
        NetcdfVariableDiscription() {}
        NetcdfVariableDiscription(const std::string& n, const NcType& t) : name(n), type(t), dim_names() {}
        NetcdfVariableDiscription(const std::string& n, const NcType& t, const std::string& d_n ) : name(n), type(t), dim_names() { dim_names.push_back(d_n); }
        NetcdfVariableDiscription(const std::string& n, const NcType& t, const std::vector<std::string>& d_n ) : name(n), type(t), dim_names(d_n) {}

        
        std::string name;
        NcType type;
        std::vector<std::string> dim_names;
    };
    
    class NetcdfOutputWriter
    {
        public:
        
        NetcdfOutputWriter(path output_file) : netcdfFile()
        {
            open(output_file);
        }

        NetcdfOutputWriter(path output_file, 
            std::vector<NetcdfDimensionDiscription>& dimensions,
            std::vector<NetcdfVariableDiscription>& variables) : netcdfFile()
        {
            open(output_file);

            // create the dimensions
            for ( auto& dim : dimensions )
            {
                if ( dim.size > 0 )
                {
                    netcdfDims[dim.name] = netcdfFile->addDim(dim.name, dim.size);
                }
                else
                {
                    netcdfDims[dim.name] = netcdfFile->addDim(dim.name);
                }
            }

            // create the variables
            for( auto& var : variables )
            {
                std::vector<netCDF::NcDim> var_dims;

                if (var.dim_names.size() > 0 )
                {
                    for ( auto & name : var.dim_names )
                    {
                        var_dims.push_back(netcdfDims[name]);
                    }

                    netcdfVars[var.name] = netcdfFile->addVar(var.name, var.type, var_dims);
                }
                else
                {
                    netcdfVars[var.name] = netcdfFile->addVar(var.name, var.type);
                } 
            }
        }  

        virtual ~NetcdfOutputWriter() {}

        virtual void open(path output_file)
        {
            std::cerr << "Creating output file " << output_file.string() << "\n";
            netcdfFile = std::make_shared<NcFile>(output_file.string(), NcFile::replace, NcFile::nc4);
        }

        virtual std::shared_ptr<NcFile>& ncfile()
        {
            return netcdfFile;
        }

        protected:

        std::shared_ptr<NcFile> netcdfFile;
        std::unordered_map<std::string, netCDF::NcDim> netcdfDims;
        std::unordered_map<std::string, netCDF::NcVar> netcdfVars;

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
