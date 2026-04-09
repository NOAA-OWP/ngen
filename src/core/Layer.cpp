#include <Layer.hpp>
#include <Catchment_Formulation.hpp>
#include <Bmi_Formulation.hpp>

#if NGEN_WITH_MPI
#include "HY_Features_MPI.hpp"
#else
#include "HY_Features.hpp"
#endif

void ngen::Layer::update_models(boost::span<double> catchment_outflows,
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
            // Check mass balance if able
            r_c->check_mass_balance(output_time_index, simulation_time.get_total_output_times(), current_timestamp);
        }
        catch(models::external::State_Exception& e) {
            std::string msg = e.what();
            msg = msg+" at timestep "+std::to_string(output_time_index)
                +" ("+current_timestamp+")"
                +" at feature id "+id;
            throw models::external::State_Exception(msg);
        }
        catch(std::exception& e){
            std::string msg = e.what();
            msg = msg+" at timestep "+std::to_string(output_time_index)
                        +" ("+current_timestamp+")"
                        +" at feature id "+id;
            throw std::runtime_error(msg);
        }
        if (r_c->get_output_header_count() > 0) {
            // only write output if config specifies output values
            std::string output = std::to_string(output_time_index)+","+current_timestamp+","+
                r_c->get_output_line_for_timestep(output_time_index)+"\n";
            r_c->write_output(output);
        }
        //TODO put this somewhere else.  For now, just trying to ensure we get m^3/s into nexus output
        double area_sq_km;
        try {
            area_sq_km = catchment_data->get_feature(id)->get_property("areasqkm").as_real_number();
        }
        catch(std::invalid_argument &e) {
            area_sq_km = catchment_data->get_feature(id)->get_property("area_sqkm").as_real_number();
        }
        double area_sq_m = area_sq_km * 1'000'000;
        //TODO put this somewhere else as well, for now, an implicit assumption is that a module's get_response returns
        //m/timestep
#if NGEN_WITH_ROUTING
        // t-route NHF takes in catchment results in (m^3/s)
        // depth (m) x area (m^2) / dt (seconds)
        int results_index = catchment_indexes[id];
        catchment_outflows[results_index] += 
            // response is meters per timestep (m/t)
            response
            // divide by timestep seconds to get to meters per second: (m/t) * (t/s) = (m/s)
            / simulation_time.get_output_interval_seconds()
            // multiply by square meters: (m/s) * (m^2) = (m^3/s)
            * area_sq_m;
#endif // NGEN_WITH_ROUTING
        // NOTE: the conversion below loos like it's missing a conversion from per timestep to per second
        // Maintaining the current code in case there's a step later that accounts for this
        double response_m_s = response * area_sq_m;
        //since we are operating on a 1 hour (3600s) dt, we need to scale the output appropriately
        //so no response is m^2/hr...m^2/hr * 1hr/3600s = m^3/hr
        double response_m_h = response_m_s / 3600.0;
        //update the nexus with this flow
        for(auto& nexus : features.destination_nexuses(id)) {
            //TODO in a DENDRITIC network, only one destination nexus per catchment
            //If there is more than one, some form of catchment partitioning will be required.
            //for now, only contribute to the first one in the list
            if(nexus == nullptr){
                std::string throw_msg; throw_msg.assign("Invalid (null) nexus instantiation downstream of '"+id+"'");
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

void ngen::Layer::save_state_snapshot(std::shared_ptr<State_Snapshot_Saver> snapshot_saver)
{
    // XXX Handle any of this class's own state as a meta-data unit

    for (auto const& id : processing_units) {
        auto r = features.catchment_at(id);
        auto r_c = std::dynamic_pointer_cast<realization::Bmi_Formulation>(r);
        r_c->save_state(snapshot_saver);
    }
}

void ngen::Layer::load_state_snapshot(std::shared_ptr<State_Snapshot_Loader> snapshot_loader)
{
    // XXX Handle any of this class's own state as a meta-data unit

    for (auto const& id : processing_units) {
        auto r = features.catchment_at(id);
        auto r_c = std::dynamic_pointer_cast<realization::Bmi_Formulation>(r);
        r_c->load_state(snapshot_loader);
    }
}

void ngen::Layer::load_hot_start(std::shared_ptr<State_Snapshot_Loader> snapshot_loader)
{
    // XXX Handle any of this class's own state as a meta-data unit

    for (auto const& id : processing_units) {
        auto r = features.catchment_at(id);
        auto r_c = std::dynamic_pointer_cast<realization::Bmi_Formulation>(r);
        r_c->load_hot_start(snapshot_loader);
    }
}
