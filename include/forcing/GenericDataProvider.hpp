#ifndef NGEN_GENRIC_DATA_PROVIDER
#define NGEN_GENRIC_DATA_PROVIDER
#include <NGenConfig.h>

#include "DataProvider.hpp"
#include "DataProviderSelectors.hpp"
#include <memory>
#if NGEN_WITH_NETCDF
  #include <netcdf>
#endif

namespace netCDF {
    class NcFile;
}

namespace data_access
{
    class GenericDataProvider : public DataProvider<double, CatchmentAggrDataSelector>
    {
        public:

#if NGEN_WITH_NETCDF
        /** Retrun the shared_ptr to the NcFile */
        virtual std::shared_ptr<netCDF::NcFile> get_nc_file() {
            return nullptr;
        }
#endif

        private:
    };
}

#endif
