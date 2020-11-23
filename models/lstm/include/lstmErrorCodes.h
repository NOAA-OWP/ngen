#ifndef NGEN_LSTMERRORCODES_H
#define NGEN_LSTMERRORCODES_H

namespace lstm {

    // TODO: consider combining with or differentiating from similar hymod enum

    /**
     * An enumeration of codes indicating possible error states (or lack thereof) when running the LSTM model.
     */
    enum lstmErrorCodes {
        LSTM_NO_ERROR = 0,
        LSTM_MASS_BALANCE_ERROR = 100
    };
}

#endif //NGEN_LSTMERRORCODES_H
