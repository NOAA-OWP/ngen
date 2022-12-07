#ifndef NGEN_GENRIC_DATA_PROVIDER
#define NGEN_GENRIC_DATA_PROVIDER

#include "DataProvider.hpp"
#include "DataProviderSelectors.hpp"

#include <memory>
#include <netcdf>

namespace data_access
{
    class GenericDataProvider : public DataProvider<double, CatchmentAggrDataSelector>
    {
        public:
        /** Retrun the shared_ptr to the NcFile */
        virtual std::shared_ptr<netCDF::NcFile> get_nc_file() {
            return 0;
        }

        private:
    };
}

#endif
