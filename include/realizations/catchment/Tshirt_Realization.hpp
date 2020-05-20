#ifndef NGEN_TSHIRT_REALIZATION_HPP
#define NGEN_TSHIRT_REALIZATION_HPP

#include <HY_CatchmentArea.hpp>
#include <unordered_map>
#include "GIUH.hpp"
#include "tshirt/include/Tshirt.h"

namespace realization {

    class Tshirt_Realization : public HY_CatchmentArea {

    public:

        typedef long time_step_t;

        Tshirt_Realization(double soil_storage_meters,
                           double groundwater_storage_meters,
                           unique_ptr<giuh::giuh_kernel> giuh_kernel,
                           tshirt::tshirt_params params,
                           const std::vector<double>& nash_storage,
                           time_step_t t);

        Tshirt_Realization(
                double soil_storage_meters,
                double groundwater_storage_meters,
                unique_ptr<giuh::giuh_kernel> giuh_kernel,
                double maxsmc,
                double wltsmc,
                double satdk,
                double satpsi,
                double slope,
                double b,
                double multiplier,
                double alpha_fc,
                double Klf,
                double Kn,
                int nash_n,
                double Cgw,
                double expon,
                double max_gw_storage,
                const std::vector<double>& nash_storage,
                time_step_t t
                );

        virtual ~Tshirt_Realization();

        double get_response(double input_flux, time_step_t t, const shared_ptr<pdm03_struct>& et_params);

        void add_time(time_t t, double n);

    private:
        std::unordered_map<time_step_t, shared_ptr<tshirt::tshirt_state>> state;
        std::unordered_map<time_step_t, shared_ptr<tshirt::tshirt_fluxes>> fluxes;
        std::unordered_map<time_step_t, std::vector<double> > cascade_backing_storage;
        tshirt::tshirt_params params;
        unique_ptr<tshirt::tshirt_model> model;
        unique_ptr<giuh::giuh_kernel> giuh_kernel;

    };

}


#endif //NGEN_TSHIRT_REALIZATION_HPP
