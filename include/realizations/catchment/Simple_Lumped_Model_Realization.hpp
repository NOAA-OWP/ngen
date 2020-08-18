#ifndef SIMPLE_LUMPED_MODEL_REALIZATION_H
#define SIMPLE_LUMPED_MODEL_REALIZATION_H

#include "Catchment_Formulation.hpp"
#include "reservoir/Reservoir.hpp"
#include "hymod/include/Hymod.h"
#include <unordered_map>


class Simple_Lumped_Model_Realization : public realization::Catchment_Formulation
{
    public:

        typedef long time_step_t;

        Simple_Lumped_Model_Realization(
            std::string id,
            forcing_params forcing_config,
            utils::StreamHandler output_stream,
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

        Simple_Lumped_Model_Realization(std::string id, forcing_params forcing_config, utils::StreamHandler output_stream) : Catchment_Formulation(id, forcing_config, output_stream) {};

        Simple_Lumped_Model_Realization(std::string id) : Catchment_Formulation(id){};

        /**
         * @brief Explicit move constructor
         * This constuctor explicitly moves a Simple_Lumped_Model_Realization
         * and is required to properly move the HY_CatchmentRealization forcing object
         */
        Simple_Lumped_Model_Realization(Simple_Lumped_Model_Realization &&);
        /**
         * @brief Explicit copy constructor
         * This constuctor explicitly copies Simple_Lumped_Model_Realization
         * and is required to properly copy the HY_CatchmentRealization forcing object
         * as well connectet the hymod_state.Sr* to the copied cascade_backing_storage vector
         */
        Simple_Lumped_Model_Realization(const Simple_Lumped_Model_Realization &);
        virtual ~Simple_Lumped_Model_Realization();

        double get_response(double input_flux, time_step_t t, time_step_t dt, void* et_params);
        double calc_et(double soim_m, void* et_params);

        virtual void create_formulation(boost::property_tree::ptree &config, geojson::PropertyMap *global = nullptr);

        virtual std::string get_formulation_type() {
            return "simple_lumped";
        }

        void add_time(time_t t, double n);

    protected:
        std::string REQUIRED_PARAMETERS[9] = {
            "sr",
            "storage",
            "max_storage",
            "a",
            "b",
            "Ks",
            "Kq",
            "n",
            "t"
        };

        virtual std::string* get_required_parameters() {
            return this->REQUIRED_PARAMETERS;
        }

    private:

        std::unordered_map<time_step_t, hymod_state> state;
        std::unordered_map<time_step_t, hymod_fluxes> fluxes;
        std::unordered_map<time_step_t, std::vector<double> > cascade_backing_storage;
        hymod_params params;



};

#endif // SIMPLE_LUMPED_MODEL_REALIZATION_H
