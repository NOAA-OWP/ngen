#ifndef NGEN_GIUH_H
#define NGEN_GIUH_H

#include <string>
#include <utility>
#include <vector>

namespace giuh {

    struct giuh_carry_over {
        /** The amount of the original runoff input. */
        double original_input_amount;
        /** Index of the last 'cdf_ordinate_times_seconds' used to calculate and output contribution. */
        unsigned long last_outputted_cdf_index;
        /** A pointer to a "next" giuh_carry_over struct value, allowing these to self-assemble into a simple list. */
        std::shared_ptr<giuh_carry_over> next;

        giuh_carry_over(double original_runoff, unsigned long last_cdf_index) : original_input_amount(original_runoff),
                                                                                last_outputted_cdf_index(last_cdf_index),
                                                                                next(nullptr){}
    };

    class giuh_kernel {

    public:

        // TODO: perhaps add separate constructor or factory (or both) for getting info from file

        giuh_kernel(
                std::string catchment_id,
                std::vector<double> cdf_times,
                std::vector<double> cdf_cumulative_freqs
                )
        {
            this->catchment_id = std::move(catchment_id);
            this->cdf_times = std::move(cdf_times);
            // TODO: might be able to get this by calculating from times, rather than being passed
            this->cdf_cumulative_freqs = std::move(cdf_cumulative_freqs);

            carry_overs_list_head = nullptr;

            // TODO: look at not hard-coding this later
            cdf_regularity_seconds = 60;

            // Interpolate regularized CDF (might should be done out of constructor, perhaps concurrently)
            cdf_ordinate_times_seconds.push_back(0);
            regularized_cdf_ordinates.push_back(0);
            // Increment the ordinate time based on the regularity (loop below will do this at the end of each iter)
            int time_for_ordinate = cdf_ordinate_times_seconds.back() + cdf_regularity_seconds;

            // Loop through ordinate times, initializing all but the last ordinate
            while (time_for_ordinate < this->cdf_times.back()) {
                cdf_ordinate_times_seconds.push_back(time_for_ordinate);

                // Find index 'i' of largest CDF time less than the time for the current ordinate
                // Start by getting the index of the first time greater than time_for_ordinate
                int cdf_times_index_for_iteration = 0;
                while (this->cdf_times[cdf_times_index_for_iteration] < cdf_ordinate_times_seconds.back()) {
                    cdf_times_index_for_iteration++;
                }
                // With the index of the first larger, back up one to get the last smaller
                cdf_times_index_for_iteration--;

                // Then apply equation from spreadsheet
                double result = (time_for_ordinate - this->cdf_times[cdf_times_index_for_iteration]) /
                                (this->cdf_times[cdf_times_index_for_iteration + 1] -
                                 this->cdf_times[cdf_times_index_for_iteration]) *
                                (this->cdf_cumulative_freqs[cdf_times_index_for_iteration + 1] -
                                 this->cdf_cumulative_freqs[cdf_times_index_for_iteration]) +
                                this->cdf_cumulative_freqs[cdf_times_index_for_iteration];
                // Push that to the back of that collection
                regularized_cdf_ordinates.push_back(result);

                // Finally, increment the ordinate time based on the regularity
                time_for_ordinate = cdf_ordinate_times_seconds.back() + cdf_regularity_seconds;
            }

            // Finally, the last ordinate time gets set to have everything
            cdf_ordinate_times_seconds.push_back(time_for_ordinate);
            regularized_cdf_ordinates.push_back(1.0);

            // Also, now calculate the incremental values between each ordinate step
            incremental_runoff_values.resize(regularized_cdf_ordinates.size());
            for (unsigned i = 0; i < regularized_cdf_ordinates.size(); i++) {
                incremental_runoff_values[i] =
                        i == 0 ? 0 : regularized_cdf_ordinates[i] - regularized_cdf_ordinates[i - 1];
            }
        }

        /**
          * Destructor
          */
        virtual ~giuh_kernel()= default;;

        virtual double calc_giuh_output(double dt, double direct_runoff);

        /**
         * Accessor for the catchment id of the associated catchment.
         *
         * @return the catchment id of the associated catchment.
         */
        std::string get_catchment_id();

    private:
        /** Associated catchment identifier, as a string. */
        std::string catchment_id;
        /** Ranked order of time of travel cell values. */
        std::vector<double> cdf_cumulative_freqs;
        /** Times in seconds. */
        std::vector<double> cdf_times;
        /** The regularity used to interpolate and produce CDF ordinates, in seconds. */
        int cdf_regularity_seconds;
        /** The corresponding time values for each of the calculated CDF ordinates. */
        std::vector<int> cdf_ordinate_times_seconds;
        /** The regularized CDF ordinate values. */
        std::vector<double> regularized_cdf_ordinates;
        /**
         * The incremental increases in the regularized CDF ordinate at a given index from the value at the previous
         * index.
         */
        std::vector<double> incremental_runoff_values;
        /** Queue to hold carry-over amounts from previous inputs, which didn't all flow out at that time step. */
        std::shared_ptr<giuh_carry_over> carry_overs_list_head;

    };
}

#endif //NGEN_GIUH_H
