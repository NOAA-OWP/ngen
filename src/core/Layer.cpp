#include <Layer.hpp>

namespace ngen
{
    void Layer::update_models(boost::span<double> catchment_outflows, 
                              std::unordered_map<std::string, int> &catchment_indexes,
                              boost::span<double> nexus_downstream_flows,
                              std::unordered_map<std::string, int> &nexus_indexes,
                              int current_step)
    {
        auto idx = simulation_time.next_timestep_index();
        auto step = simulation_time.get_output_interval_seconds();
            
        //std::cout<<"Output Time Index: "<<output_time_index<<std::endl;
        if(output_time_index%1000 == 0) std::cout<<"Running timestep " << output_time_index <<std::endl;
        std::string current_timestamp = simulation_time.get_timestamp(output_time_index);
        for(const auto& id : processing_units) {
            int sub_time = output_time_index;
            //std::cout<<"Running cat "<<id<<std::endl;
            auto r = features.catchment_at(id);
            //TODO redesign to avoid this cast
            auto r_c = std::dynamic_pointer_cast<realization::Catchment_Formulation>(r);
            double response(0.0);
            try {
                response = r_c->get_response(output_time_index, simulation_time.get_output_interval_seconds());
            }
            catch(models::external::State_Exception& e) {
                std::string msg = e.what();
                msg = msg+" at timestep "+std::to_string(output_time_index)
                    +" ("+current_timestamp+")"
                    +" at feature id "+id;
                throw models::external::State_Exception(msg);
            }
#if NGEN_WITH_ROUTING
            int results_index = catchment_indexes[id];
            catchment_outflows[results_index] = response;
#endif // NGEN_WITH_ROUTING
            std::string output = std::to_string(output_time_index)+","+current_timestamp+","+
                r_c->get_output_line_for_timestep(output_time_index)+"\n";
            r_c->write_output(output);
            //TODO put this somewhere else.  For now, just trying to ensure we get m^3/s into nexus output
            double area;
            try {
                area = catchment_data->get_feature(id)->get_property("areasqkm").as_real_number();
            }
            catch(std::invalid_argument &e) {
                area = catchment_data->get_feature(id)->get_property("area_sqkm").as_real_number();
            }
            double response_m_s = response * (area * 1000000);
            //TODO put this somewhere else as well, for now, an implicit assumption is that a module's get_response returns
            //m/timestep
            //since we are operating on a 1 hour (3600s) dt, we need to scale the output appropriately
            //so no response is m^2/hr...m^2/hr * 1hr/3600s = m^3/hr
            double response_m_h = response_m_s / 3600.0;
            //update the nexus with this flow
            for(auto& nexus : features.destination_nexuses(id)) {
                //TODO in a DENDRITIC network, only one destination nexus per catchment
                //If there is more than one, some form of catchment partitioning will be required.
                //for now, only contribute to the first one in the list
                if(nexus == nullptr){
                    std::string throw_msg; throw_msg.assign("Invalid (null) nexus instantiation downstream of "+id+". "+SOURCE_LOC);
                    LOG(throw_msg, LogLevel::WARNING);
                    throw std::runtime_error(throw_msg);
                }
                nexus->add_upstream_flow(response_m_h, id, output_time_index);
                /*std::cerr << "Add water to nexus ID = " << nexus->get_id() << " from catchment ID = " << id << " value = "
                  << response << ", ID = " << id << ", time-index = " << output_time_index << std::endl; */
                break;
            }
                
        } //done catchments   

        ++output_time_index;
        if ( output_time_index < simulation_time.get_total_output_times() ) {
            simulation_time.advance_timestep();
        }       
    }
}
