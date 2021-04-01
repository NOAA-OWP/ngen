#include "HY_PointHydroNexusRemoteDownstream.hpp"

#ifdef NGEN_MPI_ACTIVE

HY_PointHydroNexusRemoteDownstream::HY_PointHydroNexusRemoteDownstream(int nexus_id_number, std::string nexus_id, int num_downstream) : HY_PointHydroNexus(nexus_id_number, nexus_id, num_downstream)
{

   int count = 3;
   const int array_of_blocklengths[3] = { 1, 1, 1};
   const MPI_Aint array_of_displacements[3] = { 0, sizeof(long), sizeof(long) + sizeof(long) };
   const MPI_Datatype array_of_types[3] = { MPI_LONG, MPI_LONG, MPI_DOUBLE };

   MPI_Type_create_struct(count, array_of_blocklengths, array_of_displacements, array_of_types, &time_step_and_flow_type);

   MPI_Type_commit(&time_step_and_flow_type);
}

HY_PointHydroNexusRemoteDownstream::~HY_PointHydroNexusRemoteDownstream()
{
    //dtor
}

void HY_PointHydroNexusRemoteDownstream::add_upstream_flow(double val, long catchment_id, time_step_t t)
{
   //TODO:
   //Need to update code for the possibility of multiple downstreams. Need to send this message to all downstreams.
   //Send timestep(key to a dict to extract the info) and flow value as bytes with a custom type that packs these together
   //Custom mpi type that is a pass structure

   //Receive downstream_flow from Upstream Remote Nexus to this Downstream Remote Nexus
   MPI_Recv(
     /* data         = */ &received, 
     /* count        = */ 1, 
     /* datatype     = */ time_step_and_flow_type, 
     /* source       = */ upstream_remote_nexus_rank, 
     /* tag          = */ 0, 
     /* communicator = */ MPI_COMM_WORLD, 
     /* status       = */ MPI_STATUS_IGNORE);

     upstream_catchment_id = received.catchment_id;

     upstream_flow = received.flow;

     time_step = received.time_step;

     HY_PointHydroNexus::add_upstream_flow(upstream_flow, upstream_catchment_id, time_step);

     return;
}

double HY_PointHydroNexusRemoteDownstream::get_upstream_flow_value()
{
   return upstream_flow;
}

long HY_PointHydroNexusRemoteDownstream::get_catchment_id()
{
   return upstream_catchment_id;
}

long HY_PointHydroNexusRemoteDownstream::get_time_step()
{
   return time_step;
}

int HY_PointHydroNexusRemoteDownstream::get_world_rank()
{
   return world_rank;
}

#endif // NGEN_MPI_ACTIVE
