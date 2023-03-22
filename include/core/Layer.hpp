#ifndef __NGEN_LAYER__
#define __NGEN_LAYER__

#include "LayerData.hpp"
#include "Simulation_Time.h"

namespace ngen
{
    class Layer
    {
        public:

        Layer(const LayerDescription& desc, const std::vector<std::string>& p_u, const Simulation_Time_Object& s_t) :
            description(desc),
            processing_units(p_u),
            simulation_time(s_t)
        {

        }



        private:

        LayerDescription description;
        std::vector<std::string> processing_units;
        Simulation_Time_Object simulation_time;

    };
}

#endif