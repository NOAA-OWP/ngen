#ifndef NGEN_AORCFORCING_H
#define NGEN_AORCFORCING_H

// CSDMS Standard Names for several forcings
#define CSDMS_STD_NAME_RAIN_VOLUME_FLUX "atmosphere_water__rainfall_volume_flux"
#define CSDMS_STD_NAME_PRECIP_MASS_FLUX "atmosphere_water__precipitation_mass_flux"
#define CSDMS_STD_NAME_SOLAR_LONGWAVE "land_surface_radiation~incoming~longwave__energy_flux"
#define CSDMS_STD_NAME_SOLAR_SHORTWAVE "land_surface_radiation~incoming~shortwave__energy_flux"
#define CSDMS_STD_NAME_SURFACE_AIR_PRESSURE "land_surface_air__pressure"
#define CSDMS_STD_NAME_HUMIDITY "atmosphere_air_water~vapor__relative_saturation"
#define CSDMS_STD_NAME_LIQUID_EQ_PRECIP_RATE "atmosphere_water__liquid_equivalent_precipitation_rate"
#define CSDMS_STD_NAME_SURFACE_TEMP "land_surface_air__temperature"
#define CSDMS_STD_NAME_WIND_U_X "land_surface_wind__x_component_of_velocity"
#define CSDMS_STD_NAME_WIND_V_Y "land_surface_wind__y_component_of_velocity"
#define NGEN_STD_NAME_SPECIFIC_HUMIDITY "atmosphere_air_water~vapor__relative_saturation" // This is not present in standard names, use this for now... may change!

using namespace std;

/**
 * @brief forcing_params providing configuration information for forcing time period and source.
 */
struct forcing_params
{
  std::string path;
  std::string start_time;
  std::string end_time;
  std::string date_format =  "%Y-%m-%d %H:%M:%S";
  std::string provider;
  time_t start_t;
  time_t end_t;
  /*
    Constructor for forcing_params
  */
  forcing_params(std::string path, std::string provider, std::string start_time, std::string end_time):
    path(path), provider(provider), start_time(start_time), end_time(end_time)
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

//AORC Forcing Data Struct
struct AORC_data
{
  double APCP_surface_kg_per_meters_squared; //Total Precipitation (kg/m^2)
  double DLWRF_surface_W_per_meters_squared; //Downward Long-Wave Rad. (Flux W/m^2)
  double DSWRF_surface_W_per_meters_squared; //Downward Short-Wave Radiation (Flux W/m^2)
  double PRES_surface_Pa; //Pressure (Pa)
  double SPFH_2maboveground_kg_per_kg; //Specific Humidity (kg/kg)
  double TMP_2maboveground_K; //Temperature (K)
  double UGRD_10maboveground_meters_per_second; //U-Component of Wind (m/s)
  double VGRD_10maboveground_meters_per_second; //V-Component of Wind (m/s)
};


#endif // NGEN_AORCFORCING_H