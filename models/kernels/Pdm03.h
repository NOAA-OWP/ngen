#ifndef PDM03_H_INCLUDED
#define PDM03_H_INCLUDED

#include <cstdlib>

using std::max;
using std::min;

extern "C"
{
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

    void Pdm03(int modelDay, double Cpar, double B, double *XHuz, double Huz,
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

}


#endif // PDM03_H_INCLUDED
