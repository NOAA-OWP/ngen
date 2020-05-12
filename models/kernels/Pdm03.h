
#ifndef PDM03_H
#define PDM03_H

#include <cstdlib>

using std::max;
using std::min;

extern "C"
{
    struct pdm03_struct
    {
        int modelDay;
        double Cpar;
        double B;
        double XHuz;
        double Huz;
        double OV;
        double AE;
        double XCuz;
        double effPrecip;
        double PE;
        double Kv;
    };

    void Pdm03(int modelDay,
        double Cpar,
        double B,
        double *XHuz,
        double Huz,
        double *OV,
        double *AE,
        double *XCuz,
        double effPrecip,
        double PE,
        double Kv);

    /*
    %%=========================================================================
    %% Code to run Soil Moisture Accounting model
    %% INPUTS
    %%   modelDay   - model time step
    %%   Cpar       - maximum combined contents of all stores, calculated using Huz and B
    %%   B          - scaled distribution function shape parameter (0-2)
    %%   Huz        - maximum hight of the soil moisture storage tank. (0-inf.)
    %%   effPrecip  - precipitation this time step
    %%   PE         - potential evapotranspiration this time step
    %%   Kv         - vegetation adjustment to PE (0-1)
    % OUTPUTS
    %%   OV  - total effective rainfall (effective runoff?) this time step
    %%   AE  - actual evapotranspiration this time step
    %%   XCuz - final storage in the upper zone after ET at end of this time step
    %%   XHuz - final height of the reservoir at end of this time step
    %%=========================================================================
    */

    inline void Pdm03(int modelDay, double Cpar, double B, double *XHuz, double Huz,
              double *OV, double *AE, double *XCuz, double effPrecip, double PE, double Kv)
    {
      /* local variables */
      double Cbeg, OV2, PPinf, Hint, Cint, OV1;

      //Storage contents at begining
      Cbeg = Cpar * (1.0 - pow(1.0-(*XHuz/Huz),1.0+B));

      //Compute effective rainfall filling all storage elements
      OV2 = max(0.0, effPrecip + *XHuz - Huz);

      //Remaining net rainfall
      PPinf = effPrecip - OV2;

      //New actual height
      Hint = min(Huz, *XHuz+PPinf);

      //New storage content
      Cint = Cpar*(1.0-pow(1.0-(Hint/Huz),1.0+B));

      //Additional effective rainfall produced by stores smaller than Cmax
      OV1 = max(0.0, PPinf + Cbeg - Cint);

      //Compute total effective rainfall
      *OV = OV1 + OV2;

      //Compute actual evapotranspiration
      *AE = min(Cint,(Cint/Cpar)*PE*Kv);

      //Storage contents after ET
      *XCuz = max(0.0,Cint - *AE);

      //Compute final height of the reservoir
      *XHuz = Huz*(1.0-pow(1.0-(*XCuz/Cpar),1.0/(1.0+B)));

      return;

    }

    inline void pdm03_wrapper(pdm03_struct* pdm_data)
    {
        return Pdm03(pdm_data->modelDay,
        pdm_data->Cpar,
        pdm_data->B,
        &pdm_data->XHuz,
        pdm_data->Huz,
        &pdm_data->OV,
        &pdm_data->AE,
        &pdm_data->XCuz,
        pdm_data->effPrecip,
        pdm_data->PE,
        pdm_data->Kv);
    }

    /*
    %%=========================================================================
    %% Hamon Potential ET model
    %% INPUTS
    %%   avgTemp      - average air temperature this time step (C)
    %%   Latitude     - latitude of catchment centroid (degrees)
    %%   day of year  - (1-365) or (1-366) in leap years
    % OUTPUTS
    %%   PE           - potential evapotranspiration this time step
    %%=========================================================================
    */

    inline double calculateHamonPE(double avgTemp, double Latitude, int day_of_year)
    {
        double solarDeclination, dayLength, eStar, PE;

        solarDeclination =
            asin(0.39795*cos(0.2163108 + 2.0 * atan(0.9671396*tan(0.00860*
                                                (double)(day_of_year-186)))));
        dayLength = 24.0 - (24.0/M_PI)*(acos((sin(0.8333*M_PI/180.0)+
                    sin(Latitude*M_PI/180.0)*sin(solarDeclination))/
                    (cos(Latitude*M_PI/180.0)*cos(solarDeclination))));
        eStar = 0.6108*exp((17.27*avgTemp)/(237.3+avgTemp));
        PE = (715.5*dayLength/24.0)*eStar/(avgTemp + 273.2);

        return PE;
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


#endif // PDM03_H
