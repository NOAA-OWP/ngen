#include "HY_PointHydroNexusRemoteUpstream.hpp"

//Can allow multiple upstreams to be on different ranks / multiple processors.
//Downstream is always local
//Everything that extracts water from a nexus needs to be colocal / on the same processor.


HY_PointHydroNexusRemoteUpstream::HY_PointHydroNexusRemoteUpstream(int nexus_id_number, std::string nexus_id, int num_downstream) : HY_PointHydroNexus(nexus_id_number, nexus_id, num_downstream)
{
   MPI_Init(NULL, NULL);

   MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

}

HY_PointHydroNexusRemoteUpstream::~HY_PointHydroNexusRemoteUpstream()
{
    //dtor
}

double HY_PointHydroNexusRemoteUpstream::get_downstream_flow(long catchment_id, time_step_t t, double percent_flow)
{
   double downstream_flow = HY_PointHydroNexus::get_downstream_flow(catchment_id, t, percent_flow);

   //Determine this with a broadcast of the ranks at init?
   int downstream_remote_nexus_rank = 1;

   //Send downstream_flow from this Upstream Remote Nexus to the Downstream Remote Nexus
   MPI_Send(
     /* data         = */ &downstream_flow, 
     /* count        = */ 1, 
     /* datatype     = */ MPI_DOUBLE, 
     /* destination  = */ downstream_remote_nexus_rank, 
     /* tag          = */ 0, 
     /* communicator = */ MPI_COMM_WORLD);

   //Return downstream_flow or 0.0 since it was passed to the downstream remote nexus?
   return downstream_flow;
}

void HY_PointHydroNexusRemoteUpstream::add_upstream_flow(double val, long catchment_id, time_step_t t)
{
   HY_PointHydroNexus::add_upstream_flow(val, catchment_id, t);

   return;

}

int HY_PointHydroNexusRemoteUpstream::get_world_rank()
{
   return world_rank;
}
