#ifndef NGEN_ET_ACCOUNTABLE_HPP
#define NGEN_ET_ACCOUNTABLE_HPP

#include <memory>
#include "Et_Aware.hpp"

namespace realization {
    /**
     * Interface for type that is able to account for evapotranspiration in certain calculations, and thus maintains a
     * set of parameters for ET.
     */
    class Et_Accountable : public virtual Et_Aware {
    public:

        Et_Accountable() {}

        ~Et_Accountable() {}

        virtual double calc_et() = 0;

        /**
         * Get a reference to the appropriate object holding ET parameters for use when executing the model
         * calculations.
         *
         * @return A reference object for model ET parameters.
         */
        virtual const pdm03_struct &get_et_params() {
            return *et_params;
        }

        /**
         * Test whether ET params have been set.
         *
         * @return Whether ET params have been set.
         */
        bool is_et_params_set() override {
            return et_params != nullptr;
        }

        /**
         * Set a reference for an appropriate object for holding ET parameters for use when executing the model
         * calculations.
         */
        void set_et_params(std::shared_ptr<pdm03_struct> params) override {
            et_params = params;
        }

    protected:

        virtual std::shared_ptr<pdm03_struct> get_et_params_ptr() {
            return et_params;
        }

    private:
        std::shared_ptr<pdm03_struct> et_params;

    };

}

#endif //NGEN_ET_ACCOUNTABLE_HPP
