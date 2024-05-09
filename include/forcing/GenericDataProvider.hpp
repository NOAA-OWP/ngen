#ifndef NGEN_GENRIC_DATA_PROVIDER
#define NGEN_GENRIC_DATA_PROVIDER

#include "DataProvider.hpp"
#include "DataProviderSelectors.hpp"

namespace data_access
{
    using GenericDataProvider = DataProvider<double, CatchmentAggrDataSelector>;
}

#endif
