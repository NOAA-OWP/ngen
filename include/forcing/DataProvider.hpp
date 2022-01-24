#ifndef NGEN_DATAPROVIDER_HPP
#define NGEN_DATAPROVIDER_HPP


namespace data_access
{
    /** An abstraction for classes that provider access to data.
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

        virtual ~DataProvider() = default;

        /** Return the variables that are accessable by this data provider */

        virtual const std::veector<std::string> get_avaliable_variable_names() = 0;

        /** Return the first valid time for which data from the request variaable  can be requested */

        virtual get_data_start_time(std::string var) = 0;

        /** Return the last valid time for which data from the requested variablle can be requested */

        virtual get_data_stop_time(std::string var) = 0;

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
         * @param output_name The name of the data property of interest.
         * @param init_time_epoch The epoch time (in seconds) of the start of the time period.
         * @param duration_seconds The length of the time period, in seconds.
         * @param output_units The expected units of the desired output value.
         * @return The value of the forcing property for the described time period, with units converted if needed.
         * @throws std::out_of_range If data for the time period is not available.
         */
        virtual data_type get_value(selection_type selector, const std::string &variable_name, const time_t &init_time, const long &duration_s,
                                 const std::string &output_units) = 0;

        private:
    };

}



#endif // NGEN_DATAPROVIDER
