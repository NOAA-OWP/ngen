#ifndef HY_POINTHYDRONEXUSREMOTEDOWNSTREAM_H
#define HY_POINTHYDRONEXUSREMOTEDOWNSTREAM_H

#ifdef NGEN_MPI_ACTIVE

#include <HY_PointHydroNexus.hpp>
#include <mpi.h>
#include <vector>

#include <unordered_map>


/** This class representa a point nexus that can have both upstream and downstream connections to catments that are
*   in seperate MPI processes.
*
*   When attempting to add upstream flows from a remote catchment a MPI_Irecv call will be generated
*   When attempting to send flows to remote downstream a MPI_Send will be generated
*   In either case the change in local water amounts for the time step will be recorded when the MPI operation completes */

class HY_PointHydroNexusRemote : public HY_PointHydroNexus
{
    public:
        /** This is map of catchment ids to MPI ranks it does not have to include any catchment that is local or any catchment that will not be
        * communicated with */

        typedef std::unordered_map <long, long> catcment_location_map_t;

        HY_PointHydroNexusRemote(int nexus_id_number, std::string nexus_id, int num_downstream, catcment_location_map_t loc_map);
        virtual ~HY_PointHydroNexusRemote();

        /** get the request percentage of downstream flow through this nexus at timestep t. If the indicated catchment is not local a async send will be
            created. Will attempt to process all async recieves currently queued before processing flows*/
        double get_downstream_flow(long catchment_id, time_step_t t, double percent_flow);

        /** add flow to this nexus for timestep t. If the indicated catchment is not local an async receive will be started*/
        void add_upstream_flow(double val, long catchment_id, time_step_t t);


        int get_world_rank();

        long get_time_step();


    private:
        void process_communications();

        int world_rank;

        long time_step;

        catcment_location_map_t catchment_id_to_mpi_rank;

        // Create the datatype
        MPI_Datatype time_step_and_flow_type;
        MPI_Datatype time_step_and_flow_precent_type;

        struct time_step_and_flow_t
        {
            long time_step;
            long catchment_id;
            double flow;
        };

        struct async_request
        {
            time_step_and_flow_t* buffer;
            MPI_Request mpi_request;
        };

        std::vector<async_request> stored_recieves;
        std::vector<async_request> stored_sends;

};

#endif // NGEN_MPI_ACTIVE
#endif // HY_POINTHYDRONEXUSREMOTEDOWNSTREAM_H
