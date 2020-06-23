#ifndef NGEN_TSHIRTERRORCODES_H
#define NGEN_TSHIRTERRORCODES_H

namespace tshirt {

    // TODO: consider combining with or differentiating from similar hymod enum

    /**
     * An enumeration of codes indicating possible error states (or lack thereof) when running the Tshirt model.
     */
    enum TshirtErrorCodes {
        TSHIRT_NO_ERROR = 0,
        TSHIRT_MASS_BALANCE_ERROR = 100
    };
}

#endif //NGEN_TSHIRTERRORCODES_H
