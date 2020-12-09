#ifndef NGEN_LSTM_STATE_H
#define NGEN_LSTM_STATE_H

namespace lstm {

     /**
      * Structure for containing and organizing the state values of the lstm hydrological model.
      */
    struct lstm_state {
        // TODO: confirm this is correct
        double soil_storage_meters;              //!< current water storage in soil column nonlinear reservoir ("Ss")
        //double groundwater_storage_meters;       //!< current water storage in ground water nonlinear reservoir ("Sgw")
        //vector<double> nash_cascade_storeage_meters;    //!< water storage in nonlinear reservoirs of Nash Cascade for lateral subsurface flow

        // I think this doesn't belong in state, and so is just in run() in the lstm::lstm_model class
        //double column_total_soil_moisture_deficit;    //!< soil column total moisture deficit

/*
        lstm_state(double soil_storage_meters = 0.0, double groundwater_storage_meters = 0.0,
                     vector<double> nash_cascade_storeage_meters = vector<double>())
                : soil_storage_meters(soil_storage_meters),
                  groundwater_storage_meters(groundwater_storage_meters),
                  nash_cascade_storeage_meters(std::move(nash_cascade_storeage_meters)) {}
*/


        lstm_state(double soil_storage_meters = 0.0)
                : soil_storage_meters(soil_storage_meters) {}


    };
}

#endif //NGEN_LSTM_STATE_H
