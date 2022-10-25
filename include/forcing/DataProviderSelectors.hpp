#ifndef NGEN_DATA_SELECTORS_HPP
#define NGEN_DATA_SELECTORS_HPP

#include <string>

/**  
 * @brief This class is intended to be the virtual base for all data selectors used with the 
 * DataProvider API
 * 
 */

class CatchmentAggrDataSelector
{
    public:

    /**
     * @brief Construct a new Catchment Aggregate Data Selector object with default values
     * 
     */
    CatchmentAggrDataSelector() : 
        variable_name(), 
        init_time(0), 
        duration_s(1), 
        output_units(),
        id_str()
    {}

    /**
     * @brief Construct a new Data Selector object with inital values
     * 
     * @param var The variable to be accessed by the selector   
     * @param start THe start time for this selector
     * @param dur The duration for this selector   
     * @param units The units to output the result in
     * @param id the id of the associated catchment
     */
    CatchmentAggrDataSelector(std::string id, std::string var, time_t start, long dur, std::string units) : 
        id_str(id),
        variable_name(var), 
        init_time(start), 
        duration_s(dur), 
        output_units(units)
    {}

    /**
     * @brief Get the variable name for this selector
     * 
     * @return std::string 
     */
    std::string get_variable_name() const { return variable_name; }

    /**
     * @brief Get the initial time for this selector
     * 
     * @return time_t 
     */
    time_t get_init_time() const { return init_time; }

    /**
     * @brief Get the duration in seconds that is requested by this selector
     * 
     * @return long 
     */
    long get_duration_secs() const { return duration_s; }

    /**
     * @brief Get the output units that is requested by this selector
     * 
     * @return std::string 
     */
    std::string get_output_units() const { return output_units; }

    /**
     * @brief Set the variable name for this selector
     * 
     * @param var The name of the variable to access
     */
    void set_variable_name(std::string var) { variable_name = var; }

    /**
     * @brief Set the init time for this selector
     * 
     * @param start The inital time for this query
     */
    void set_init_time(time_t start) { init_time = start; }

    /**
     * @brief Set the duration in secs for the query represented by this selector
     * 
     * @param dur the duration of the query in seconds 
     */
    void set_duration_secs(long dur) { duration_s = dur; }

    /**
     * @brief Set the output units for the data to be returned with this selector
     * 
     * @param units The units of the output
     */
    void set_output_units(std::string units) { output_units = units; }

    /**
     * @brief Get the id string for this NetCDF Data Selector
     * 
     * @return std::string 
     */
    std::string get_id() const { return id_str; }

    /**
     * @brief Set the id string for this NetCDF Data Selector
     * 
     * @param s 
     */
    void set_id(std::string s) { id_str = s; }

    private:

    std::string variable_name; //!< The variable name that should be queried
    time_t init_time; //!< The inital time to query the requested variable
    long duration_s;  //!< The duration of the query
    std::string output_units; //!< required units for the result to be return in
    std::string id_str; //< the catchment to access data for
};

/**
 * @brief This a data selector intended for use with CSV data
 * 
 */

class CSVDataSelector : public CatchmentAggrDataSelector
{
    public:

    CSVDataSelector(std::string var, time_t start, long dur, std::string units) : 
        CatchmentAggrDataSelector(std::string(), var, start, dur, units)
    {}

    #if GCC_VERSION < 8 && !defined(__llvm__)
    operator const CatchmentAggrDataSelector&() const { return *this; }
    #endif

    private:
};

class BMIDataSelector : public CatchmentAggrDataSelector
{
    public:

    BMIDataSelector(std::string var, time_t start, long dur, std::string units) : 
        CatchmentAggrDataSelector(std::string(), var, start, dur, units)
    {}

    #if GCC_VERSION < 8 && !defined(__llvm__)
    operator const CatchmentAggrDataSelector&() const { return *this; }
    #endif

    private:
};

#ifdef NETCDF_ACTIVE
/**
 * @brief This is the data selector intended for use with netcdf providers
 * 
 */

class NetCDFDataSelector : public CatchmentAggrDataSelector
{
    public:

    NetCDFDataSelector(std::string id) : CatchmentAggrDataSelector(id,std::string(),0,0,std::string()) {}
    NetCDFDataSelector(const char* id) : CatchmentAggrDataSelector(std::string(id),std::string(),0,0,std::string()) {}
    NetCDFDataSelector(std::string id, std::string var, time_t start, long dur, std::string units) :
        CatchmentAggrDataSelector(id, var, start, dur, units)
    {
        
    }

    #if GCC_VERSION < 8 && !defined(__llvm__)
    operator const CatchmentAggrDataSelector&() const { return *this; }
    #endif

    private: 
};
#endif

#endif
