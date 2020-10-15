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
  /*
    Constructor for simulation_time_params
  */
  simulation_time_params(std::string start_time, std::string end_time):
    start_time(start_time), end_time(end_time)
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

#endif // SIMULATION_TIME_H

