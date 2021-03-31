#include "HY_PointHydroNexusRemoteUpstream.hpp"

#ifdef NGEN_MPI_ACTIVE

//Can allow multiple upstreams to be on different ranks / multiple processors.
//Downstream is always local
//Everything that extracts water from a nexus needs to be colocal / on the same processor.


HY_PointHydroNexusRemoteUpstream::HY_PointHydroNexusRemoteUpstream(int nexus_id_number, std::string nexus_id, int num_downstream) : HY_PointHydroNexus(nexus_id_number, nexus_id, num_downstream)
{
   MPI_Init(NULL, NULL);

   MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

   int count = 2;
   const int array_of_blocklengths[2] = { 1, 1 };
   const MPI_Aint array_of_displacements[2] = { 0, sizeof(long) };
   const MPI_Datatype array_of_types[2] = { MPI_LONG, MPI_DOUBLE };

   MPI_Type_create_struct(count, array_of_blocklengths, array_of_displacements, array_of_types, &time_step_and_flow_type);

   MPI_Type_commit(&time_step_and_flow_type);


}

HY_PointHydroNexusRemoteUpstream::~HY_PointHydroNexusRemoteUpstream()
{
    //dtor
}


void HY_PointHydroNexusRemoteUpstream::add_upstream_flow(double val, long catchment_id, time_step_t t)
{
   //Don't call below on the updstream nexus because don't want to add to the total here.
   //This upstream nexus is only to pass the flow to the downstream nexus, which will
   //do the accounting.
   //HY_PointHydroNexus::add_upstream_flow(val, catchment_id, t);



   // Create the datatype
   //MPI_Datatype time_step_and_flow_type;

   /*   
   int count = 2;
   const int array_of_blocklengths[2] = { 1, 1 };
   const MPI_Aint array_of_displacements[2] = { 0, sizeof(long) };
   const MPI_Datatype array_of_types[2] = { MPI_LONG, MPI_DOUBLE }; 

   MPI_Type_create_struct(count, array_of_blocklengths, array_of_displacements, array_of_types, &time_step_and_flow_type);

   MPI_Type_commit(&time_step_and_flow_type);
   */


   //struct time_step_and_flow_t
   //{
   //    long time_step;
   //   double flow;
   //}; 


   //struct time_step_and_flow_t buffer;

   buffer.time_step = t;
   buffer.flow = val;

   //Send downstream_flow from this Upstream Remote Nexus to the Downstream Remote Nexus
   MPI_Send(
     /* data         = */ &buffer, 
     /* count        = */ 1, 
     /* datatype     = */ time_step_and_flow_type, 
     /* destination  = */ downstream_remote_nexus_rank, 
     /* tag          = */ 0, 
     /* communicator = */ MPI_COMM_WORLD);


   return;

}

int HY_PointHydroNexusRemoteUpstream::get_world_rank()
{
   return world_rank;
}

#endif // NGEN_MPI_ACTIVE
