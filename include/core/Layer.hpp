#ifndef __NGEN_LAYER__
#define __NGEN_LAYER__

#include "LayerData.hpp"
#include "Simulation_Time.hpp"
#include "State_Exception.hpp"

#ifdef NGEN_MPI_ACTIVE
#include "HY_Features_MPI.hpp"
#else
#include "HY_Features.hpp"
#endif

#include "OutputWriter.hpp"

namespace ngen
{
    
    /**
     * @brief an enumeration of the types of layer classes
     */

    enum LayerClass
    {
        kLayer = 1,
        kSurfaceLayer = 2,
        kDomainLayer = 3,
        kCatchmentLayer = 4,
        kNexusLayer = 5,
        kRoutingLayer = 6,
        kOverlayLayer = 7
    };


    class Layer
    {    
        public:

        #ifdef NGEN_MPI_ACTIVE
            using feature_type = hy_features::HY_Features_MPI;
        #else
            using feature_type = hy_features::HY_Features;
        #endif

        Layer(
                const LayerDescription& desc, 
                const std::vector<std::string>& p_u, 
                const Simulation_Time& s_t, 
                feature_type& f, 
                geojson::GeoJSON cd, 
                long idx) :
            description(desc),
            processing_units(p_u),
            simulation_time(s_t),
            features(f),
            catchment_data(cd),
            output_time_index(idx)
        {

        }

        /**
         * @brief Construct a minimum layer object
         * 
         * @param desc 
         * @param s_t 
         * @param f 
         * @param idx 
         */
        Layer(
                const LayerDescription& desc, 
                const Simulation_Time& s_t, 
                feature_type& f,
                long idx) :
            description(desc),
            simulation_time(s_t),
            features(f),
            output_time_index(idx)
        {

        }

        virtual ~Layer() {}

        /**
        * @brief Get a class id for this layer object
        * 
        * @return int
        */
        virtual int class_id() { return LayerClass::kLayer; }

        /**
         * @brief Get a list of output variables names for this layer
         * 
         * @return vector of output variable names
        */

        /***
         * @brief Return the next timestep that will be processed by this layer in epoch time units
        */
        time_t next_timestep_epoch_time() { return simulation_time.next_timestep_epoch_time(); }


        /***
         * @brief Return the last timestep that was processed by this layer in epoch time units
        */
        time_t current_timestep_epoch_time() { return simulation_time.get_current_epoch_time(); }


        /***
         * @brief Return the numeric id of this layer
        */
        int get_id() const { return this->description.id; }

        /***
         * @brief Return the name of this layer
        */
        const std::string& get_name() const { return this->description.name; }

        /***
         * @brief Return this time_step interval used for this layer
        */
        const double get_time_step() const { return this->description.time_step; }

        /***
         * @brief Return the units for the time_step value of this layer
        */
        const std::string& get_time_step_units() const { return this->description.time_step_units; }

        /**
         * @brief Return the contained ids for this layer
        */

        const std::vector<std::string>& get_contents() { return processing_units; }

        /**
         * @brief Return the Formulation for an id
        */

        std::shared_ptr<HY_CatchmentRealization> get_realization(const std::string& id)
        {
            return features.catchment_at(id);
        }

        /***
         * @brief Run one simulation timestep for each model in this layer
        */

        virtual void update_models(std::shared_ptr<data_output::OutputWriter> writer);


        protected:

        const LayerDescription description;
        //TODO is this really required at the top level?
        //See "minimum" constructor above used for DomainLayer impl...
        const std::vector<std::string> processing_units;
        Simulation_Time simulation_time;
        feature_type& features;
        //TODO is this really required at the top level? or can this be moved to SurfaceLayer?
        const geojson::GeoJSON catchment_data;
        long output_time_index;   

        std::unordered_map<std::string, std::vector<double> >double_buffers;
        std::unordered_map<std::string, std::vector<float> >float_buffers;
        std::unordered_map<std::string, std::vector<int> > int_buffers;
        std::unordered_map<std::string, std::vector<long> > long_buffers;
           

    };
}

#endif