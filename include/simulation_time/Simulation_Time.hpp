#ifndef SIMULATION_TIME_H
#define SIMULATION_TIME_H

#include <ctime>
#include <time.h>
#include <string>
#include <stdexcept>

/**
 * @brief simulation_time_params providing configuration information for simulation time period.
 */
struct simulation_time_params
{
  std::string start_time;
  std::string end_time;
  std::string date_format =  "%Y-%m-%d %H:%M:%S";
  time_t start_t;
  time_t end_t;
  int output_interval;

  /*
   * @brief Constructor for simulation_time_params
   * @param start_time
   * @param end_time
   * @param output_interval
  */
  simulation_time_params(std::string start_time, std::string end_time, int output_interval):
    start_time(start_time), end_time(end_time), output_interval(output_interval)
    {
      /// \todo converting to UTC can be tricky, especially if thread safety is a concern
      /* https://stackoverflow.com/questions/530519/stdmktime-and-timezone-info */
      struct tm tm;
      strptime(this->start_time.c_str(), this->date_format.c_str() , &tm);
      //mktime returns time in local time based on system timezone
      //FIXME use timegm (not standard)? or implement timegm (see above link)
      this->start_t = timegm( &tm );

      strptime(this->end_time.c_str(), this->date_format.c_str() , &tm);
      this->end_t = timegm( &tm );
    }
};


/**
 * @brief Simulation Time class providing time-series variables and methods to the model.
 */
class Simulation_Time
{
    public:
    /**
     * @brief Constructor building a Simulation Time object
     * @param start_date_time_epoch
     * @param end_date_time_epoch
     * @param current_date_time_epoch
     * @param output_interval_seconds
     */
    Simulation_Time(simulation_time_params simulation_time_config):start_date_time_epoch(simulation_time_config.start_t),
                                           end_date_time_epoch(simulation_time_config.end_t),
                                           current_date_time_epoch(simulation_time_config.start_t),
                                           output_interval_seconds(simulation_time_config.output_interval)
    {

        simulation_total_time_seconds = end_date_time_epoch - start_date_time_epoch;

        /**
         * @brief Calculate total output_timess. Adding 1 to account for the first time output_time.
         */
        total_output_times = simulation_total_time_seconds / output_interval_seconds + 1;
    }

    Simulation_Time(const Simulation_Time& t, int interval) : Simulation_Time(t)
    {
        output_interval_seconds = interval;
        total_output_times = simulation_total_time_seconds / output_interval_seconds + 1;
    }

    /**
     * @brief Accessor to the total number of time steps
     * @return total_output_times
     */
    int get_total_output_times()
    {
        return total_output_times;
    }

    /**
     * @brief Accessor to the output_interval_seconds
     * @return output_interval_seconds
     */
    int get_output_interval_seconds()
    {
        return output_interval_seconds;
    }

    /**
     * @brief Accessor to the the current simulation time
     * @return current_date_time_epoch
    */

    time_t get_current_epoch_time()
    {
        return current_date_time_epoch;
    }   

    /**
     * @brief Accessor to the current timestamp string
     * @return current_timestamp
     */ 
    std::string get_timestamp(int current_output_time_index)
    {
        // "get" method mutates state!
        current_date_time_epoch = start_date_time_epoch + current_output_time_index * output_interval_seconds;
            
        struct tm *temp_gmtime_struct;

        temp_gmtime_struct = gmtime(&current_date_time_epoch);

        char current_timestamp[20];
        const char* time_format = "%Y-%m-%d %T";

        if (strftime(current_timestamp, sizeof(current_timestamp), time_format, temp_gmtime_struct) == 0) { 
            fprintf(stderr, "ERROR: strftime returned 0");
            exit(EXIT_FAILURE); 
        }

        return current_timestamp;
    }

    inline int next_timestep_index(int epoch_time_seconds)
    {
        return int(epoch_time_seconds - start_date_time_epoch) / output_interval_seconds;
    }

    inline int next_timestep_index()
    {
        return next_timestep_index(current_date_time_epoch);
    }

    inline time_t next_timestep_epoch_time(int epoch_time_seconds){
        return start_date_time_epoch + ( next_timestep_index(epoch_time_seconds) * output_interval_seconds );
    }

    inline time_t next_timestep_epoch_time(){
        return next_timestep_epoch_time(current_date_time_epoch);
    }

    inline int diff(const Simulation_Time& other){
        return start_date_time_epoch - other.start_date_time_epoch;
    }

    /**
     * @brief move this simulation time object to represent the next time step as the current time
    */

    inline void advance_timestep(){
        current_date_time_epoch += output_interval_seconds;

        if (current_date_time_epoch > end_date_time_epoch)
        {
            throw std::runtime_error("Simulation time objects current time exceeded the end_date_time_epoch value for that object");
        }
    }


    private:

    int total_output_times;
    int simulation_total_time_seconds;
    int output_interval_seconds;

    time_t start_date_time_epoch;
    time_t end_date_time_epoch;
    time_t current_date_time_epoch;
};


#endif // SIMULATION_TIME_H
