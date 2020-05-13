#ifndef NGEN_GIUH_H
#define NGEN_GIUH_H

#include <string>
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
            this->catchment_id = catchment_id;
            this->cdf_times = cdf_times;
            // TODO: might be able to get this by calculating from times, rather than being passed
            this->cdf_cumulative_freqs = cdf_cumulative_freqs;
        }

        /**
          * Destructor
          */
        virtual ~giuh_kernel(){};

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

    };
}

#endif //NGEN_GIUH_H
