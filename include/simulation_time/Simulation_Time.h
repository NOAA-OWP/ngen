#ifndef SIMULATION_TIME_H
#define SIMULATION_TIME_H

#include <ctime>
#include <time.h>

using namespace std;

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
     * @brief Accessor to the current timestamp string
     * @return current_timestamp
     */ 
    std::string get_timestamp(int current_output_time_index)
    {
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

    private:

    int total_output_times;
    int simulation_total_time_seconds;
    int output_interval_seconds;

    time_t start_date_time_epoch;
    time_t end_date_time_epoch;
    time_t current_date_time_epoch;
};


#endif // SIMULATION_TIME_H
