#ifndef NGEN_GENRIC_DATA_PROVIDER
#define NGEN_GENRIC_DATA_PROVIDER

#include "DataProvider.hpp"
#include "DataProviderSelectors.hpp"

#ifdef NETCDF_ACTIVE
#include <memory>
#include <netcdf>
#endif

namespace data_access
{
    class GenericDataProvider : public DataProvider<double, CatchmentAggrDataSelector>
    {
        public:

  #ifdef NETCDF_ACTIVE
        /** Retrun the shared_ptr to the NcFile */
        virtual std::shared_ptr<netCDF::NcFile> get_nc_file() {
            return 0;
        }
  #endif

        private:
    };
}

#endif //NGEN_GENRIC_DATA_PROVIDER
