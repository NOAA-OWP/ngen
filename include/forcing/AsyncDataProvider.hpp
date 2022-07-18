#ifndef NGEN_ASYNC_DATA_PROVIDER
#define NGEN_ASYNC_DATA_PROVIDER

#include "DataProvider.hpp"

namespace data_access
{
    template <class data_type, class selection_type> class AsyncDataProvider : public DataProvider<data_type, selection_type>
    {
        virtual bool value_ready(selection_type selector, const std::string &variable_name, const time_t &init_time, const long &duration_s,
                                 const std::string &output_units, ReSampleMethod m=SUM) = 0;

        virtual void request_value(selection_type selector, const std::string &variable_name, const time_t &init_time, const long &duration_s,
                                 const std::string &output_units, ReSampleMethod m=SUM) = 0;
    };
}

#endif