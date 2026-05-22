#ifndef __NGEN_LAYER_DATA__
#define __NGEN_LAYER_DATA__

#include <unordered_map>
#include <string>
#include <vector>

namespace ngen
{
    /**
     * @brief A simple structure to hold meta data related  to a computational layer
     * 
     * This structure is used to hold information about a layers time_step, include value and units
     * as well as the numeric id that is used to store and retrieve the layer 
    */
    
    struct LayerDescription
    {
        std::string name;             //> The name of the computational layer
        std::string time_step_units;  //> The time units that time_step is expressed in
        int id;                       //> The layer id which determines the order that this layer will be processed in
        double time_step;             //> The time step that will be used for models in this layer
    };
    
    /**
     * @brief A class to hold and manage layer metadata for the system
     * 
     * Layer can be accessed by interger id the default valid range is -20 .. 80
    */

    class LayerDataStorage
    {
        public:

        LayerDataStorage() {}

        ~LayerDataStorage() {}

        /**
         * @brief return the layer object at this numeric id
        */

        const LayerDescription& get_layer(int id) const 
        { 
            return stored_layers.at(id); 
        }

        /** 
         * @brief store a new layer description in the indicated id slot
        */

        void put_layer(const LayerDescription& desc, int id) 
        {
             stored_layers[id] = desc;
             keys.push_back(id);
        }

        /***
         * @brief check to see if a layer has been associated with an id number
        */

        bool exists(int id) 
        { 
            return stored_layers.find(id) != stored_layers.end(); 
        }

        std::vector<int>& get_keys() { return keys; }

        private:

        std::unordered_map<int,LayerDescription> stored_layers;   //< Map object that is used to store layer data objects
        std::vector<int> keys;                                    //< A list of the currently valid map keys
    };
}

#endif 