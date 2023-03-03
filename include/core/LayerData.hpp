#ifndef __NGEN_LAYER_DATA__
#define __NGEN_LAYER_DATA__

#include <unordered_map>
#include <string>

namespace ngen
{
    /**
     * @brief A simple structure to hold meta data realated to a computational layer
    */
    
    struct LayerDescription
    {
        std::string name;             //> The name of the computational layer
        std::string time_step_units;  //> The time units that time_step is expressed in
        int id;                       //> The layer id which determins the order that this layer will be processed in
        double time_step;             //> The time step that will be used for models in this layer
    };
    
    /**
     * @brief A class to hold a manage layer metadata for the system
    */

    class LayerDataStorage
    {
        public:

        LayerDataStorage() {}

        ~LayerDataStorage() {}

        const LayerDescription& get_layer(int id) const { return stored_layers.at(id); }

        void put_layer(const LayerDescription& desc, int id) { stored_layers[id] = desc;}

        bool exists(int id) { return stored_layers.find(id) != stored_layers.end(); }

        private:

        std::unordered_map<int,LayerDescription> stored_layers;
    };
}

#endif 