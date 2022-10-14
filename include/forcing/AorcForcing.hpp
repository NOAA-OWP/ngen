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

// Recognized Forcing Value Names (in particular for use when configuring BMI input variables)
// TODO: perhaps create way to configure a mapping of these to something different
#define AORC_FIELD_NAME_PRECIP_RATE "precip_rate"
#define AORC_FIELD_NAME_SOLAR_SHORTWAVE "DSWRF_surface"
#define AORC_FIELD_NAME_SOLAR_LONGWAVE "DLWRF_surface"
#define AORC_FIELD_NAME_PRESSURE_SURFACE "PRES_surface"
#define AORC_FIELD_NAME_TEMP_2M_AG "TMP_2maboveground"
#define AORC_FIELD_NAME_APCP_SURFACE "APCP_surface"
#define AORC_FIELD_NAME_WIND_U_10M_AG "UGRD_10maboveground"
#define AORC_FIELD_NAME_WIND_V_10M_AG "VGRD_10maboveground"
#define AORC_FIELD_NAME_SPEC_HUMID_2M_AG "SPFH_2maboveground"

#include <map>

//using namespace std// causes build error on gcc-12 with boost::geometry

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
  time_t simulation_start_t;
  time_t simulation_end_t;
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
      this->simulation_start_t = timegm( &tm );

      strptime(this->end_time.c_str(), this->date_format.c_str() , &tm);
      this->simulation_end_t = timegm( &tm );
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

namespace data_access {

    const std::map<std::string, std::tuple<std::string, std::string>> WellKnownFields = {
        {"precip_rate", { CSDMS_STD_NAME_LIQUID_EQ_PRECIP_RATE, "mm s^-1" } }, 
        {"APCP_surface", { CSDMS_STD_NAME_RAIN_VOLUME_FLUX, "kg m^-2" } }, // Especially this one, is it correct? 
        {"DLWRF_surface", { CSDMS_STD_NAME_SOLAR_LONGWAVE, "W m-2" } }, 
        {"DSWRF_surface", { CSDMS_STD_NAME_SOLAR_SHORTWAVE, "W m-2" } }, 
        {"PRES_surface", { CSDMS_STD_NAME_SURFACE_AIR_PRESSURE, "Pa" } }, 
        {"SPFH_2maboveground", { NGEN_STD_NAME_SPECIFIC_HUMIDITY, "kg kg-1" } }, 
        {"TMP_2maboveground", { CSDMS_STD_NAME_SURFACE_TEMP, "K" } }, 
        {"UGRD_10maboveground", { CSDMS_STD_NAME_WIND_U_X, "m s-1" } }, 
        {"VGRD_10maboveground", { CSDMS_STD_NAME_WIND_V_Y, "m s-1" } }, 
        {"RAINRATE", { CSDMS_STD_NAME_LIQUID_EQ_PRECIP_RATE , "mm s^-1" } }, 
        {"T2D", { CSDMS_STD_NAME_SURFACE_TEMP, "K" } }, 
        {"Q2D", { NGEN_STD_NAME_SPECIFIC_HUMIDITY, "kg kg-1" } }, 
        {"U2D", { CSDMS_STD_NAME_WIND_U_X, "m s-1" } }, 
        {"V2D", { CSDMS_STD_NAME_WIND_V_Y, "m s-1" } }, 
        {"PSFC", { CSDMS_STD_NAME_SURFACE_AIR_PRESSURE, "Pa" } }, 
        {"SWDOWN", { CSDMS_STD_NAME_SOLAR_SHORTWAVE, "W m-2" } }, 
        {"LWDOWN", { CSDMS_STD_NAME_SOLAR_LONGWAVE, "W m-2" } }
    };

}

#endif // NGEN_AORCFORCING_H
