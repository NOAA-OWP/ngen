#ifndef NGEN_GIUH_KERNEL_HPP
#define NGEN_GIUH_KERNEL_HPP

#include <iostream>
#include <vector>
#include <string>

namespace giuh {

    /**
     * Pseudo-abstract class declaring the interface for a GIUH calculation kernel for a catchment.
     */
    class giuh_kernel {
    public:

        giuh_kernel(std::string catchment_id, std::string comid, unsigned int interpolation_regularity_seconds)
                : catchment_id(std::move(catchment_id)),
                  comid(std::move(comid)),
                  interpolation_regularity_seconds(interpolation_regularity_seconds) { }

        giuh_kernel(std::string catchment_id, std::string comid) : giuh_kernel(catchment_id, comid, 60) { }

        /**
         * Calculate the GIUH output for the given time step and runoff value.
         *
         * The default implementation for the interface simply passes through the input value.
         *
         * @param dt Time step value, in seconds.
         * @param direct_runoff The runoff input for this time step.
         * @return The calculated output for this time step.
         */
        virtual double calc_giuh_output(double dt, double direct_runoff) {
            // TODO: perhaps log this more effectively?
            std::cerr << "Pass-through kernel being used for GIUH calculations; raw input value `" << direct_runoff
                      << "` being passed through and immediately returned as output." << std::endl;
            return direct_runoff;
        }

        /**
         * Accessor for the catchment id of the associated catchment.
         *
         * @return the catchment id of the associated catchment.
         */
        std::string get_catchment_id() {
            return catchment_id;
        }

        /**
         * Accessor for the associated COMID for the GIUH source data.
         *
         * @return The associated COMID.
         */
        std::string get_comid() {
            return comid;
        }

        virtual std::vector<int> get_interpolated_ordinate_times() {
            return {((int) interpolation_regularity_seconds)};
        }

        /**
         * Get the object's interpolation regularity value - the difference in seconds between each regularized CDF.
         *
         * @return The object's interpolation regularity value - the difference in seconds between each regularized CDF.
         */
        unsigned int get_interpolation_regularity_seconds() const {
            return interpolation_regularity_seconds;
        }

        virtual std::vector<double> get_interpolated_incremental_ordinates() const {
            return {1.0};
        };

        virtual std::vector<double> get_interpolated_regularized_ordinates() const {
            return {1.0};
        };

        /**
         * Set the object's interpolation regularity value.
         *
         * @param regularity_seconds The interpolation regularity value - i.e., the difference between each of the
         *                           corresponding times of regularized CDF ordinate - in seconds
         */
        virtual void set_interpolation_regularity_seconds(unsigned int regularity_seconds) {
            interpolation_regularity_seconds = regularity_seconds;
        }

    protected:
        /** Main catchment identifier, as a string. */
        std::string catchment_id;
        /**
         * Associated COMID that is the look-up key for the GIUH serial data used to create this object, as a string.
         */
        std::string comid;
    private:
        /**
         * The regularity (i.e., time between each increment) used to interpolate and produce CDF ordinates, in seconds.
         */
        unsigned int interpolation_regularity_seconds;
    };
}

#endif //NGEN_GIUH_KERNEL_HPP
