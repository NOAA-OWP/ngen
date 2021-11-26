#ifndef TRIVALFORCINGPROVIDER_HPP
#define TRIVALFORCINGPROVIDER_HPP

#include <string>
#include <vector>
#include "ForcingProvider.hpp"

#define OUTPUT_NAME_1 "output_name_1"
#define OUTPUT_VALUE_1 42.0
#define OUTPUT_DEFAULT_1 36.8

using namespace std;

namespace forcing {
    namespace test {
/**
 * A trivial implementation, strictly for testing OptionalWrappedProvider.
 */
        class TrivialForcingProvider : public ForcingProvider {
        public:

            TrivialForcingProvider() {
                outputs.push_back(OUTPUT_NAME_1);
            }

            const vector<string> &get_available_forcing_outputs() override {
                return outputs;
            }

            time_t get_forcing_output_time_begin(const string &output_name) override {
                return 0;
            }

            time_t get_forcing_output_time_end(const string &output_name) override {
                return 1000000;
            }

            size_t get_ts_index_for_time(const time_t &epoch_time) override {
                return 0;
            }

            double get_value(const string &output_name, const time_t &init_time, const long &duration_s,
                             const string &output_units) override {
                return (output_name == OUTPUT_NAME_1) ? OUTPUT_VALUE_1 : 0.0;
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