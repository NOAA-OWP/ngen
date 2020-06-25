#ifndef PDM03_H_INCLUDED
#define PDM03_H_INCLUDED

#include <cstdlib>
#include <cmath>

using std::max;
using std::min;

extern "C"
{
    struct pdm03_struct
    {
        int model_time_step;                             //int modelDay;
        double maximum_combined_contents;                //double Cpar;
        double scaled_distribution_fn_shape_parameter;   //double B;
        double final_height_reservoir;                   //double XHuz
        double max_height_soil_moisture_storerage_tank;  //double Huz;
        double total_effective_rainfall;                 //double OV;
        double actual_et;                                //double AE;
        double final_storage_upper_zone;                 //double XCuz;
        double precipitation;                            //double effPrecip;
        double potential_et;                             //double PE;
        double vegetation_adjustment;                    //double Kv;
    };

    void Pdm03(int model_time_step,
               double maximum_combined_contents,
               double scaled_distribution_fn_shape_parameter,
               double *final_height_reservoir,
               double max_height_soil_moisture_storerage_tank,
               double *total_effective_rainfall,
               double *actual_et,
               double *final_storage_upper_zone,
               double precipitation,
               double potential_et,
               double vegetation_adjustment);

    /*
    ========================================================================================================================================
    Code to run Soil Moisture Accounting model
    INPUTS
        model_time_step                          - model time step  // modelDay
        maximum_combined_contents                - maximum combined contents of all stores, calculated using Huz and B  // Cpar [mm]
        scaled_distribution_fn_shape_parameter   - scaled distribution function shape parameter (0-2)  // B [unitless]
        max_height_soil_moisture_storerage_tank  - maximum height of the soil moisture storage tank. (0-inf.)  // Huz [mm]
        precipitation                            - precipitation this time step  // effPrecip [mm/day]
        potential_et                             - potential evapotranspiration this time step  // PE [mm/day]
        vegetation_adjustment                    - vegetation adjustment to PE (0-1)  // Kv [unitless]
    OUTPUTS
        total_effective_rainfall                 - total effective rainfall (effective runoff?) this time step  // OV [mm]
        actual_et                                - actual evapotranspiration this time step  // AE [mm/day]
        final_storage_upper_zone                 - final storage in the upper zone after ET at end of this time step  // Xcuz [mm]
        final_height_reservoir                   - final height of the reservoir at end of this time step  // XHuz [mm]
    ========================================================================================================================================
    */

    inline void Pdm03(int model_time_step,
                      double maximum_combined_contents,
                      double scaled_distribution_fn_shape_parameter,
                      double *final_height_reservoir,
                      double max_height_soil_moisture_storerage_tank,
                      double *total_effective_rainfall,
                      double *actual_et,
                      double *final_storage_upper_zone,
                      double precipitation,
                      double potential_et,
                      double vegetation_adjustment)
    {
      /* local variables */
      //double Cbeg, OV2, PPinf, Hint, Cint, OV1;
      double cbeg, ov2, ppinf, hint, cint, ov1;

      //Storage contents at begining
      //Cbeg = Cpar * (1.0 - pow(1.0-(*XHuz/Huz),1.0+B));
      cbeg = maximum_combined_contents * (1.0 - pow(1.0-(*final_height_reservoir/max_height_soil_moisture_storerage_tank),
                                                    1.0+scaled_distribution_fn_shape_parameter));

      //Compute effective rainfall filling all storage elements
      //OV2 = max(0.0, effPrecip + *XHuz - Huz);
      ov2 = max(0.0, precipitation + *final_height_reservoir - max_height_soil_moisture_storerage_tank);

      //Remaining net rainfall
      //PPinf = effPrecip - OV2;
      ppinf = precipitation - ov2;

      //New actual height
      //Hint = min(Huz, *XHuz+PPinf);
      hint = min(max_height_soil_moisture_storerage_tank, *final_height_reservoir+ppinf);

      //New storage content
      //Cint = Cpar*(1.0-pow(1.0-(Hint/Huz),1.0+B));
      cint = maximum_combined_contents*(1.0-pow(1.0-(hint/max_height_soil_moisture_storerage_tank),
                                                1.0+scaled_distribution_fn_shape_parameter));

      //Additional effective rainfall produced by stores smaller than Cmax
      //OV1 = max(0.0, PPinf + Cbeg - Cint);
      ov1 = max(0.0, ppinf + cbeg - cint);

      //Compute total effective rainfall
      //*OV = OV1 + OV2;
      *total_effective_rainfall = ov1 + ov2;

      //Compute actual evapotranspiration
      //*AE = min(Cint,(Cint/Cpar)*PE*Kv);
      *actual_et = min(cint,(cint/maximum_combined_contents)*potential_et*vegetation_adjustment);

      //Storage contents after ET
      //*XCuz = max(0.0,Cint - *AE);
      *final_storage_upper_zone = max(0.0,cint - *actual_et);

      //Compute final height of the reservoir
      //*XHuz = Huz*(1.0-pow(1.0-(*XCuz/Cpar),1.0/(1.0+B)));
      *final_height_reservoir = max_height_soil_moisture_storerage_tank*
              (1.0-pow(1.0-(*final_storage_upper_zone/maximum_combined_contents),
               1.0/(1.0+scaled_distribution_fn_shape_parameter)));

      return;

    }

    inline void pdm03_wrapper(pdm03_struct* pdm_data)
    {
        return Pdm03(pdm_data->model_time_step,
                     pdm_data->maximum_combined_contents,
                     pdm_data->scaled_distribution_fn_shape_parameter,
                     &pdm_data->final_height_reservoir,
                     pdm_data->max_height_soil_moisture_storerage_tank,
                     &pdm_data->total_effective_rainfall,
                     &pdm_data->actual_et,
                     &pdm_data->final_storage_upper_zone,
                     pdm_data->precipitation,
                     pdm_data->potential_et,
                     pdm_data->vegetation_adjustment);
    }


    /*
    ==================================================================================================
    Hamon Potential ET model
    INPUTS
        average_temp_degree_celcius           - average air temperature this time step (C)
        latitude_of_catchment_centoid_degree  - latitude of catchment centroid (degrees)
        day_of_year                           - (1-365) or (1-366) in leap years
    OUTPUTS
        potential_et                          - potential evapotranspiration this time step [mm/day]
    ==================================================================================================
    */

    inline double calculate_hamon_pet(double average_temp_degree_celcius,
                                      double latitude_of_catchment_centoid_degree,
                                      int day_of_year)
    {
        /* local variables
        solar_declination          - [radian]
        day_length                 - [hours]
        saturation_vapor_pressure  - [kPa]
        potential_et               - potential evapotranspiration this time step [mm/day]
        */
        double solar_declination, day_length, saturation_vapor_pressure, potential_et;

        solar_declination =
            asin(0.39795*cos(0.2163108 + 2.0 * atan(0.9671396*tan(0.00860*
                                                (double)(day_of_year-186)))));
        day_length = 24.0 - (24.0/M_PI)*(acos((sin(0.8333*M_PI/180.0)+
                    sin(latitude_of_catchment_centoid_degree*M_PI/180.0)*
                    sin(solar_declination))/
                    (cos(latitude_of_catchment_centoid_degree*M_PI/180.0)*
                    cos(solar_declination))));
        saturation_vapor_pressure = 0.6108*exp((17.27*average_temp_degree_celcius)/(237.3+average_temp_degree_celcius));
        potential_et = (715.5*day_length/24.0)*saturation_vapor_pressure/(average_temp_degree_celcius+ 273.2);

        return potential_et;
    }

     /*
     * convert Gregorian days to Julian date
     *
     * Modify as needed for your application.
     *
     * The Julian day starts at noon of the Gregorian day and extends
     * to noon the next Gregorian day.
     *
     */
    /*
    ** Takes a date, and returns a Julian day. A Julian day is the number of
    ** days since some base date  (in the very distant past).
    ** Handy for getting date of x number of days after a given Julian date
    ** (use jdate to get that from the Gregorian date).
    ** Author: Robert G. Tantzen, translator: Nat Howard
    ** Translated from the algol original in Collected Algorithms of CACM
    ** (This and jdate are algorithm 199).
    */


    inline double greg_2_jul(long year, long mon, long day, long h, long mi, double se)
    {
        long m = mon, d = day, y = year;
        long c, ya, j;
        double seconds = h * 3600.0 + mi * 60 + se;

        if (m > 2)
        m -= 3;
        else {
        m += 9;
        --y;
        }
        c = y / 100L;
        ya = y - (100L * c);
        j = (146097L * c) / 4L + (1461L * ya) / 4L + (153L * m + 2L) / 5L + d + 1721119L;
        if (seconds < 12 * 3600.0) {
        j--;
        seconds += 12.0 * 3600.0;
        }
        else {
        seconds = seconds - 12.0 * 3600.0;
        }
        return (j + (seconds / 3600.0) / 24.0);
    }

    /* Julian date converter. Takes a julian date (the number of days since
    ** some distant epoch or other), and returns an int pointer to static space.
    ** ip[0] = month;
    ** ip[1] = day of month;
    ** ip[2] = year (actual year, like 1977, not 77 unless it was  77 a.d.);
    ** ip[3] = day of week (0->Sunday to 6->Saturday)
    ** These are Gregorian.
    ** Copied from Algorithm 199 in Collected algorithms of the CACM
    ** Author: Robert G. Tantzen, Translator: Nat Howard
    **
    **
    */
    inline void calc_date(double jd, long *y, long *m, long *d, long *h, long *mi,
                          double *sec)
    {
        static int ret[4];

        long j;
        double tmp;
        double frac;

        j=(long)jd;
        frac = jd - j;

        if (frac >= 0.5) {
        frac = frac - 0.5;
            j++;
        }
        else {
        frac = frac + 0.5;
        }

        ret[3] = (j + 1L) % 7L;
        j -= 1721119L;
        *y = (4L * j - 1L) / 146097L;
        j = 4L * j - 1L - 146097L * *y;
        *d = j / 4L;
        j = (4L * *d + 3L) / 1461L;
        *d = 4L * *d + 3L - 1461L * j;
        *d = (*d + 4L) / 4L;
        *m = (5L * *d - 3L) / 153L;
        *d = 5L * *d - 3 - 153L * *m;
        *d = (*d + 5L) / 5L;
        *y = 100L * *y + j;
        if (*m < 10)
        *m += 3;
        else {
        *m -= 9;
        *y=*y+1;
        }

        tmp = 3600.0 * (frac * 24.0);
        *h = (long) (tmp / 3600.0);
        tmp = tmp - *h * 3600.0;
        *mi = (long) (tmp / 60.0);
        *sec = tmp - *mi * 60.0;
    }

}


#endif // PDM03_H_INCLUDED
