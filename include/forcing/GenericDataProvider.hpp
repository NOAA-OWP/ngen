#ifndef NGEN_GENRIC_DATA_PROVIDER
#define NGEN_GENRIC_DATA_PROVIDER

#include "DataProvider.hpp"
#include "DataProviderSelectors.hpp"
#include "MeshPointsSelectors.hpp"

namespace data_access
{
    using GenericDataProvider = DataProvider<double, CatchmentAggrDataSelector>;
    using MeshPointsDataProvider = DataProvider<double, MeshPointsSelector>;
}

#endif
