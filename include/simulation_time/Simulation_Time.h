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
    Constructor for simulation_time_params
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
 * @brief Simulation Time class providing time-series arrays to the model.
 */
class Simulation_Time
{
    public:
int total_time_steps;
    /**
     * Constructor building a Simulation Time object
     */
    Simulation_Time(simulation_time_params simulation_time_config):start_date_time_epoch(simulation_time_config.start_t),
                                           end_date_time_epoch(simulation_time_config.end_t),
                                           current_date_time_epoch(simulation_time_config.start_t),
                                           output_time_step_seconds(simulation_time_config.output_time_step),
                                           simulation_time_vector_index(-1)
    {
        //read_forcing_aorc(forcing_config.path);

        simulation_total_time_seconds = end_date_time_epoch - start_date_time_epoch;

        total_time_steps = simulation_total_time_seconds / output_time_step_seconds;


    }

/*
    int get_total_time_steps()
    {
        return total_time_steps;
    }
*/
    //int total_time_steps;


    private:

    vector<time_t> simulation_time_epoch_vector; 
    vector<string> simulation_time_string_vector;
    
    int simulation_time_vector_index;

    //int total_time_steps;
    int simulation_total_time_seconds;
    
   
    int output_time_step_seconds; //INCLUDE THIS????


    time_t start_date_time_epoch;
    time_t end_date_time_epoch;
    time_t current_date_time_epoch;


};


#endif // SIMULATION_TIME_H

