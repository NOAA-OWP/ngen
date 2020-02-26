#ifndef SIMPLE_LUMPED_MODEL_REALIZATION_H
#define SIMPLE_LUMPED_MODEL_REALIZATION_H

#include <HY_CatchmentArea.h>
#include "LinearReservoir.h"
#include "models/hymod/include/Hymod.h"

#include <unordered_map>


class Simple_Lumped_Model_Realization : public HY_CatchmentArea
{
    public:

        typedef long time_step_t;

        Simple_Lumped_Model_Realization(
            double storage,
            double max_storage,
            double a,
            double b,
            double Ks,
            double Kq,
            long n,
            const std::vector<double>& Sr,
            time_step_t t
        );

        virtual ~Simple_Lumped_Model_Realization();

        double get_response(double input_flux, time_step_t t, void* et_params);
        double calc_et(double soim_m, void* et_params);

        void add_time(time_t t, double n);

    protected:

    private:


        std::unordered_map<time_step_t, hymod_state> state;
        std::unordered_map<time_step_t, hymod_fluxes> fluxes;
        std::unordered_map<time_step_t, std::vector<double> > cascade_backing_storage;
        hymod_params params;



};

#endif // SIMPLE_LUMPED_MODEL_REALIZATION_H
