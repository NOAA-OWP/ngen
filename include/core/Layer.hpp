#ifndef __NGEN_LAYER__
#define __NGEN_LAYER__

#include "LayerData.hpp"
#include "Simulation_Time.hpp"

#ifdef NGEN_MPI_ACTIVE
#include "HY_Features_MPI.hpp"
#else
#include "HY_Features.hpp"
#endif

#include "Pdm03.h"

namespace ngen
{

    class Layer
    {    
        public:

        #ifdef NGEN_MPI_ACTIVE
            typedef hy_features::HY_Features_MPI feature_type;
        #else
            typedef hy_features::HY_Features feature_type;
        #endif

        Layer(
                const LayerDescription& desc, 
                const std::vector<std::string>& p_u, 
                const Simulation_Time& s_t, 
                feature_type& f, 
                geojson::GeoJSON cd, 
                std::shared_ptr<pdm03_struct> pdmd,
                long idx) :
            description(desc),
            processing_units(p_u),
            simulation_time(s_t),
            features(f),
            catchment_data(cd),
            pdm_et_data(pdmd),
            output_time_index(idx)
        {

        }

        virtual ~Layer() {}

        /***
         * @brief Return the next timestep that will be processed by this layer in epoc time units
        */
        time_t next_timestep_epoch_time() { return simulation_time.next_timestep_epoch_time(); }


        /***
         * @brief Return the last timesteo that was processed by this layer in epoc time units
        */
        time_t current_timestep_epoch_time() { return simulation_time.get_current_epoc_time(); }


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
         * @breif Run one simulation timestep for each model in this layer
        */
        virtual void update_models()
        {
            auto idx = simulation_time.next_timestep_index();
            auto step = simulation_time.get_output_interval_seconds();
            
            //std::cout<<"Output Time Index: "<<output_time_index<<std::endl;
            if(output_time_index%100 == 0) std::cout<<"Running timestep " << output_time_index <<std::endl;
            std::string current_timestamp = simulation_time.get_timestamp(output_time_index);
            for(const auto& id : processing_units) 
            {
                int sub_time = output_time_index;
                //std::cout<<"Running cat "<<id<<std::endl;
                auto r = features.catchment_at(id);
                //TODO redesign to avoid this cast
                auto r_c = std::dynamic_pointer_cast<realization::Catchment_Formulation>(r);
                r_c->set_et_params(pdm_et_data);
                double response = r_c->get_response(output_time_index, description.time_step);
                std::string output = std::to_string(output_time_index)+","+current_timestamp+","+
                                    r_c->get_output_line_for_timestep(output_time_index)+"\n";
                r_c->write_output(output);
                //TODO put this somewhere else.  For now, just trying to ensure we get m^3/s into nexus output
                try{
                    response *= (catchment_data->get_feature(id)->get_property("areasqkm").as_real_number() * 1000000);
                }catch(std::invalid_argument &e)
                {
                    response *= (catchment_data->get_feature(id)->get_property("area_sqkm").as_real_number() * 1000000);
                }
                //TODO put this somewhere else as well, for now, an implicit assumption is that a modules get_response returns
                //m/timestep
                //since we are operating on a 1 hour (3600s) dt, we need to scale the output appropriately
                //so no response is m^2/hr...m^2/hr * 1hr/3600s = m^3/hr
                response /= 3600.0;
                //update the nexus with this flow
                for(auto& nexus : features.destination_nexuses(id)) {
                    //TODO in a DENDRIDIC network, only one destination nexus per catchment
                    //If there is more than one, some form of catchment partitioning will be required.
                    //for now, only contribute to the first one in the list
                    nexus->add_upstream_flow(response, id, output_time_index);
                    std::cerr << "Add water to nexus ID = " << nexus->get_id() << " from catchment ID = " << id << " value = "
                              << response << ", ID = " << id << ", time-index = " << output_time_index << std::endl; 
                    break;
                }
                
            } //done catchments   

            ++output_time_index;
            simulation_time.advance_timestep();       
        }
        

        protected:

        LayerDescription description;
        std::vector<std::string> processing_units;
        Simulation_Time simulation_time;
        feature_type& features;
        geojson::GeoJSON catchment_data;
        std::shared_ptr<pdm03_struct> pdm_et_data;
        long output_time_index;       

    };
}

#endif