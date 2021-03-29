#ifndef HY_POINTHYDRONEXUSREMOTEUPSTREAM_H
#define HY_POINTHYDRONEXUSREMOTEUPSTREAM_H

#include <HY_PointHydroNexus.hpp>
#include <mpi.h>


class HY_PointHydroNexusRemoteUpstream : public HY_PointHydroNexus
{
    public:
        HY_PointHydroNexusRemoteUpstream(int nexus_id_number, std::string nexus_id, int num_downstream);
        virtual ~HY_PointHydroNexusRemoteUpstream();

        /** get the request percentage of downstream flow through this nexus at timestep t. */
        double get_downstream_flow(long catchment_id, time_step_t t, double percent_flow);

        /** add flow to this nexus for timestep t. */
        void add_upstream_flow(double val, long catchment_id, time_step_t t);

        int get_world_rank();

    private:
        int world_rank;


};

#endif // HY_POINTHYDRONEXUSREMOTEUPSTREAM_H
