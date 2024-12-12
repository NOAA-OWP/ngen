#ifndef TRIVALFORCINGPROVIDER_HPP
#define TRIVALFORCINGPROVIDER_HPP

#include <string>
#include <vector>
#include "GenericDataProvider.hpp"


#define OUTPUT_NAME_1 "output_name_1"
#define OUTPUT_VALUE_1 42.0
#define OUTPUT_DEFAULT_1 36.8


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
                                  
            boost::span<const std::string> get_available_variable_names() const override {
                return outputs;
            }

            long get_data_start_time() const override {
                return std::numeric_limits<long>::min();
            }

            long get_data_stop_time() const override {
                return std::numeric_limits<long>::max();
            }

            long record_duration() const override {
                return 1;
            }

            size_t get_ts_index_for_time(const time_t &epoch_time) const override {
                return 0;
            }

            double get_value(const CatchmentAggrDataSelector& selector, data_access::ReSampleMethod m) override {
                return (selector.get_variable_name() == OUTPUT_NAME_1) ? OUTPUT_VALUE_1 : 0.0;
            }
	   
            std::vector<double> get_values(const CatchmentAggrDataSelector& selector, data_access::ReSampleMethod m) override
            {
                return std::vector<double>(1, get_value(selector, m));
            }

            bool is_property_sum_over_time_step(const std::string &name) const override {
                return true;
            }

        private:
            std::vector<std::string> outputs;
        };
    }
}

#endif // TRIVALFORCINGPROVIDER_HPP
