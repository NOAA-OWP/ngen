#ifndef NGEN_GIUH_H
#define NGEN_GIUH_H

#include <string>
#include <utility>
#include <vector>

namespace giuh {

    struct giuh_carry_over {
        /** The amount of the original runoff input. */
        double original_input_amount;
        /** Index of the last 'interpolated_ordinate_times_seconds' used to calculate and output contribution. */
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
            interpolation_regularity_seconds = 60;

            // Interpolate regularized CDF (might should be done out of constructor, perhaps concurrently)
            interpolated_ordinate_times_seconds.push_back(0);
            interpolated_regularized_ordinates.push_back(0);
            // Increment the ordinate time based on the regularity (loop below will do this at the end of each iter)
            int time_for_ordinate = interpolated_ordinate_times_seconds.back() + interpolation_regularity_seconds;

            // Loop through ordinate times, initializing all but the last ordinate
            while (time_for_ordinate < this->cdf_times.back()) {
                interpolated_ordinate_times_seconds.push_back(time_for_ordinate);

                // Find index 'i' of largest CDF time less than the time for the current ordinate
                // Start by getting the index of the first time greater than time_for_ordinate
                int cdf_times_index_for_iteration = 0;
                while (this->cdf_times[cdf_times_index_for_iteration] < interpolated_ordinate_times_seconds.back()) {
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
                interpolated_regularized_ordinates.push_back(result);

                // Finally, increment the ordinate time based on the regularity
                time_for_ordinate = interpolated_ordinate_times_seconds.back() + interpolation_regularity_seconds;
            }

            // Finally, the last ordinate time gets set to have everything
            interpolated_ordinate_times_seconds.push_back(time_for_ordinate);
            interpolated_regularized_ordinates.push_back(1.0);

            // Also, now calculate the incremental values between each ordinate step
            interpolated_incremental_runoff_values.resize(interpolated_regularized_ordinates.size());
            for (unsigned i = 0; i < interpolated_regularized_ordinates.size(); i++) {
                interpolated_incremental_runoff_values[i] =
                        i == 0 ? 0 : interpolated_regularized_ordinates[i] - interpolated_regularized_ordinates[i - 1];
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
        // TODO: document how member variables are named with 'cdf_*' being things that come from external data, and
        //  'interpolated_*' being things that get produced within the class (to help keep straight what things are).

        /** Associated catchment identifier, as a string. */
        std::string catchment_id;
        /** Ranked order of time of travel cell values for CDF. */
        std::vector<double> cdf_cumulative_freqs;
        /** CDF times in seconds. */
        std::vector<double> cdf_times;
        /**
         * The regularity (i.e., time between each increment) used to interpolate and produce CDF ordinates, in seconds.
         */
        int interpolation_regularity_seconds;
        /** The corresponding, regular time values for each of the interpolated CDF ordinates. */
        std::vector<int> interpolated_ordinate_times_seconds;
        /** The regularized CDF ordinate interpolated values. */
        std::vector<double> interpolated_regularized_ordinates;
        /**
         * The incremental increases at each index of the interpolated, regularized CDF ordinate value, from the value
         * at the previous index.
         */
        std::vector<double> interpolated_incremental_runoff_values;
        /** List to hold carry-over amounts from previous inputs, which didn't all flow out at that time step. */
        std::shared_ptr<giuh_carry_over> carry_overs_list_head;

    };
}

#endif //NGEN_GIUH_H
