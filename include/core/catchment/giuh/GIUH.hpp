#ifndef NGEN_GIUH_H
#define NGEN_GIUH_H

#include <string>
#include <utility>
#include <vector>
#include <memory>

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
                std::string comid,
                std::vector<double> cdf_times,
                std::vector<double> cdf_cumulative_freqs
                )
        {
            this->catchment_id = std::move(catchment_id);
            this->comid = std::move(comid);
            this->cdf_times = std::move(cdf_times);
            // TODO: might be able to get this by calculating from times, rather than being passed
            this->cdf_cumulative_freqs = std::move(cdf_cumulative_freqs);

            carry_overs_list_head = nullptr;

            interpolation_regularity_seconds = 60;

            // TODO: have this be called by constructor, but consider later handling this concurrently
            interpolate_regularized_cdf();
        }

        /**
          * Destructor
          */
        virtual ~giuh_kernel()= default;;

        /**
         * Calculate the GIUH output for the given time step and runoff value, lazily generating the necessary
         * regularized CDF ordinates if appropriate.
         *
         * @param dt Time step value, in seconds.
         * @param direct_runoff The runoff input for this time step.
         * @return The calculated output for this time step.
         */
        virtual double calc_giuh_output(double dt, double direct_runoff);

        /**
         * Accessor for the catchment id of the associated catchment.
         *
         * @return the catchment id of the associated catchment.
         */
        std::string get_catchment_id();

        /**
         * Accessor for the COMID of the associated catchment.
         *
         * @return the COMID of the associated catchment.
         */
        std::string get_comid();

        unsigned int get_interpolation_regularity_seconds();

        /**
         * Set the object's interpolation regularity value, triggering recalculation of the interpolated, regularized
         * CDF ordinates IFF the new regularity value is different from the previous.
         *
         * @param regularity_seconds The interpolation regularity value - i.e., the difference between each of the
         *                           corresponding times of regularized CDF ordinate - in seconds
         */
        void set_interpolation_regularity_seconds(unsigned int regularity_seconds);

    private:
        // TODO: document how member variables are named with 'cdf_*' being things that come from external data, and
        //  'interpolated_*' being things that get produced within the class (to help keep straight what things are).

        /** Main catchment identifier, as a string. */
        std::string catchment_id;
        /** COMID catchment identifier, as a string. */
        std::string comid;
        /** Ranked order of time of travel cell values for CDF. */
        std::vector<double> cdf_cumulative_freqs;
        /** CDF times in seconds. */
        std::vector<double> cdf_times;
        /**
         * The regularity (i.e., time between each increment) used to interpolate and produce CDF ordinates, in seconds.
         */
        unsigned int interpolation_regularity_seconds;
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

        /**
         * Perform the interpolation of regularized CDF ordinates.
         */
        void interpolate_regularized_cdf();

    };
}

#endif //NGEN_GIUH_H
