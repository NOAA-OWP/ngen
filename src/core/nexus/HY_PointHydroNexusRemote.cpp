#include "HY_PointHydroNexusRemote.hpp"
#include "Constants.h"


#if NGEN_WITH_MPI

#include <HY_Features_Ids.hpp>
#include <chrono>
#include <thread>

// TODO add loggin to this function

void MPI_Handle_Error(int status)
{
    if ( status == MPI_SUCCESS )
    {
        return;
    }
    else
    {
        MPI_Abort(MPI_COMM_WORLD, status);
    }
}

HY_PointHydroNexusRemote::HY_PointHydroNexusRemote(std::string nexus_id, Catchments receiving_catchments, Catchments contributing_catchments, catcment_location_map_t loc_map)
    : HY_PointHydroNexus(nexus_id, receiving_catchments, contributing_catchments),
        catchment_id_to_mpi_rank(loc_map)
{
   int count = 3;
   const int array_of_blocklengths[3] = { 1, 1, 1};
   const MPI_Aint array_of_displacements[3] = { 0, sizeof(long), sizeof(long) + sizeof(long) };
   const MPI_Datatype array_of_types[3] = { MPI_LONG, MPI_LONG, MPI_DOUBLE };

   MPI_Type_create_struct(count, array_of_blocklengths, array_of_displacements, array_of_types, &time_step_and_flow_type);

   MPI_Type_commit(&time_step_and_flow_type);

   MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

   bool is_sender = false;
   bool is_receiver = false;
   //Establish the communication pattern required for this nexus
   //Sender
   for(auto receiver : get_receiving_catchments()){
       //Loop through all downstream catchments, see if they are in the remote map
       try{
            auto& remote_rank = catchment_id_to_mpi_rank.at(receiver);
            downstream_ranks.insert(remote_rank);
            remote_receivers.push_back(receiver);
            is_sender = true;
       }
       catch( std::out_of_range &e ){
           local_receivers.push_back(receiver);
           continue; //receiver not found, go to next
       }
    }

    for(auto contributer : get_contributing_catchments()){
        //Loop through all upstream catchments, see if they are in the remote map
        try{
            auto& remote_rank = catchment_id_to_mpi_rank.at(contributer);
            upstream_ranks.insert(remote_rank);
            remote_contributers.push_back(contributer);
            is_receiver = true;
        }
        catch( std::out_of_range &e ){
            local_contributers.push_back(contributer);
            continue; //contributer not found, go to next
        }
    }

    if( is_sender && !is_receiver ){
        type = sender;
    }
    else if( is_receiver && !is_sender ){
        type = receiver;
    }
    else if( is_receiver && is_sender ){
        type = sender_receiver;
    }
    else{ 
        type = local;
    }

}

HY_PointHydroNexusRemote::HY_PointHydroNexusRemote(std::string nexus_id, Catchments receiving_catchments, catcment_location_map_t loc_map)
    : HY_PointHydroNexusRemote(nexus_id, receiving_catchments, Catchments(), loc_map)
{
   
}

HY_PointHydroNexusRemote::~HY_PointHydroNexusRemote()
{
    long wait_time = 0;

    // This destructore might be called after MPI_Finalize so do not attempt communication if
    // this has occured
    int mpi_finalized;
    MPI_Finalized(&mpi_finalized);

    while ( (stored_receives.size() > 0 || stored_sends.size() > 0) && !mpi_finalized )
    {
        //std::cerr << "Neuxs with rank " << id << " has pending communications\n";

        process_communications();

        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        wait_time += 1;

        if ( wait_time > 120000 )
        {
            // TODO log warning message that some comunications could not complete

        }
    }
}

void HY_PointHydroNexusRemote::post_receives()
{
    // Post receives if not already posted (for pure receiver nexuses)
    if (stored_receives.empty())
    {
        for (int rank : upstream_ranks)
        {
            stored_receives.push_back({});
            stored_receives.back().buffer = std::make_shared<time_step_and_flow_t>();
            int tag = extract(id);
            
            MPI_Handle_Error(MPI_Irecv(
                stored_receives.back().buffer.get(),
                1,
                time_step_and_flow_type,
                rank,
                tag,
                MPI_COMM_WORLD,
                &stored_receives.back().mpi_request));
        }
    }
}

double HY_PointHydroNexusRemote::get_downstream_flow(std::string catchment_id, time_step_t t, double percent_flow)
{
    double remote_flow = 0.0;
    if ( type == sender )
    {
        //At this point, calling `get_downstream_flow` on a remote sender is undefined behaviour
        //because the `add_upstream_flow` call triggers a `send` which removes from the local accounting
        //all available water and sends it to the remote counterpart for this nexus.
        std::string msg = "Nexus "+id+" attempted to get_downstream_flow, but its communicator type is sender only.";
        throw std::runtime_error(msg);
    }
    else if ( type == receiver || type == sender_receiver )
    {
        post_receives();
        // Wait for receives to complete
        // This ensures all upstream flows are received before returning
        // and that we have matched all sends with receives for a given time step.
        // As long as the functions are called appropriately, e.g. one call to
        // `add_upstream_flow` per upstream catchment per time step, followed
        // by a call to `get_downstream_flow` for each downstream catchment per time step,
        // this loop will terminate and ensures the synchronization of flows between
        // ranks.
        while ( stored_receives.size() > 0 )
    	{
    		process_communications();
    		std::this_thread::sleep_for(std::chrono::milliseconds(1));
    	}
    }
    
    return HY_PointHydroNexus::get_downstream_flow(catchment_id, t, percent_flow);
}

void HY_PointHydroNexusRemote::add_upstream_flow(double val, std::string catchment_id, time_step_t t)
{
	// Process any completed communications to free resources
    // If no communications are pending, this call will do nothing.
	process_communications();
    // NOTE: It is possible for a partition to get "too far" ahead since the sends are now
    // truely asynchronous.  For pure receivers and sender_receivers, this isn't a problem
    // because the get_downstream_flow function will block until all receives are processed.
    // However, for pure senders, this could be a problem.
    // We can use this spinlock here to limit how far ahead a partition can get.
    // in this case, approximately 100 time steps per downstream catchment...
    while( stored_sends.size() > downstream_ranks.size()*100 )
    {
        process_communications();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
	
	// Post receives before sending to prevent deadlock
	// When stored_receives is empty, we need to post for incoming messages
	if ((type == receiver || type == sender_receiver) && stored_receives.empty())
	{
		post_receives();
	}
	
	// first add flow to local copy
	HY_PointHydroNexus::add_upstream_flow(val, catchment_id, t);
	
	// if we are a sender check to see if all of our upstreams have been added for the indicated time step
	if ( type == sender || type  == sender_receiver )
	{
		auto& flows_for_timestep = upstream_flows[t];
		bool all_found = true;
		
		// check for stored data for each contributer
		for ( auto& id : get_local_contributing_catchments() )
		{
			auto pos = std::find_if(flows_for_timestep.begin(), flows_for_timestep.end(), [&id] (flows& v) { return v.first == id; } );
			
			if ( pos == flows_for_timestep.end() )
			{
				all_found = false;
				break;
			}
		}
		
		// if we have all of our upstreams for this time step send the data
		if ( all_found )
		{
		    // allocate the message buffer
		    stored_sends.resize(stored_sends.size() + 1);
		    stored_sends.back().buffer = std::make_shared<time_step_and_flow_t>();

		    // fill the message buffer
		    stored_sends.back().buffer->time_step = t;
		    stored_sends.back().buffer->catchment_id = std::stoi( id.substr( id.find(hy_features::identifiers::seperator)+1 ) );

		    // get the correct amount of flow using the inherted function this means are local bookkeeping is accurate
		    stored_sends.back().buffer->flow = HY_PointHydroNexus::get_downstream_flow(id, t, 100.0);;

		    int tag = extract(id);

		    //Send downstream_flow from this Upstream Remote Nexus to the Downstream Remote Nexus
		    MPI_Handle_Error(
                MPI_Isend(
		        stored_sends.back().buffer.get(),
		        1,
		        time_step_and_flow_type,
		        *downstream_ranks.begin(), //TODO currently only support a SINGLE downstream message pairing
		        tag,
		        MPI_COMM_WORLD,
		        &stored_sends.back().mpi_request)
            );
		        
		    //std::cerr << "Creating send with target_rank=" << *downstream_ranks.begin() << " on tag=" << tag << "\n";
		    
		    // Send is async, the next call to add_upstream_flow will test and ensure the send has completed
            // and free the memory associated with the send.
            // This prevents a potential deadlock situation where a send isn't able to complete
            // because the remote receiver is also trying to send and the underlying mpi buffers/protocol
            // are forced into a rendevous protocol.  So we ensure that we always post receives before sends.
            // and that we always test for completed sends before freeing the memory associated with the send.
		}
	}
}

void HY_PointHydroNexusRemote::process_communications()
{
    int flag;                                      // boolean value for if a request has completed
    MPI_Status status;                              // status of the completed request

    for ( auto i = stored_receives.begin(); i != stored_receives.end(); )
    {
        MPI_Handle_Error( MPI_Test(&i->mpi_request, &flag, &status) );

        if (flag)
        {
            // get the data from the communication buffer
            long time_step = i->buffer->time_step;
            std::string contributing_id = id;
            double flow = i->buffer->flow;

            // remove this object from the vector
            i = stored_receives.erase(i);

            // add the received flow
            HY_PointHydroNexus::add_upstream_flow(flow, contributing_id, time_step);
        }
        else
        {
            ++i;
        }
    }

    // Check for and remove any stored sends requests that have completed
    for ( auto i = stored_sends.begin(); i != stored_sends.end(); )
    {
        MPI_Handle_Error( MPI_Test(&i->mpi_request, &flag, &status) );

        if (flag)
        {
            // remove this object from the vector
            i = stored_sends.erase(i);
        }
        else
        {
            ++i;
        }
    }
}

long HY_PointHydroNexusRemote::get_time_step()
{
   return time_step;
}

int HY_PointHydroNexusRemote::get_world_rank()
{
   return world_rank;
}

#endif // NGEN_WITH_MPI
