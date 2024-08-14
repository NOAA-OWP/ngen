#include "Layer.hpp"

namespace ngen
{
    /***
     * @brief Run one simulation timestep for each model in this layer
    */

    void Layer::update_models(std::shared_ptr<data_output::OutputWriter> writer)
    {
        auto idx = simulation_time.next_timestep_index();
        auto step = simulation_time.get_output_interval_seconds();
        
        //std::cout<<"Output Time Index: "<<output_time_index<<std::endl;
        if(output_time_index%100 == 0) std::cout<<"Running timestep " << output_time_index <<std::endl;
        std::string current_timestamp = simulation_time.get_timestamp(output_time_index);

        std::size_t buffer_size = processing_units.size();
    
        for(const auto& id : processing_units) 
        {
            int sub_time = output_time_index;
            //std::cout<<"Running cat "<<id<<std::endl;
            auto r = features.catchment_at(id);
            //TODO redesign to avoid this cast
            auto r_c = std::dynamic_pointer_cast<realization::Catchment_Formulation>(r);
            double response(0.0);
            try{
                response = r_c->get_response(output_time_index, simulation_time.get_output_interval_seconds());
            }
            catch(models::external::State_Exception& e){
                std::string msg = e.what();
                msg = msg+" at timestep "+std::to_string(output_time_index)
                            +" ("+current_timestamp+")"
                            +" at feature id "+id;
                throw models::external::State_Exception(msg);
            }
            std::string output = std::to_string(output_time_index)+","+current_timestamp+","+
                                r_c->get_output_line_for_timestep(output_time_index)+"\n";
            r_c->write_output(output);
            //TODO put this somewhere else.  For now, just trying to ensure we get m^3/s into nexus output
            double area;
            try{
                area = catchment_data->get_feature(id)->get_property("areasqkm").as_real_number();
            }
            catch(std::invalid_argument &e)
            {
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
                    throw std::runtime_error("Invalid (null) nexus instantiation downstream of "+id+". "+SOURCE_LOC);
                }
                nexus->add_upstream_flow(response_m_h, id, output_time_index);
                /*std::cerr << "Add water to nexus ID = " << nexus->get_id() << " from catchment ID = " << id << " value = "
                            << response << ", ID = " << id << ", time-index = " << output_time_index << std::endl; */
                break;
            }

            // TODO extract variables by BMI

            // retrieve a pointer to realization 
            auto fm = layer->get_realization(id);

            // cast the pointer to a bmi type
            auto bmi_f = std::dynamic_pointer_cast<realization::Bmi_Formulation>(fm);

            try
            {
            // get the variable names associated with this realization
            auto names = bmi_f->get_output_variable_names();

            for( auto name : names )
            {
                
            }

            }
            catch (std::exception e)
            {
                throw(e);
            }
            
        } //done catchments   

        ++output_time_index;
        if ( output_time_index < simulation_time.get_total_output_times() )
        {
            simulation_time.advance_timestep();
        }       
    }    
}