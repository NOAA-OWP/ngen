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
         * The function essentially transforms the provided runoff values vector into a CDF frequency vector by setting
         * each ``i``-th frequency value to the ``(i-1)``-th frequency value plus the ``i``-th incremental value.  The
         * ``0``-th index values is set to ``0``.
         *
         * A corresponding vector of regularized times is also created, with each value being
         * ``interpolation_regularity_seconds`` larger than the previous (again, starting at ``0``).
         *
         * These vectors are then used in a call to the ``giuh_kernel_impl`` constructor, with the resulting object
         * returned.
         *
         * @param catchment_id
         * @param comid
         * @param interpolation_regularity_seconds  The regular interval between the provided incremental runoff values.
         * @param incremental_runoffs   A collection of incremental runoff values (akin to `
         *                              `get_interpolated_incremental_runoff()``).
         * @return A new ``giuh_kernel_impl`` object induced from the implied CDF ordinates.
         */
        static giuh_kernel_impl make_from_incremental_runoffs(std::string catchment_id,
                                                              std::string comid,
                                                              int interpolation_regularity_seconds,
                                                              std::vector<double> incremental_runoffs)
        {
            std::vector<double> ordinates(incremental_runoffs.size() + 1, 0.0);
            std::vector<double> ordinate_times(incremental_runoffs.size() + 1, 0.0);

            double ordinate_sum = 0.0;
            int time_sum = 0;
            for (unsigned int i = 1; i < ordinates.size(); ++i) {
                ordinate_sum += incremental_runoffs[i-1];
                ordinates[i] = ordinate_sum;
                time_sum += interpolation_regularity_seconds;
                ordinate_times[i] = time_sum;
            }

            return giuh_kernel_impl(catchment_id, comid, ordinate_times, ordinates, interpolation_regularity_seconds);
        }

        /**
         * Initialize, using the given (not-necessarily-regular) CDF times and cumulative frequencies, and the provided
         * interpolation regularity interval size.
         *
         * The give CDF times and frequencies form the base data for the object.  These values will not change.  From
         * these, interpolated regularized values will be inferred.  These will be set during object construction,
         * although they can potentially be changed by later changing the interpolation regularity interval value.
         *
         * @param catchment_id
         * @param comid
         * @param cdf_times A vector of base CDF time values, which may or may not be spaced in regular intervals.
         * @param cdf_cumulative_freqs A vector of CDF cumulative frequency values, corresponding to each of the times
         *                             in the ``cdf_times`` parameter.
         * @param interpolation_regularity_seconds The size of regular intervals to use when interpolating the
         *                                         regularize CDF ordinates.
         */
        giuh_kernel_impl(
                std::string catchment_id,
                std::string comid,
                std::vector<double> cdf_times,
                std::vector<double> cdf_cumulative_freqs,
                unsigned int interpolation_regularity_seconds
        ) : giuh_kernel(std::move(catchment_id), std::move(comid), interpolation_regularity_seconds),
            cdf_times(std::move(cdf_times)), cdf_cumulative_freqs(std::move(cdf_cumulative_freqs)) {
            carry_overs_list_head = nullptr;

            // TODO: have this be called by constructor, but consider later handling this concurrently
            interpolate_regularized_cdf();
        }

        /**
         * Initialize, using the given (not-necessarily-regular) CDF times and cumulative frequencies, and assuming
         * a default interpolation regularity of 60 seconds.
         *
         * The give CDF times and frequencies form the base data for the object.  These values will not change.  From
         * these, interpolated regularized values will be inferred.  These will be set during object construction,
         * although they can potentially be changed by later changing the interpolation regularity interval value.
         *
         * @param catchment_id
         * @param comid
         * @param cdf_times A vector of base CDF time values, which may or may not be spaced in regular intervals.
         * @param cdf_cumulative_freqs A vector of CDF cumulative frequency values, corresponding to each of the times
         *                             in the ``cdf_times`` parameter.
         */
        giuh_kernel_impl(
                std::string catchment_id,
                std::string comid,
                std::vector<double> cdf_times,
                std::vector<double> cdf_cumulative_freqs
        ) : giuh_kernel_impl(std::move(catchment_id), std::move(comid), std::move(cdf_times),
                             std::move(cdf_cumulative_freqs), 60) {

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

        std::vector<double> get_interpolated_incremental_runoff() const override;

        std::vector<int> get_regularized_times_s() override;

        std::vector<double> get_interpolated_regularized_cdf() const override;

        /**
         * Set the object's interpolation regularity value, also triggering recalculation of the interpolated,
         * regularized CDF ordinates IFF the new regularity value is different from the previous.
         *
         * @param regularity_seconds The interpolation regularity value - i.e., the difference between each of the
         *                           corresponding times of regularized CDF ordinate - in seconds
         */
        void set_interpolation_regularity_seconds(unsigned int regularity_seconds) override;

    private:
        /**
         * The cumulative frequency (or rank order) of each ``i``-th cell in the travel time cumulative distribution
         * function, as originally supplied to the object.
         *
         * For all values in the collection, where the collection is of size ``s``, the ``i``-th value is equal to:
         *      ``i / (s - 1)``
         */
        const std::vector<double> cdf_cumulative_freqs;
        /**
         * The travel time in seconds for each i-th cell in the cumulative distribution function, as originally supplied
         * to the object.
         */
        const std::vector<double> cdf_times;
        /**
         * Regular time values (in seconds) for interpolated CDF ordinates, regularized according to the value of
         * ``interpolation_regularity_seconds``.
         *
         * This a collection of size ``s``, where the values can be defined by a function ``t(i)``, where:
         *      ``t(0) = 0``
         *      ``t(i) == t(i-1) + interpolation_regularity_seconds`` for all ``0 < i < s``
         *
         * E.g., for regularity of 60 seconds, something like ``0, 60, 120, 180, 240 ...``.
         */
        std::vector<int> regularized_times_s;
        /**
         * The regularized CDF values interpolated from the original CDF data from ``cdf_cumulative_freqs`` when using
         * the regularized times in ``regularized_times_s``.
         */
        std::vector<double> interpolated_regularized_cdf;
        /**
         * The incremental increase at each index ``i`` of ``interpolated_regularized_cdf`` from index ``i-1``.
         */
        std::vector<double> interpolated_incremental_runoff;
        /** List to hold carry-over amounts from previous inputs, which didn't all flow out at that time step. */
        std::shared_ptr<giuh_carry_over> carry_overs_list_head;

        /**
         * Perform the interpolation of regularized CDF ordinates.
         */
        void interpolate_regularized_cdf();

    };
}

#endif //NGEN_GIUH_H
