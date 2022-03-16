#ifndef NGEN_DATAPROVIDER_HPP
#define NGEN_DATAPROVIDER_HPP

#include <string>


namespace data_access
{
    /** An abstraction for classes that provides access to data.
    *
    * Data may be pre-provided from some external source, internally calculated by the implementing type, or some
    * combination of both.
    */

    template <class data_type, class selection_type> class DataProvider
    {
        /** This class provides a generic interface to data services
        *
        */

        public:

        enum ReSampleMethod
        {
            MEAN,
            SUM
        };

        virtual ~DataProvider() = default;

        /** Return the variables that are accessable by this data provider */

        virtual const std::vector<std::string>& get_avaliable_variable_names() = 0;

        /** Return the first valid time for which data from the request variable  can be requested */

        virtual long get_data_start_time() = 0;

        /** Return the last valid time for which data from the requested variable can be requested */

        virtual long get_data_stop_time() = 0;

        /** Return the stride in the time dimension */
        virtual long record_duration() = 0;

        /**
         * Get the index of the data time step that contains the given point in time.
         *
         * An @ref std::out_of_range exception should be thrown if the time is not in any time step.
         *
         * @param epoch_time The point in time, as a seconds-based epoch time.
         * @return The index of the forcing time step that contains the given point in time.
         * @throws std::out_of_range If the given point is not in any time step.
         */
        virtual size_t get_ts_index_for_time(const time_t &epoch_time) = 0;

        /**
         * Get the value of a forcing property for an arbitrary time period, converting units if needed.
         *
         * An @ref std::out_of_range exception should be thrown if the data for the time period is not available.
         *
         * @param selector Data required to establish what subset of the stored data should be accessed
         * @param variable_name The name of the data property of interest.
         * @param init_time_epoch The epoch time (in seconds) of the start of the time period.
         * @param duration_seconds The length of the time period, in seconds.
         * @param output_units The expected units of the desired output value.
         * @param m How data is to be resampled if there is a mismatch in data alignment or repeat rate
         * @return The value of the forcing property for the described time period, with units converted if needed.
         * @throws std::out_of_range If data for the time period is not available.
         */
        virtual data_type get_value(selection_type selector, const std::string &variable_name, const time_t &init_time, const long &duration_s,
                                 const std::string &output_units, ReSampleMethod m=SUM) = 0;

        private:
    };

}



#endif // NGEN_DATAPROVIDER_HPP
