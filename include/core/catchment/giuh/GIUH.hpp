#ifndef NGEN_GIUH_H
#define NGEN_GIUH_H

#include <string>
#include <utility>
#include <vector>

namespace giuh {
    class giuh_kernel {

        // TODO: create more complete class definition and implementation

    public:

        // TODO: perhaps add separate constructor or factory (or both) for getting info from file

        // TODO: complete constructor
        giuh_kernel(
                string catchment_id,
                vector<double> cdf_times,
                vector<double> cdf_cumulative_freqs
                )
        {
            this->catchment_id = std::move(catchment_id);
            this->cdf_times = std::move(cdf_times);
            // TODO: might be able to get this by calculating from times, rather than being passed
            this->cdf_cumulative_freqs = std::move(cdf_cumulative_freqs);

            // TODO: look at not hard-coding this later
            cdf_regularity_seconds = 60;

            // Interpolate regularized CDF (might should be done out of constructor, perhaps concurrently)
            cdf_ordinate_times_seconds.push_back(0);
            regularized_cdf_ordinates.push_back(0);
            // TODO: this condition may need to be refined slightly
            while (regularized_cdf_ordinates.back() < 1.0) {
                // Determine current regularity interval iteration (last iteration plus regularity)
                int current_iteration_time = cdf_ordinate_times_seconds.back() + cdf_regularity_seconds;
                cdf_ordinate_times_seconds.push_back(current_iteration_time);
                // Find index 'i' of largest CDF time less that current regularity interval iteration
                int cdf_times_index_for_iteration = 0;
                while (this->cdf_times[cdf_times_index_for_iteration] < cdf_ordinate_times_seconds.back()) {
                    cdf_times_index_for_iteration++;
                }
                // This has found the index of the first bigger, so we need to back up one to get the last smaller
                cdf_times_index_for_iteration--;
                // Then apply equation from spreadsheet

                double result = (current_iteration_time - this->cdf_times[cdf_times_index_for_iteration]) /
                                (this->cdf_times[cdf_times_index_for_iteration + 1] -
                                 this->cdf_times[cdf_times_index_for_iteration]) *
                                (this->cdf_cumulative_freqs[cdf_times_index_for_iteration + 1] -
                                 this->cdf_cumulative_freqs[cdf_times_index_for_iteration]) +
                                this->cdf_cumulative_freqs[cdf_times_index_for_iteration];
                // Push that to the back of that collection
                regularized_cdf_ordinates.push_back(result);
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
        string get_catchment_id();

    private:
        string catchment_id;                    //!< Associated catchment identifier, as a string
        vector<double> cdf_cumulative_freqs;    //!< ranked order of time of travel cell values
        vector<double> cdf_times;               //!< times in seconds
        int cdf_regularity_seconds;             //!< the regularity used to interpolate and produce CDF ordinates, in seconds
        vector<int> cdf_ordinate_times_seconds;
        vector<double> regularized_cdf_ordinates;

    };
}

#endif //NGEN_GIUH_H
