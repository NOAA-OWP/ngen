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

    /** \brief This class provides a description of a netcdf dimension including name and type */

    struct NetcdfDimensionDiscription
    {
        NetcdfDimensionDiscription() {} 
        NetcdfDimensionDiscription(std::string n, long s = 0) : name(n), size(s) {}
        
        std::string name;
        unsigned long size;             // use size zero for scalar 
    };

    /*** \breif this function returns NcType objects from string type names if the name is not know ncType() is returned. */
    NcType strtonctype(const std::string& s);
   

    /** \brief This class provides a discription of a netcdf variable including variable type and dimensions */

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

    /** \brief This a wrapper class for a std::vector<std::size_t>. It is used to indicate that this array is
     * to be used for a offset vector.
     */

    class NetcdfOutputWriterOffset
    {
        public:

        NetcdfOutputWriterOffset(std::vector<std::size_t>&& v) : m_offset(v) {}
        NetcdfOutputWriterOffset(std::vector<std::size_t>& v) : m_offset() { m_offset.swap(v); }

        std::vector<std::size_t>& data() { return m_offset; }
        const std::vector<std::size_t>& data() const { return m_offset; }

        std::vector<std::size_t>& operator()() { return data(); }
        const std::vector<std::size_t>& operator()() const { return data(); }

        private:

        std::vector<std::size_t> m_offset;
    };

    /** \brief This a wrapper class for a std::vector<std::size_t>. It is used to indicate that this array is
     * to be used for a stride vector.
     */

    class NetcdfOutputWriterStride
    {
        public:

        NetcdfOutputWriterStride(std::vector<std::size_t>&& v) : m_stride(v) {}
        NetcdfOutputWriterStride(std::vector<std::size_t>& v) : m_stride() { m_stride.swap(v); }

        std::vector<std::size_t>& data() { return m_stride; }
        const std::vector<std::size_t>& data() const { return m_stride; }

        std::vector<std::size_t>& operator()() { return data(); }
        const std::vector<std::size_t>& operator()() const { return data(); }

        private:

        std::vector<std::size_t> m_stride;
    };
    
    /***
     * \brief The NetcdfOutputWriter allows the creation of netcdf files from a list of dimensions descriptions and a list of variables dimensions.
     * 
     * The NetcdfOutputWriter allows the creation of netcdf files from a list of dimensions descriptions and a list of variables dimensions.
     * Helper class and stream operators are defined to allow easy data output.
     * 
     * Example:
     *      <NetcdfOutputWritter>["var1"] << nc_stride(0,10,20) << nc_stride(10,10,10) << <vector-data>
     * 
     *      The above call would write the contents of <vector-data> into a 10x10x10 region in the the variable "var1"
     *      with data input starting at position [0,10,20]
     * 
    */

    class NetcdfOutputWriter
    {
        public:
        
        NetcdfOutputWriter(path output_file) : netcdfFile()
        {
            open(output_file);
        }

        /** \brief Open this writter with a new output path. This replaces any existing file at the same location. 
         *  The variables and dimensions are setup according the to the contents of the input lists.
        */

        NetcdfOutputWriter(path output_file, 
            std::vector<NetcdfDimensionDiscription>& dimensions,
            std::vector<NetcdfVariableDiscription>& variables) : netcdfFile()
        {
            open(output_file, dimensions, variables);
        }  

        virtual ~NetcdfOutputWriter() {}

        /** \brief Open this writter with a new output path. This replaces any existing file at the same location.*/

        virtual void open(path output_file)
        {
            std::cerr << "Creating output file " << output_file.string() << "\n";
            netcdfFile = std::make_shared<NcFile>(output_file.string(), NcFile::replace, NcFile::nc4);
        }

        /** \brief Open this writter with a new output path. This replaces any existing file at the same location. 
         * The file will be populated with variables and dimension as indicated by the dimension and variable 
         * descriptions. */

        virtual void open(path output_file,
            std::vector<NetcdfDimensionDiscription>& dimensions,
            std::vector<NetcdfVariableDiscription>& variables)
        {
            std::cerr << "Creating output file " << output_file.string() << "\n";
            netcdfFile = std::make_shared<NcFile>(output_file.string(), NcFile::replace, NcFile::nc4);

            netcdfDims.clear();
            netcdfVars.clear();

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

        /** \brief return a shared pointer to the C++ API NcFile object */

        virtual std::shared_ptr<NcFile>& ncfile()
        {
            return netcdfFile;
        }

        /** \brief This is a helper class to allow the use off stream opperators for netcdf file output.
         * 
         * This is a helper class that allows a NetcdfOutputWriter class to be used for data output using a combination of 
         * indexing by variable name and two special i/o manipulator functions nc_offset() and nc_stride().
         * 
         * Usage:
         *      
         *      <NetcdfOutputWritter>["var1"] << nc_stride(0,10,20) << nc_stride(10,10,10) << <vector-data>
         * 
         *      The above call would write the contents of <vector-data> into a 10x10x10 region in the the variable "var1"
         *      with data input starting at position [0,10,20]
         * 
        */

        class NetcdfOutputWriterHelper
        {
            public:

            /** \brief Bind a writer object and variable name to create the helper object. */

            NetcdfOutputWriterHelper(NetcdfOutputWriter& w, const std::string s) : writer(w), var(s) 
            {
                
            }

            /** \brief process streamed data of type <NetcdfOutputWriterOffset> this changes the current offset position. */

            NetcdfOutputWriterHelper& operator<<(NetcdfOutputWriterOffset new_offset)
            {
                offset.swap(new_offset.data());

                return *this;
            }

            /** \brief process streamed data of type <NetcdfOutputWriterStride> this changes the current stride size and dimensions. */

            NetcdfOutputWriterHelper& operator<<(NetcdfOutputWriterStride new_stride)
            {
                stride.swap(new_stride.data());

                return *this;
            }

            /** \brief attempt to use the data proved by pointer for a write operation with the current offset and stride */

            template <class T> NetcdfOutputWriterHelper& operator<<(const T* d)
            {

                writer.netcdfVars[var].putVar(offset, stride, d);

                return *this;
            }

            /** \brief attempt to use the data proved by vector for a write operation with the current offset and stride */

            template <class T> NetcdfOutputWriterHelper& operator<<(std::vector<T>& d)
            {

                writer.netcdfVars[var].putVar(offset, stride, &d[0]);

                return *this;
            }

            private:

            NetcdfOutputWriter& writer;             //!< the writer object
            const std::string& var;                 //!< the variable where data will be written
            std::vector<std::size_t> offset;        //!< the current offset to be used durring writing
            std::vector<std::size_t> stride;        //!< the current stride for the next data write
        };

        // let the helper class access private members 
        friend class NetcdfOutputWriterHelper;

        /** \brief construct a helper object to access the requested variable */

        NetcdfOutputWriterHelper operator[](const std::string var)
        {
            return NetcdfOutputWriterHelper(*this,var);
        }

        protected:

        std::shared_ptr<NcFile> netcdfFile;                         //!< the netcdf file reference
        std::unordered_map<std::string, netCDF::NcDim> netcdfDims;  //!< a map of dimension names to dimension references
        std::unordered_map<std::string, netCDF::NcVar> netcdfVars;  //!< a map of variable names to variable references

    };

    /**  \brief Helper function to create a NetcdfOutputWriterOffset object from a sequence of integers
     *
     *  This is a convience function to construct a NetcdfOutputWriterStride object in a similar manner as tranditional 
     *  I/O stream manipulators. This allows the stride of the next netcdf output operation to be adjusted with an
     *  expression that is part of sequence of output stream operations ussing '<<'.
     * 
     *  Examples
     *  nc_offset(1) // the next output is one dimensional stating at position [0]
     *  nc_offset(100) // the next output is one dimensional starting at position [100]
     *  nc_offset(10,5) // the next output is two dimensional starting at position [10,5]
     *  nc_offset(0,10) // the next output is two dimensional starting at position [0,10]
     *  nc_offset(100,100,200) // the next output is three dimensional starting at position[100,100,200]
     *
     */

    template<class... Types> NetcdfOutputWriterOffset nc_offset(Types... args)
    {
        std::size_t length_of_arg_list = sizeof...(args);               // get the size of the arguement list
        std::vector<std::size_t> offset_vector(length_of_arg_list);     // create a vector the correct size

        std::size_t position = 0;
        for( auto v: {args...} )                                        // iterate through the arguments
        {
            offset_vector[position++] = v;                              // copy arg values to vector
        }

        return NetcdfOutputWriterOffset(std::move(offset_vector));      // Make the helper object
    }

    /*!  \brief Helper function to create a NetcdfOutputWriterStride object from a sequence of integers
     *
     *  This is a convience function to construct a NetcdfOutputWriterStride object in a similar manner as tranditional 
     *  I/O stream manipulators. This allows the stride of the next netcdf output operation to be adjusted with an
     *  expression that is part of sequence of output stream operations ussing '<<'.
     * 
     *  Examples
     *  [NetcdfOutputWriter] << nc_offset(0) << nc_stride(1) << [std::vector<type>>]; // the next output is one dimensional with a size of 1
     *  nc_stride(100) // the next output is one dimensional with a size of 100
     *  nc_stride(1,2) // the next output is two dimensional with a size of 1, in the first variable dimension and a size of 2 in the second dimension
     *  nc_stride(10,10) // the next output is two dimensional with a size of 1o in both dimensions
     *  nc_stride(100,100,200) // the next output is three dimensional with a size of 100, 100, and 200 in the three variable dimensions
     *
     */

    template<class... Types> NetcdfOutputWriterStride nc_stride(Types... args)
    {
        std::size_t length_of_arg_list = sizeof...(args);               // get the size of the arguement list
        std::vector<std::size_t> stride_vector(length_of_arg_list);     // create a vector the correct size

        std::size_t position = 0;
        for( auto positional_arg: {args...} )                           // iterate through the arguments
        {
            stride_vector[position++] = positional_arg;                 // copy arg values to vector
        }

        return NetcdfOutputWriterStride(std::move(stride_vector));      // return the helper object
    }

}

#endif  //NGEN_NETCDF_CATCHMENT_OUTPUT_WRITER_HPP

#endif // NETCDF_ACTIVE
