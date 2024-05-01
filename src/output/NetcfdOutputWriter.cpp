#ifdef NETCDF_ACTIVE

#include "NetcdfOutputWriter.hpp"

#include <boost/algorithm/string/case_conv.hpp>

namespace data_output
{
    NcType strtonctype(const std::string& s)
    {
        std::string string_rep = boost::to_lower_copy(s);
        if (  string_rep == "byte" || string_rep == "nc_byte")
        {
            return NcType(NC_BYTE);
        }
        else if ( string_rep == "char" || string_rep == "nc_char")
        {
            return NcType(NC_CHAR);
        }
        else if ( string_rep == "short" || string_rep == "nc_short")
        {
            return NcType(NC_SHORT);
        }
        else if ( string_rep == "int" || string_rep == "nc_int")
        {
            return NcType(NC_INT);
        }
        else if ( string_rep == "float" || string_rep == "nc_float")
        {
            return NcType(NC_FLOAT);
        }
        else if ( string_rep == "double" || string_rep == "nc_double")
        {
            return NcType(NC_FLOAT);
        }
        else if ( string_rep == "ubyte" || string_rep == "nc_ubyte")
        {
            return NcType(NC_UBYTE);
        }
        else if ( string_rep == "ushort" || string_rep == "nc_ushort")
        {
            return NcType(NC_USHORT);
        }
        else if ( string_rep == "uint" || string_rep == "nc_uint")
        {
            return NcType(NC_UINT);
        }
        else if ( string_rep == "int64" || string_rep == "nc_int64")
        {
            return NcType(NC_INT64);
        }
        else if ( string_rep == "uint64" || string_rep == "nc_uint64")
        {
            return NcType(NC_UINT64);
        }
        else if ( string_rep == "string" || string_rep == "nc_string")
        {
            return NcType(NC_STRING);
        }
        else if ( string_rep == "vlen" || string_rep == "nc_vlen")
        {
            return NcType(NC_VLEN);
        }
        else if ( string_rep == "opaque" || string_rep == "nc_opaque")
        {
            return NcType(NC_OPAQUE);
        }
        else if ( string_rep == "enum" || string_rep == "nc_enum")
        {
            return NcType(NC_ENUM);
        }
        else if ( string_rep == "compound" || string_rep == "nc_compound")
        {
            return NcType(NC_COMPOUND);
        }
        else
        {
            return NcType();
        }       
    }

}

#endif