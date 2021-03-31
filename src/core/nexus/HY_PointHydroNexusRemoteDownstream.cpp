#include "HY_PointHydroNexusRemoteDownstream.hpp"

#ifdef NGEN_MPI_ACTIVE

HY_PointHydroNexusRemoteDownstream::HY_PointHydroNexusRemoteDownstream(int nexus_id_number, std::string nexus_id, int num_downstream) : HY_PointHydroNexus(nexus_id_number, nexus_id, num_downstream)
{
   MPI_Init(NULL, NULL);

   MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

}

HY_PointHydroNexusRemoteDownstream::~HY_PointHydroNexusRemoteDownstream()
{
    //dtor
}

double HY_PointHydroNexusRemoteDownstream::get_downstream_flow(long catchment_id, time_step_t t, double percent_flow)
{
   //double downstream_flow = HY_PointHydroNexus::get_downstream_flow(catchment_id, t, percent_flow);

   double downstream_flow;

   int upstream_remote_nexus_rank = 0;

   //Need to update code for the possibility of multiple downstreams. Need to send this message to all downstreams.

   //Send timestep(key to a dict to extract the info) and flow value as bytes with a custom type that packs these together
   //Custom mpi type that is a pass structure

   //Receive downstream_flow from Upstream Remote Nexus to this Downstream Remote Nexus
   MPI_Recv(
     /* data         = */ &downstream_flow, 
     /* count        = */ 1, 
     /* datatype     = */ MPI_DOUBLE, 
     /* source       = */ upstream_remote_nexus_rank, 
     /* tag          = */ 0, 
     /* communicator = */ MPI_COMM_WORLD, 
     /* status       = */ MPI_STATUS_IGNORE);

   std::cout << "NexusRemoteDownstream downstream_flow: " << downstream_flow << std::endl; 

   return downstream_flow;
}

void HY_PointHydroNexusRemoteDownstream::add_upstream_flow(double val, long catchment_id, time_step_t t)
{

   double upstream_flow;

   int upstream_remote_nexus_rank = 0;

   struct time_step_and_flow_t
   {
       long time_step;
       double flow;
   };

   struct time_step_and_flow_t received;

   // Create the datatype
   MPI_Datatype time_step_and_flow_type;


   int count = 2;
   const int array_of_blocklengths[2] = { 1, 1 };
   const MPI_Aint array_of_displacements[2] = { 0, sizeof(long) };
   const MPI_Datatype array_of_types[2] = { MPI_LONG, MPI_DOUBLE };

   MPI_Type_create_struct(count, array_of_blocklengths, array_of_displacements, array_of_types, &time_step_and_flow_type);

   MPI_Type_commit(&time_step_and_flow_type);




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

     long time_step = received.time_step;

     upstream_flow = received.flow;


   std::cout << "NexusRemoteDownstream upstream_flow: " << upstream_flow << std::endl; 



  HY_PointHydroNexus::add_upstream_flow(upstream_flow, catchment_id, time_step);

  return;

}

int HY_PointHydroNexusRemoteDownstream::get_world_rank()
{
   return world_rank;
}

#endif // NGEN_MPI_ACTIVE
