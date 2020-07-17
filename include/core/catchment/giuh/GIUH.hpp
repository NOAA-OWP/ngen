#ifndef NGEN_GIUH_H
#define NGEN_GIUH_H

#include "giuh_kernel.hpp"
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

    /**
     * A concrete implementation of a GIUH calculation kernel.
     */
    class giuh_kernel_impl : public giuh_kernel {

    public:

        /**
         * Factory create a ``giuh_kernel_impl`` object, deriving CDF times and frequencies from a regularized
         * interpolation interval value and a collection of incremental runoff values.
         *
         * The function essentially transforms the provided runoff values vector into a CDF vector setting each ``i``-th
         * ordinate to the ``i``-th incremental value plus the ``(i-1)``-th ordinate value.  A corresponding vector of
         * times is also created, with each value being ``interpolation_regularity_seconds`` larger than the previous
         * (starting at ``0``).  These are then used in a call to the ``giuh_kernel_impl`` constructor, with the
         * resulting object returned.
         *
         * @param catchment_id
         * @param comid
         * @param interpolation_regularity_seconds
         * @param incremental_runoffs
         * @return
         */
        static giuh_kernel_impl make_from_incremental_runoffs(std::string catchment_id,
                                                              std::string comid,
                                                              int interpolation_regularity_seconds,
                                                              std::vector<double> incremental_runoffs)
        {
            std::vector<double> ordinates(incremental_runoffs.size() + 1);
            std::vector<double> ordinate_times(incremental_runoffs.size() + 1);

            double ordinate_sum = 0.0;
            int time_sum = 0;
            ordinates.push_back(ordinate_sum);
            ordinate_times.push_back(time_sum);
            for (unsigned int i = 1; i < ordinates.size(); ++i) {
                ordinate_sum += incremental_runoffs[i-1];
                ordinates[i] = ordinate_sum;
                time_sum += interpolation_regularity_seconds;
                ordinate_times[i] = time_sum;
            }

            return giuh_kernel_impl(catchment_id, comid, ordinate_times, ordinates, interpolation_regularity_seconds);
        }

        giuh_kernel_impl(
                std::string catchment_id,
                std::string comid,
                std::vector<double> cdf_times,
                std::vector<double> cdf_cumulative_freqs,
                unsigned int interpolation_regularity_seconds
                ) : giuh_kernel(std::move(catchment_id), std::move(comid), interpolation_regularity_seconds)
        {
            this->cdf_times = std::move(cdf_times);
            // TODO: might be able to get this by calculating from times, rather than being passed
            this->cdf_cumulative_freqs = std::move(cdf_cumulative_freqs);

            carry_overs_list_head = nullptr;

            // TODO: have this be called by constructor, but consider later handling this concurrently
            interpolate_regularized_cdf();
        }

        giuh_kernel_impl(
                std::string catchment_id,
                std::string comid,
                std::vector<double> cdf_times,
                std::vector<double> cdf_cumulative_freqs
        ) : giuh_kernel_impl(std::move(catchment_id), std::move(comid), cdf_times, cdf_cumulative_freqs, 60) {

        }

        /**
          * Destructor
          */
        virtual ~giuh_kernel_impl()= default;

        /**
         * Calculate the GIUH output for the given time step and runoff value, lazily generating the necessary
         * regularized CDF ordinates if appropriate.
         *
         * @param dt Time step value, in seconds.
         * @param direct_runoff The runoff input for this time step.
         * @return The calculated output for this time step.
         */
        double calc_giuh_output(double dt, double direct_runoff) override;

        /**
         * Set the object's interpolation regularity value, also triggering recalculation of the interpolated,
         * regularized CDF ordinates IFF the new regularity value is different from the previous.
         *
         * @param regularity_seconds The interpolation regularity value - i.e., the difference between each of the
         *                           corresponding times of regularized CDF ordinate - in seconds
         */
        void set_interpolation_regularity_seconds(unsigned int regularity_seconds) override;

    private:
        // TODO: document how member variables are named with 'cdf_*' being things that come from external data, and
        //  'interpolated_*' being things that get produced within the class (to help keep straight what things are).
        /** Ranked order of time of travel cell values for CDF. */
        std::vector<double> cdf_cumulative_freqs;
        /** CDF times in seconds. */
        std::vector<double> cdf_times;
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
