#ifndef __NGEN_LAYER__
#define __NGEN_LAYER__

#include <NGenConfig.h>
#include "Logger.hpp"

#include "LayerData.hpp"
#include "Simulation_Time.hpp"
#include "State_Exception.hpp"
#include "geojson/FeatureBuilder.hpp"
#include "state_save_restore/State_Save_Restore.hpp"
#include <boost/core/span.hpp>
#include <boost/serialization/serialization.hpp>
#include <map>

namespace hy_features
{
    class HY_Features;
    class HY_Features_MPI;
}

class State_Snapshot_Saver;
class State_Snapshot_Loader;

namespace ngen
{

    class Layer
    {    
        public:

        #if NGEN_WITH_MPI && NGEN_WITH_NEXUSES
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

        /***
         * @brief Run one simulation timestep for each model in this layer
        */
        virtual void update_models(boost::span<double> catchment_outflows, 
                                   std::unordered_map<std::string, int> &catchment_indexes,
#if NGEN_WITH_NEXUSES
                                   boost::span<double> nexus_downstream_flows,
                                   std::unordered_map<std::string, int> &nexus_indexes,
#endif // NGEN_WITH_NEXUSES
                                   int current_step);

        /**
         * Save the current state including metatdata related to current layer times
         */
        virtual void save_checkpoint(std::shared_ptr<State_Snapshot_Saver> snapshot_saver);
        /**
         * Save the current state excluding metatdata related to current layer times
         */
        virtual void save_end_of_run(std::shared_ptr<State_Snapshot_Saver> snapshot_saver);
        virtual void load_checkpoint(std::shared_ptr<State_Snapshot_Loader> snapshot_loader);
        virtual void load_hot_start(std::shared_ptr<State_Snapshot_Loader> snapshot_loader);

        std::string unit_name() const;
        virtual std::vector<std::string> required_checkpoint_units() const;

        virtual const std::map<std::string, std::string>& get_catchment_output_data_for_timestep();
        virtual void set_simulations_output_format(std::vector<std::string> out_formats);
        virtual std::vector<std::string> get_simulations_output_format();
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
        std::map<std::string, std::string> catchment_output_values;
        std::vector<std::string> output_formats;

        // Serialization template will be defined and instantiated in the .cpp file
        friend class boost::serialization::access;
        template <class Archive>
        void serialize(Archive& ar, const unsigned int version) {
            ar & this->output_time_index;
            ar & this->simulation_time;
        }
    };
}
#endif
