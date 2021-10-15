#ifndef HY_POINTHDRONEXUSREMOTE_H
#define HY_POINTHDRONEXUSREMOTE_H

#ifdef NGEN_MPI_ACTIVE

#include <HY_PointHydroNexus.hpp>
#include <mpi.h>
#include <vector>

#include <unordered_map>
#include <string>


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

        typedef std::unordered_map <std::string, long> catcment_location_map_t;

        HY_PointHydroNexusRemote(std::string nexus_id, Catchments receiving_catchments, catcment_location_map_t loc_map);
        HY_PointHydroNexusRemote(std::string nexus_id, Catchments receiving_catchments, Catchments contributing_catchments, catcment_location_map_t loc_map);

        virtual ~HY_PointHydroNexusRemote();

        /** get the request percentage of downstream flow through this nexus at timestep t. If the indicated catchment is not local a async send will be
            created. Will attempt to process all async recieves currently queued before processing flows*/
        double get_downstream_flow(std::string catchment_id, time_step_t t, double percent_flow);

        /** add flow to this nexus for timestep t. If the indicated catchment is not local an async receive will be started*/
        void add_upstream_flow(double val, std::string catchment_id, time_step_t t);

        /** extract a numeric id from the catchment id for use as a mpi tag */
        static long extract(std::string s) {  return std::stoi(s.substr(4)); }
        
        const Catchments& get_local_contributing_catchments(){
            return local_contributers;
        };

		/** Test if this nexus is a sending nexus in a remote nexus pair */
        bool is_remote_sender()
        {
            return type == sender || type == sender_receiver;
        }


        int get_world_rank();

        long get_time_step();
        
        enum communication_type 
        {
        	local,
        	sender,
        	receiver,
            sender_receiver
        };
        
        /** return the communicator type for this nexus */
		communication_type get_communicator_type() { return type; }

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
            std::shared_ptr<time_step_and_flow_t> buffer;
            MPI_Request mpi_request;
        };

        std::list<async_request> stored_recieves;
        std::list<async_request> stored_sends;

        std::string nexus_prefix = "cat-";
        
        /** The type of communication at this nexus
         *
         */
        communication_type type;
        /** List of ranks we expect to send data to (downstream).
         *
         *  Note that in a dendridic network, downstream_ranks.size() == 1 (only one downstream receiver on a single rank)
         */
        std::unordered_set<int> downstream_ranks; //Set
        /** List of ranks we expect to receive data from
         * 
         */
        std::unordered_set<int> upstream_ranks; //Set
        /** Catchments local to the same MPI rank which contribute (call add_upstream_flow) to this nexus
         * 
         */
        Catchments local_contributers;
        /** Catchments NOT local to this rank which contribute (call add_upstream_flow) to this nexus
         * 
         */
        Catchments remote_contributers;
        /** Catchments local to the same MPI rank which receive (call get_downstream_flow) from this nexus
         * 
         */
        Catchments local_receivers;
        /** Catchments NOT local to this rank which receive (call get_downstream_flow) from this nexus
         * 
         */
        Catchments remote_receivers;
};

namespace std
{
	inline std::string to_string(HY_PointHydroNexusRemote::communication_type t)
	{
		switch(t)
		{
			case HY_PointHydroNexusRemote::communication_type::local:
			
			return std::string("local");
			break;
			
			case HY_PointHydroNexusRemote::communication_type::sender:

			return std::string("sender");
			break;
			
			case HY_PointHydroNexusRemote::communication_type::receiver:
			
			return std::string("receiver");
			break;
			
			case HY_PointHydroNexusRemote::communication_type::sender_receiver:
			
			return std::string("sender_receiver");
			break;
		}
	}
}

#endif // NGEN_MPI_ACTIVE
#endif // HY_POINTHYDRONEXUSREMOTEDOWNSTREAM_H
