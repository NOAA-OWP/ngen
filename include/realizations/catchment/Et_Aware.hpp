#ifndef NGEN_ET_AWARE_H
#define NGEN_ET_AWARE_H

#include <memory>
#include "Pdm03.h"

namespace realization {
    /**
     * Interface for type that is aware of parameters for evapotranspiration in certain calculations, and can accept
     * such parameters being provided.
     *
     * It is not necessarily the case that implementations use ET params in any way, only that they provide an interface
     * for accepting them.  As such, it is not necessarily the case that implementations maintain any provided params
     * as part of their state.  The details of how this is handled should be documented for individual subtypes.
     */
    class Et_Aware {
    public:
        Et_Aware() {}

        ~Et_Aware() {}

        /**
         * Test whether ET params have been set.
         *
         * @return Whether ET params have been set.
         */
        virtual bool is_et_params_set() = 0;

        /**
         * Accept a reference for an appropriate object for holding ET parameters for use when executing the model
         * calculations, if maintained by the type.
         */
        virtual void set_et_params(std::shared_ptr<pdm03_struct> params) = 0;
    };
}

#endif //NGEN_ET_AWARE_H
