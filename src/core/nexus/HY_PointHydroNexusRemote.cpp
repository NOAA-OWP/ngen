#include "HY_PointHydroNexusRemote.hpp"
#include "Constants.h"


#ifdef NGEN_MPI_ACTIVE

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
        MPI_Abort(MPI_COMM_WORLD,1);
    }
}

HY_PointHydroNexusRemote::HY_PointHydroNexusRemote(std::string nexus_id, Catchments receiving_catchments, catcment_location_map_t loc_map)
    : HY_PointHydroNexus(nexus_id, receiving_catchments),
      catchment_id_to_mpi_rank(loc_map)
{

   int count = 3;
   const int array_of_blocklengths[3] = { 1, 1, 1};
   const MPI_Aint array_of_displacements[3] = { 0, sizeof(long), sizeof(long) + sizeof(long) };
   const MPI_Datatype array_of_types[3] = { MPI_LONG, MPI_LONG, MPI_DOUBLE };

   MPI_Type_create_struct(count, array_of_blocklengths, array_of_displacements, array_of_types, &time_step_and_flow_type);

   MPI_Type_commit(&time_step_and_flow_type);

   MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
}

HY_PointHydroNexusRemote::~HY_PointHydroNexusRemote()
{
    long wait_time = 0;

    // This destructore might be called after MPI_Finalize so do not attempt communication if
    // this has occured
    int mpi_finalized;
    MPI_Finalized(&mpi_finalized);

    while ( (stored_recieves.size() > 0 || stored_sends.size() > 0) && !mpi_finalized )
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

double HY_PointHydroNexusRemote::get_downstream_flow(std::string catchment_id, time_step_t t, double percent_flow)
{
    while ( stored_recieves.size() > 0)
    {
        //std::cerr << "Waiting on " << stored_recieves.size() << " recieves\n";

        process_communications();

        if (stored_recieves.size()) std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    auto iter = catchment_id_to_mpi_rank.find(catchment_id);
    if ( iter != catchment_id_to_mpi_rank.end() )
    {
        // allocate the message buffer
        stored_sends.resize(stored_sends.size() + 1);
        stored_sends.back().buffer = std::make_shared<time_step_and_flow_t>();

        // fill the message buffer
        stored_sends.back().buffer->time_step = t;
        stored_sends.back().buffer->catchment_id = std::stoi( catchment_id.substr(4) );

        // get the correct amount of flow using the inherted function this means are local bookkeeping is accurate
        stored_sends.back().buffer->flow = HY_PointHydroNexus::get_downstream_flow(catchment_id, t, percent_flow);;

        int tag = extract(id);

        //Send downstream_flow from this Upstream Remote Nexus to the Downstream Remote Nexus
        MPI_Isend(
            /* data         = */ stored_sends.back().buffer.get(),
            /* count        = */ 1,
            /* datatype     = */ time_step_and_flow_type,
            /* destination  = */ iter->second,
            /* tag          = */ tag,
            /* communicator = */ MPI_COMM_WORLD,
            /* request      = */ &stored_sends.back().mpi_request);

        //std::cerr << "Remote send from rank " << world_rank << " to rank " << iter->second << " on tag " << tag << std::endl;
        return (double)SENTINEL_IS_REMOTELY_HANDLED;
    }
    else
    {
        return HY_PointHydroNexus::get_downstream_flow(catchment_id, t, percent_flow);
    }

}

void HY_PointHydroNexusRemote::add_upstream_flow(double val, std::string catchment_id, time_step_t t)
{

   auto iter = catchment_id_to_mpi_rank.find(catchment_id);
   if ( iter != catchment_id_to_mpi_rank.end() )
   {
       //TODO:
       //Need to update code for the possibility of multiple downstreams. Need to send this message to all downstreams.
       //Send timestep(key to a dict to extract the info) and flow value as bytes with a custom type that packs these together
       //Custom mpi type that is a pass structure

       MPI_Request request;
       int status;

       stored_recieves.resize(stored_recieves.size() + 1);
       stored_recieves.back().buffer = std::make_shared<time_step_and_flow_t>();

       int tag = extract(catchment_id);

       //Receive downstream_flow from Upstream Remote Nexus to this Downstream Remote Nexus
       status = MPI_Irecv(
         /* data         = */ stored_recieves.back().buffer.get(),
         /* count        = */ 1,
         /* datatype     = */ time_step_and_flow_type,
         /* source       = */ iter->second,
         /* tag          = */ tag,
         /* communicator = */ MPI_COMM_WORLD,
         /* request       = */ &stored_recieves.back().mpi_request);

       MPI_Handle_Error(status);

       //std::cerr << "Remote recieve on rank " << world_rank << " from rank " << iter->second << " on tag " << tag << std::endl;

     }
     else // if this catchment is not remote call base function
     {
        HY_PointHydroNexus::add_upstream_flow(val, catchment_id, t);
     }

     return;
}

void HY_PointHydroNexusRemote::process_communications()
{
    int flag;                                      // boolean value for if a request has completed
    MPI_Status status;                              // status of the completed request

    for ( auto i = stored_recieves.begin(); i != stored_recieves.end(); )
    {
        MPI_Handle_Error( MPI_Test(&i->mpi_request, &flag, &status) );

        if (flag)
        {
            // get the data from the communication buffer
            long time_step = i->buffer->time_step;
            std::string catchment_id = std::string(nexus_prefix + std::to_string(i->buffer->catchment_id));
            double flow = i->buffer->flow;

            // remove this object from the vector
            i = stored_recieves.erase(i);

            // add the recieved flow
            HY_PointHydroNexus::add_upstream_flow(flow, catchment_id, time_step);
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

#endif // NGEN_MPI_ACTIVE
