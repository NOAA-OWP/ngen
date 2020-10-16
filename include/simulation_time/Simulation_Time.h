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
  int output_time_step;

  /*
   * @brief Constructor for simulation_time_params
   * @param start_time
   * @param end_time
   * @param output_time_step
  */
  simulation_time_params(std::string start_time, std::string end_time, int output_time_step):
    start_time(start_time), end_time(end_time), output_time_step(output_time_step)
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
     * @param output_time_step_seconds
     */
    Simulation_Time(simulation_time_params simulation_time_config):start_date_time_epoch(simulation_time_config.start_t),
                                           end_date_time_epoch(simulation_time_config.end_t),
                                           current_date_time_epoch(simulation_time_config.start_t),
                                           output_time_step_seconds(simulation_time_config.output_time_step)
    {

        simulation_total_time_seconds = end_date_time_epoch - start_date_time_epoch;

        /**
         * @brief Calculate total time_steps. Adding 1 to account for the first time time_step.
         */
        total_time_steps = simulation_total_time_seconds / output_time_step_seconds + 1;
    }

    /**
     * @brief Accessor to the total number of time steps
     * @return total_time_steps
     */
    int get_total_time_steps()
    {
        return total_time_steps;
    }

    /**
     * @brief Accessor to the current timestamp string
     * @return current_timestamp
     */ 
    std::string get_timestamp(int current_time_step)
    {
        current_date_time_epoch = start_date_time_epoch + current_time_step * output_time_step_seconds;
            
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

    int total_time_steps;
    int simulation_total_time_seconds;
    int output_time_step_seconds;

    time_t start_date_time_epoch;
    time_t end_date_time_epoch;
    time_t current_date_time_epoch;
};


#endif // SIMULATION_TIME_H
