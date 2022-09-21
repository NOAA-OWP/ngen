#ifndef TRIVALFORCINGPROVIDER_HPP
#define TRIVALFORCINGPROVIDER_HPP

#include <string>
#include <vector>
#include "GenericDataProvider.hpp"


#define OUTPUT_NAME_1 "output_name_1"
#define OUTPUT_VALUE_1 42.0
#define OUTPUT_DEFAULT_1 36.8

using namespace std;

namespace data_access {
    namespace test {
/**
 * A trivial implementation, strictly for testing OptionalWrappedDataProvider.
 */
        class TrivialForcingProvider : public GenericDataProvider {
        public:

            TrivialForcingProvider() {
                outputs.push_back(OUTPUT_NAME_1);
            }
                                  
            const std::vector<std::string>& get_avaliable_variable_names() override {
                return outputs;
            }

            long get_data_start_time() override {
                return std::numeric_limits<long>::min();
            }

            long get_data_stop_time() override {
                return std::numeric_limits<long>::max();
            }

            long record_duration() override {
                return 1;
            }

            size_t get_ts_index_for_time(const time_t &epoch_time) override {
                return 0;
            }

            double get_value(const CatchmentAggrDataSelector& selector, data_access::ReSampleMethod m) override {
                return (selector.get_variable_name() == OUTPUT_NAME_1) ? OUTPUT_VALUE_1 : 0.0;
            }
	   
            std::vector<double> get_values(const CatchmentAggrDataSelector& selector, data_access::ReSampleMethod m) override
            {
                return std::vector<double>(1, get_value(selector, m));
            }

            bool is_property_sum_over_time_step(const string &name) override {
                return true;
            }

        private:
            vector<string> outputs;
        };
    }
}

#endif // TRIVALFORCINGPROVIDER_HPP
