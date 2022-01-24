#ifndef NGEN_NETCDF_PER_FEATURE_DATAPROVIDER_HPP
#define NGEN_NETCDF_PER_FEATURE_DATAPROVIDER_HPP

#include "DataProvider.hpp"

namespace data_access
{
    class Catchment_Id
    {
        public:

        Catchment_Id(std::string id) : id_str(id) {}

        std::string operator std::string() const { return id_str}

        private:

        std::string id_str;
    };

    class NetCDFPerFeatureDataProvider : public DataProvider<double, Catchment_Id>
    {

    };
}


#endif // NGEN_NETCDF_PER_FEATURE_DATAPROVIDER_HPP
