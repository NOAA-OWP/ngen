#ifndef NGEN_TSHIRT_C_REALIZATION_HPP
#define NGEN_TSHIRT_C_REALIZATION_HPP

#include "core/catchment/HY_CatchmentArea.hpp"
#include "tshirt_params.h"
#include "forcing/Forcing.h"
#include "GiuhJsonReader.h"
#include "tshirt_c.h"
#include <memory>
#include <unordered_map>

using namespace tshirt;

namespace realization {

    class Tshirt_C_Realization : public HY_CatchmentArea {

    public:

        typedef long time_step_t;

        Tshirt_C_Realization(forcing_params forcing_config,
                             double soil_storage_meters,
                             double groundwater_storage_meters,
                             std::string catchment_id,
                             giuh::GiuhJsonReader &giuh_json_reader,
                             tshirt::tshirt_params params,
                             const std::vector<double> &nash_storage);

        Tshirt_C_Realization(
                forcing_params forcing_config,
                double soil_storage_meters,
                double groundwater_storage_meters,
                std::string catchment_id,
                giuh::GiuhJsonReader &giuh_json_reader,
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
                const std::vector<double> &nash_storage);

        virtual ~Tshirt_C_Realization();

    private:
        std::string catchment_id;
        // TODO: note sure if these are needed
        /*
        std::unordered_map<time_step_t, shared_ptr<tshirt::tshirt_state>> state;
        std::unordered_map<time_step_t, shared_ptr<tshirt::tshirt_fluxes>> fluxes;
        std::unordered_map<time_step_t, std::vector<double> > cascade_backing_storage;
        */

        tshirt::tshirt_params params;
        NWM_soil_parameters c_soil_params;

        // TODO: this needs to be something else
        //std::unique_ptr<tshirt::tshirt_model> model;

        // Still need this for getting at the ordinates
        std::shared_ptr<giuh::giuh_kernel> giuh_kernel;


        // TODO: remember to do array conversion in function calls
        std::vector<double> nash_storage;

        aorc_forcing_data c_aorc_params;

        // TODO: might want to consider having an initial time step value for reference (implied size is 1 hour)
        // TODO: this probably need to be converted to use a different fluxes type that can be dealt with externally
        std::unordered_map<time_step_t, std::shared_ptr<tshirt_c_result_fluxes>> fluxes;

        // TODO: rename once setup complete (easier to refactor then)
        conceptual_reservoir groundwater_conceptual_reservoir;
        conceptual_reservoir soil_conceptual_reservoir;

    };
}

#endif