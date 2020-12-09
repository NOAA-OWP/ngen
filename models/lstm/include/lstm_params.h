#ifndef NGEN_LSTM_PARAMS_H
#define NGEN_LSTM_PARAMS_H

namespace lstm {

    /**
     * Structure for containing and organizing the parameters of the lstm hydrological model.
     */
    struct lstm_params {
        double latitude;
        double longitude;
        double area;

        /**
         * Constructor for instances, initializing members that correspond one-to-one with a parameter and deriving
         * remaining (non-const and hard-coded) members.
         *
         * @param lat Initialization param for latitude member.
         * @param lon Initialization param for longitude member.
         * @param area Initialization param for basin area member (in FIXME units).
         */
        lstm_params(double lat, double lon, double area) :
              latitude(lat),
              longitude(lon),
              area(area)
              {}
        lstm_params()
          : latitude(0), longitude(0), area(0)
        {}
    };
}

#endif //NGEN_LSTM_PARAMS_H
