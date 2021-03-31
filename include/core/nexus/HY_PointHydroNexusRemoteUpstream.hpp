#ifndef HY_POINTHYDRONEXUSREMOTEUPSTREAM_H
#define HY_POINTHYDRONEXUSREMOTEUPSTREAM_H

#ifdef NGEN_MPI_ACTIVE

#include <HY_PointHydroNexus.hpp>
#include <mpi.h>


class HY_PointHydroNexusRemoteUpstream : public HY_PointHydroNexus
{
    public:
        HY_PointHydroNexusRemoteUpstream(int nexus_id_number, std::string nexus_id, int num_downstream);
        virtual ~HY_PointHydroNexusRemoteUpstream();

        /** add flow to this nexus for timestep t. */
        void add_upstream_flow(double val, long catchment_id, time_step_t t);

        int get_world_rank();

    private:
        int world_rank;

        //Determine this with a broadcast of the ranks at init?
        int downstream_remote_nexus_rank = 1;

        // Create the MPI datatype
        MPI_Datatype time_step_and_flow_type;

        struct time_step_and_flow_t
        {
           long time_step;
           double flow;
        };

        struct time_step_and_flow_t buffer;


};

#endif // NGEN_MPI_ACTIVE
#endif // HY_POINTHYDRONEXUSREMOTEUPSTREAM_H
