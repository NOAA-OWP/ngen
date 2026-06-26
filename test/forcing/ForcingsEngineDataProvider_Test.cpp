#include <forcing/ForcingsEngineDataProvider.hpp>

#include "DataProviderSelectors.hpp"
#include "GriddedDataSelector.hpp"

#include "ForcingsEngineTestHelpers.hpp"

template struct data_access::ForcingsEngineDataProvider<double, CatchmentAggrDataSelector>;
template struct data_access::ForcingsEngineDataProvider<double, GriddedDataSelector>;

ForcingsEngineDataProviderTest::mpi_info ForcingsEngineDataProviderTest::mpi_ = {};
std::shared_ptr<utils::ngenPy::InterpreterUtil> ForcingsEngineDataProviderTest::gil_ = nullptr;
std::time_t ForcingsEngineDataProviderTest::time_start = 0;
std::time_t ForcingsEngineDataProviderTest::time_end = 0;
