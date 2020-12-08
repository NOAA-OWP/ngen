#ifndef NGEN_LSTM_FLUXES_H
#define NGEN_LSTM_FLUXES_H

namespace lstm {

     /**
      * Structure for containing and organizing the calculated flux values of the lstm hydrological model.
      */
    struct lstm_fluxes {
        double flow;

        lstm_fluxes(double f)
                : flow(f) {
        }
        lstm_fluxes()
        {
          flow = 0.0;
        }
    };


}

#endif //NGEN_LSTM_FLUXES_H
