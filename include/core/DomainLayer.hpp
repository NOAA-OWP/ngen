#ifndef NGEN_DOMAIN_LAYER
#define NGEN_DOMAIN_LAYER

#include "Catchment_Formulation.hpp"
#include "Layer.hpp"
#include "State_Exception.hpp"
#include "CatchmentOutputsMgr.hpp"

#include <memory>
#include <string>

namespace ngen
{
    class DomainLayer : public Layer
    {
        public:
        DomainLayer() = delete;
        /**
         * @brief Construct a new Domain Layer object.
         * 
         * Unlike HY_Features types, the feature relationship with a DomainLayer is
         * indirect.  The @p features collection defines features associated
         * (e.g. overlapping) with the Domain.  The domain must be further queried
         * in order to provide specific information at a particular feature,
         * e.g. a catchment which this domain overlaps with, or a nexus location
         * the domain may contribute to directly/indirectly via the catchment.
         * 
         * A domain layer associated with a set of catchment features will need to have
         * outputs of the domain resampled/aggregated to the catchment.
         * 
         * Currently unsupported, but a future extension of the DomainLayer is interactions
         * beetween two or more generic DomainLayers, perhaps each with its own internal grid,
         * and resampling between the layers would be required similarly to resampling from
         * a DomainLayer to the HY_Features catchment domain.
         * 
         * @param desc LayerDescription for the domain layer
         * @param s_t Simulation_Time object associated with the domain layer
         * @param features collection of HY_Features associated with the domain
         * @param idx index of the layer
         * @param formulation Formulation associated with the domain
         * @param output_mgr Shared, driver-owned catchment output manager (may be null when output is
         *        disabled). The layer registers as its own feature (formulation id "layer_<id>",
         *        feature id = the layer name) in that manager, so its output honors the configured
         *        format/grouping and lands in its own file alongside the surface catchments.
         */
        DomainLayer(
                const LayerDescription& desc,
                const Simulation_Time& s_t,
                feature_type& features,
                long idx,
                std::shared_ptr<realization::Catchment_Formulation> formulation,
                std::shared_ptr<utils::CatchmentOutputsMgr> output_mgr):
                    Layer(desc, s_t, features, idx, std::move(output_mgr)), formulation(formulation),
                    output_formulation_id("layer_" + std::to_string(desc.id)),
                    output_catchment_id(desc.name)
        {
        }

        /***
         * @brief Run one simulation timestep for this model associated with the domain
         * 
         * A domain layer update simply advances the attached domain formulation(s) in time and records
         * the BMI accessible outputs of the domain formulation. Since this is NOT a HY_Features
         * concept/class, it doesn't directly associate with HY_Features types (e.g. catchments, nexus, ect) 
         * 
         * Any required connection to other components, e.g. providing inputs to a catchment feature,
         * is not yet implemented in this class.
        */
        void update_models(boost::span<double> catchment_outflows, 
                           std::unordered_map<std::string, int> &catchment_indexes,
                           boost::span<double> nexus_downstream_flows,
                           std::unordered_map<std::string, int> &nexus_indexes,
                           int current_step) override {
            std::string current_timestamp = simulation_time.get_timestamp(output_time_index);
            try{
                formulation->get_response(output_time_index, simulation_time.get_output_interval_seconds());
            }
            catch(models::external::State_Exception& e){
                std::string msg = e.what();
                msg = msg+" at timestep "+std::to_string(output_time_index)
                            +" ("+current_timestamp+")"
                            +" at domain layer "+description.name
                            +" (layer id: "+std::to_string(description.id)+")";
                throw models::external::State_Exception(msg);
            } 
            if (catchment_output_mgr) {
                catchment_output_mgr->receive_data_entry(
                    output_formulation_id, output_catchment_id,
                    utils::time_marker(output_time_index, simulation_time.get_current_epoch_time(), current_timestamp),
                    formulation->get_output_values_for_timestep(output_time_index));
            }
            ++output_time_index;
            if ( output_time_index < simulation_time.get_total_output_times() )
            {
               simulation_time.advance_timestep();
            }
    
        }

        private:
        std::shared_ptr<realization::Catchment_Formulation> formulation;
        //! This layer's identity in the shared manager, split into its (formulation, catchment) key.
        std::string output_formulation_id;
        std::string output_catchment_id;
    };
}

#endif // NGEN_DOMAIN_LAYER
