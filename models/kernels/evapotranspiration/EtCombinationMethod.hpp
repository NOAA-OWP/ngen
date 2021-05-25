#ifndef ET_COMBINATION_METHOD_H
#define ET_COMBINATION_METHOD_H

#include "EtStruct.h"

// FUNCTION AND SUBROUTINE PROTOTYPES

namespace et {
    namespace combined {
        double evapotranspiration_combination_method
                (
                        evapotranspiration_options *et_options,
                        evapotranspiration_params *et_params,
                        evapotranspiration_forcing *et_forcing,
                        intermediate_vars *inter_vars
                );
    }
}

#endif // ET_COMBINATION_METHOD_H
