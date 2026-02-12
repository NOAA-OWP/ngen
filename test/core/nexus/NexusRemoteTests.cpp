//#ifdef NGEN_MPI_TESTS_ACTIVE

#include "gtest/gtest.h"
#include "HY_PointHydroNexusRemote.hpp"


#include <vector>
#include <memory>

#include <unistd.h>

class Nexus_Remote_Test : public ::testing::Test
{

protected:

    Nexus_Remote_Test()
    {

    }

    ~Nexus_Remote_Test() override {

    }

    void SetUp() override;

    void TearDown() override;

    static void SetUpTestSuite()
    {
        MPI_Init(NULL, NULL);
        MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
        MPI_Comm_size(MPI_COMM_WORLD, &mpi_num_procs);
    }

    static void TearDownTestSuite()
    {
        MPI_Finalize();

        std::cerr << "Rank " << mpi_rank << " called finalize\n";
    }

    std::vector<double> stored_discharge;
    static int mpi_rank;
    static int mpi_num_procs;
};

int Nexus_Remote_Test::mpi_rank;
int Nexus_Remote_Test::mpi_num_procs;

void Nexus_Remote_Test::SetUp()
{
    //MPI_Init(NULL, NULL);
    //MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

    stored_discharge.push_back(1.2);
    stored_discharge.push_back(1.8);
    stored_discharge.push_back(2.3);
}

void Nexus_Remote_Test::TearDown()
{
}


//Test sending data with MPI from an upstream remote nexus
//to a downstream remote nexus.
TEST_F(Nexus_Remote_Test, TestInit0)
{
    if ( mpi_num_procs < 2 ) {
	    GTEST_SKIP();
    }

    HY_PointHydroNexusRemote::catcment_location_map_t loc_map;

    std::shared_ptr<HY_PointHydroNexusRemote> nexus;
    std::vector<std::string> upstream_catchments = {"cat-26"};
    std::vector<std::string> downstream_catchments = {"cat-27"};

    // create a nexus at both ranks
    if ( mpi_rank == 0)
    {
        loc_map["cat-27"] = 1;
        nexus = std::make_shared<HY_PointHydroNexusRemote>("nex-26", downstream_catchments, upstream_catchments, loc_map);
    }
    else if ( mpi_rank == 1)
    {
        loc_map["cat-26"] = 0;
        //loc_map["cat-27"] = 1;
        nexus = std::make_shared<HY_PointHydroNexusRemote>("nex-26", downstream_catchments, upstream_catchments, loc_map);
    }

    double dummy_flow = -9999.0;
    long ts = 0;

    for ( auto discharge : stored_discharge)
    {
        switch(mpi_rank)
        {
            case 0:
                std::cerr << "Rank 0: Sending flow of " << discharge << " to catchment 26\n";
                nexus->add_upstream_flow(discharge,"cat-26",ts);
                //nexus->get_downstream_flow("cat-27",ts,100);
            break;

            case 1:
                //nexus->add_upstream_flow(dummy_flow,"cat-26",ts);
                double received_flow = nexus->get_downstream_flow("cat-27",ts,100);
                ASSERT_EQ(discharge,received_flow);
                std::cerr << "Rank 1: Recieving flow of " << received_flow << " from catchment Nexus connected to catchment 26\n";
            break;
        }

        ++ts;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    //delete nexus;

    ASSERT_TRUE(true);

}

//Test sending data with MPI from an two upstream remote nexus
//to a downstream remote nexus.
TEST_F(Nexus_Remote_Test, Test2RemoteSenders)
{
    if ( mpi_num_procs < 3 )
    {
    	GTEST_SKIP();
    }
    
    HY_PointHydroNexusRemote::catcment_location_map_t loc_map;

    std::shared_ptr<HY_PointHydroNexusRemote> nexus;
    std::vector<std::string> upstream_catchments;
    std::vector<std::string> downstream_catchments = {"cat-27"};
    // create a nexus at both ranks
    if ( mpi_rank == 0)
    {
        upstream_catchments.push_back("cat-25");
        upstream_catchments.push_back("cat-26");
        downstream_catchments.push_back("cat-27");
        
        loc_map["cat-25"] = 1;
        loc_map["cat-26"] = 2;
        nexus = std::make_shared<HY_PointHydroNexusRemote>("nex-27", downstream_catchments, upstream_catchments, loc_map);
    }
    else if ( mpi_rank == 1)
    {
        upstream_catchments.push_back("cat-25");
        downstream_catchments.push_back("cat-27");
        
        loc_map["cat-27"] = 0;
        nexus = std::make_shared<HY_PointHydroNexusRemote>("nex-27", downstream_catchments, upstream_catchments, loc_map);
    }
    else if ( mpi_rank == 2)
    {
        upstream_catchments.push_back("cat-26");
        downstream_catchments.push_back("cat-27");
        
        loc_map["cat-27"] = 0;
        nexus = std::make_shared<HY_PointHydroNexusRemote>("nex-27", downstream_catchments, upstream_catchments, loc_map);
    }    

    double dummy_flow = -9999.0;
    long ts = 0;
    double received_flow = -9999.0;

    for ( auto discharge : stored_discharge)
    {
        switch(mpi_rank)
        {
            
            case 0:
                received_flow = nexus->get_downstream_flow("cat-27",ts,100);
                ASSERT_EQ(discharge+discharge,received_flow);
                std::cerr << "Rank 0: Recieving flow of " << received_flow << " from catchment Nexus connected to catchment 27\n";
            break;
            
            case 1:
                std::cerr << "Rank 1: Sending flow of " << discharge << " to catchment 27\n";
                nexus->add_upstream_flow(discharge,"cat-25",ts);
            break;

            case 2:
                std::cerr << "Rank 2: Sending flow of " << discharge << " to catchment 27\n";
                nexus->add_upstream_flow(discharge,"cat-26",ts);
            break;
            
            
        }

        ++ts;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    //delete nexus;

    ASSERT_TRUE(true);
}

//Test sending data with MPI from an two upstream remote nexi
//to a downstream remote nexus with one upstream local catchment.
TEST_F(Nexus_Remote_Test, Test2RemoteSenders1LocalSender)
{
    if ( mpi_num_procs < 3 )
    {
    	GTEST_SKIP();
    }
    
    HY_PointHydroNexusRemote::catcment_location_map_t loc_map;

    std::shared_ptr<HY_PointHydroNexusRemote> nexus;
    std::vector<std::string> upstream_catchments;
    std::vector<std::string> downstream_catchments = {"cat-27"};
    // create a nexus at both ranks
    if ( mpi_rank == 0)
    {
        upstream_catchments.push_back("cat-24");
        upstream_catchments.push_back("cat-25");
        upstream_catchments.push_back("cat-26");
        downstream_catchments.push_back("cat-27");
        
        loc_map["cat-25"] = 1;
        loc_map["cat-26"] = 2;
        nexus = std::make_shared<HY_PointHydroNexusRemote>("nex-27", downstream_catchments, upstream_catchments, loc_map);
    }
    else if ( mpi_rank == 1)
    {
        upstream_catchments.push_back("cat-25");
        downstream_catchments.push_back("cat-27");
        
        loc_map["cat-27"] = 0;
        nexus = std::make_shared<HY_PointHydroNexusRemote>("nex-27", downstream_catchments, upstream_catchments, loc_map);
    }
    else if ( mpi_rank == 2)
    {
        upstream_catchments.push_back("cat-26");
        downstream_catchments.push_back("cat-27");
        
        loc_map["cat-27"] = 0;
        nexus = std::make_shared<HY_PointHydroNexusRemote>("nex-27", downstream_catchments, upstream_catchments, loc_map);
    }    

    double dummy_flow = -9999.0;
    long ts = 0;
    double received_flow = -9999.0;

    for ( auto discharge : stored_discharge)
    {
        switch(mpi_rank)
        {
            
            case 0:
                nexus->add_upstream_flow(discharge,"cat-24",ts);
                received_flow = nexus->get_downstream_flow("cat-27",ts,100);
                ASSERT_EQ(discharge*3,received_flow);
                std::cerr << "Rank 0: Recieving flow of " << received_flow << " from catchment Nexus connected to catchment 27\n";
            break;
            
            case 1:
                std::cerr << "Rank 1: Sending flow of " << discharge << " to catchment 27\n";
                nexus->add_upstream_flow(discharge,"cat-25",ts);
            break;

            case 2:
                std::cerr << "Rank 2: Sending flow of " << discharge << " to catchment 27\n";
                nexus->add_upstream_flow(discharge,"cat-26",ts);
            break;
            
            
        }

        ++ts;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    //delete nexus;

    ASSERT_TRUE(true);
}

//Test sending data with MPI from an four upstream remote nexi
//to two differnt downstream remote nexus each of which has a one upstream local catchment.
TEST_F(Nexus_Remote_Test, Test4R2S2LS)
{
    if ( mpi_num_procs < 6 )
    {
    	GTEST_SKIP();
    }
    
    HY_PointHydroNexusRemote::catcment_location_map_t loc_map;

    std::shared_ptr<HY_PointHydroNexusRemote> nexus;
    std::vector<std::string> upstream_catchments;
    std::vector<std::string> downstream_catchments = {"cat-27"};
    // create a nexus at both ranks
    if ( mpi_rank == 0)
    {
        upstream_catchments.push_back("cat-24");
        upstream_catchments.push_back("cat-25");
        upstream_catchments.push_back("cat-26");
        downstream_catchments.push_back("cat-27");
        
        loc_map["cat-25"] = 1;
        loc_map["cat-26"] = 2;
        nexus = std::make_shared<HY_PointHydroNexusRemote>("nex-27", downstream_catchments, upstream_catchments, loc_map);
    }
    else if ( mpi_rank == 1)
    {
        upstream_catchments.push_back("cat-25");
        downstream_catchments.push_back("cat-27");
        
        loc_map["cat-27"] = 0;
        nexus = std::make_shared<HY_PointHydroNexusRemote>("nex-27", downstream_catchments, upstream_catchments, loc_map);
    }
    else if ( mpi_rank == 2)
    {
        upstream_catchments.push_back("cat-26");
        downstream_catchments.push_back("cat-27");
        
        loc_map["cat-27"] = 0;
        nexus = std::make_shared<HY_PointHydroNexusRemote>("nex-27", downstream_catchments, upstream_catchments, loc_map);
    }
    if ( mpi_rank == 3)
    {
        upstream_catchments.push_back("cat-14");
        upstream_catchments.push_back("cat-15");
        upstream_catchments.push_back("cat-16");
        downstream_catchments.push_back("cat-17");
        
        loc_map["cat-15"] = 4;
        loc_map["cat-16"] = 5;
        nexus = std::make_shared<HY_PointHydroNexusRemote>("nex-17", downstream_catchments, upstream_catchments, loc_map);
    }
    else if ( mpi_rank == 4)
    {
        upstream_catchments.push_back("cat-15");
        downstream_catchments.push_back("cat-17");
        
        loc_map["cat-17"] = 3;
        nexus = std::make_shared<HY_PointHydroNexusRemote>("nex-17", downstream_catchments, upstream_catchments, loc_map);
    }
    else if ( mpi_rank == 5)
    {
        upstream_catchments.push_back("cat-16");
        downstream_catchments.push_back("cat-17");
        
        loc_map["cat-17"] = 3;
        nexus = std::make_shared<HY_PointHydroNexusRemote>("nex-17", downstream_catchments, upstream_catchments, loc_map);
    }         

    double dummy_flow = -9999.0;
    long ts = 0;
    double received_flow = -9999.0;

    for ( auto discharge : stored_discharge)
    {
        switch(mpi_rank)
        {
            
            case 0:
                nexus->add_upstream_flow(discharge,"cat-24",ts);
                received_flow = nexus->get_downstream_flow("cat-27",ts,100);
                ASSERT_EQ(discharge*3,received_flow);
                std::cerr << "Rank 0: Recieving flow of " << received_flow << " from catchment Nexus connected to catchment 27\n";
            break;
            
            case 1:
                std::cerr << "Rank 1: Sending flow of " << discharge << " to catchment 27\n";
                nexus->add_upstream_flow(discharge,"cat-25",ts);
            break;

            case 2:
                std::cerr << "Rank 2: Sending flow of " << discharge << " to catchment 27\n";
                nexus->add_upstream_flow(discharge,"cat-26",ts);
            break;
            
            case 3:
                nexus->add_upstream_flow(discharge,"cat-14",ts);
                received_flow = nexus->get_downstream_flow("cat-17",ts,100);
                ASSERT_EQ(discharge*3,received_flow);
                std::cerr << "Rank 3: Recieving flow of " << received_flow << " from catchment Nexus connected to catchment 27\n";
            break;
            
            case 4:
                std::cerr << "Rank 4: Sending flow of " << discharge << " to catchment 17\n";
                nexus->add_upstream_flow(discharge,"cat-15",ts);
            break;

            case 5:
                std::cerr << "Rank 5: Sending flow of " << discharge << " to catchment 27\n";
                nexus->add_upstream_flow(discharge,"cat-16",ts);
            break;
            
            
        }

        ++ts;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    //delete nexus;

    ASSERT_TRUE(true);
}

TEST_F(Nexus_Remote_Test, TestDeadlock1)
{
    if ( mpi_num_procs < 2 ) {
	    GTEST_SKIP();
    }

    HY_PointHydroNexusRemote::catcment_location_map_t loc_map;

    std::shared_ptr<HY_PointHydroNexusRemote> nexus1;
    std::shared_ptr<HY_PointHydroNexusRemote> nexus2;

    double dummy_flow = -9999.0;
    double received_flow;
    long ts = 0;

    std::vector<std::string> downstream_catchments;
    std::vector<std::string> upstream_catchments;

    // In this test both processes send before recieving. If communciation are synchronus this will dead lock

    // create a nexus at both ranks
    if ( mpi_rank == 0)
    {
        downstream_catchments.push_back("cat-26");
        upstream_catchments.push_back("cat-25");
        
        loc_map["cat-26"] = 1;
        nexus1 = std::make_shared<HY_PointHydroNexusRemote>("nex-26", downstream_catchments, upstream_catchments, loc_map);
        nexus2 = std::make_shared<HY_PointHydroNexusRemote>("nex-26", upstream_catchments, downstream_catchments, loc_map);

        // We use two differnt time steps becuase a nexus does not allow water to be added after a send
        nexus1->add_upstream_flow(200.0,"cat-25",ts);						// sending to rank 1
                              
        received_flow = nexus2->get_downstream_flow("cat-26",ts,100);       // get the received flow
        std::cout << "rank 0 received a flow of " << received_flow << "\n";
    }
    else if ( mpi_rank == 1)
    {
        downstream_catchments.push_back("cat-25");
        upstream_catchments.push_back("cat-26");
        
        loc_map["cat-25"] = 0;
        nexus1 = std::make_shared<HY_PointHydroNexusRemote>("nex-26", downstream_catchments, upstream_catchments, loc_map);
        nexus2 = std::make_shared<HY_PointHydroNexusRemote>("nex-26", upstream_catchments, downstream_catchments, loc_map);

        // We use two differnt time steps becuase a nexus does not allow water to be added after a send
        nexus1->add_upstream_flow(200.0,"cat-26",ts);						// sending to rank 0
                              
        received_flow = nexus2->get_downstream_flow("cat-25",ts,100);       // get the received flow
        std::cout << "rank 1 received a flow of " << received_flow << "\n";
    }

    MPI_Barrier(MPI_COMM_WORLD);
}

TEST_F(Nexus_Remote_Test, DISABLED_TestTree1)
{
    int tree_height = 2;
    int num_nodes = std::pow(2,tree_height) - 1;  // the number of nodes in a complete binary tree of height h
    int num_non_leaf_nodes = std::pow(2,tree_height-1) - 1;
    int start_id = num_nodes;
    int stop_id = -1;

    int nodes_per_rank = std::round( double(num_nodes) / mpi_num_procs );

    HY_PointHydroNexusRemote::catcment_location_map_t loc_map;

    //std::cerr << "-----Rank " << mpi_rank << " Creating location map\n";
    // create the location map for use with nexi at this process
    for( int i = 0; i < num_nodes; ++i)
    {
        int target_rank = static_cast<int>(i / nodes_per_rank);
        if ( target_rank != mpi_rank )
        {
            loc_map[std::string("cat-"+std::to_string(i))] = target_rank;
        }
        else
        {
            if (i < start_id ) start_id = i;
            if (i > stop_id ) stop_id = i;
        }
    }
    //std::cerr << "------Rank " << mpi_rank << " Finished creating location map\n";

    MPI_Barrier(MPI_COMM_WORLD);

    // closure functions for determining tree in array positions

    auto left = [](int N){return 2*N+1;};           // left child postion
    auto right = [](int N){return 2*N+2;};          // right child postion
    auto parent = [](int N){return (N-1) / 2;};     // parent position
    auto valid = [num_nodes](int N)                 // is this id valid?
    {
        return N < num_nodes;
    };
    auto leaf = [num_non_leaf_nodes,num_nodes](int N)                 // is this id a leaf?
    {
        return num_non_leaf_nodes <= N && N < num_nodes;
    };

    // now create the nexus objects that are local

    std::unordered_map<int, std::shared_ptr<HY_PointHydroNexusRemote> > nexus_map;

    //std::cerr << "-----Rank " << mpi_rank << " Creating nexus objects\n";
    //std::cerr << "-----Rank " << mpi_rank << " Start Id " << start_id << std::endl;;
    //std::cerr << "-----Rank " << mpi_rank << " Stop Id " << stop_id << std::endl;

    for( int i = start_id; i <= stop_id; ++i)
    {
        std::vector<std::string> downstream_catchments;
        std::vector<std::string> upstream_catchments;
        HY_PointHydroNexusRemote::catcment_location_map_t local_map;
        std::string nex_id = "nex-"+std::to_string(i);
        std::string downstream_id;

        //catchments.push_back(std::string("cat-"+std::to_string(i)));
        if ( leaf(i) )
        {
        	upstream_catchments.push_back("forcing");
        	downstream_id = "cat-"+std::to_string(parent(i));
        	downstream_catchments.push_back(downstream_id);
        	
        	std::string parent_id = "cat-"+std::to_string(parent(i));
        	
        	if ( loc_map[parent_id] != mpi_rank )
            {
            	local_map[parent_id] = loc_map[parent_id];
            }	
        }
        else
        {
        	std::string left_id = "cat-"+std::to_string(left(i));
        	std::string right_id = "cat-"+std::to_string(right(i));
        	
            upstream_catchments.push_back(left_id);
            upstream_catchments.push_back(right_id);
            
            if ( loc_map[left_id] != mpi_rank )
            {
            	local_map[left_id] = loc_map[left_id];
            }
            
            if ( loc_map[right_id] != mpi_rank )
            {
            	local_map[right_id] = loc_map[right_id];
            }
            
            downstream_id = "cat-"+std::to_string(parent(i));
            std::string parent_id = "cat-"+std::to_string(left(i));
        	
        	if ( loc_map[parent_id] != mpi_rank )
            {
            	local_map[parent_id] = loc_map[parent_id];
            }
        }
        
        nexus_map[i] = std::make_shared<HY_PointHydroNexusRemote>(nex_id, downstream_catchments, upstream_catchments, local_map);
        
                
        std::cerr << "mpi rank: " << mpi_rank << " constucted nexus with type = " 
        	      << std::to_string(nexus_map[i]->get_communicator_type()) << " at tree position " << i << "\n";
        std::cerr << "upstream catchments = [ " ;
        for ( std::size_t i = 0; i < upstream_catchments.size(); ++i )
        	std::cerr << upstream_catchments[i] << ",";
        std::cerr << "\b] for node=" << i << "\n";
        
        std::cerr << "downstream catchments = [ " ;
        for ( std::size_t i = 0; i < downstream_catchments.size(); ++i )
        	std::cerr << downstream_catchments[i] << ",";
        std::cerr << "\b] for node=" << i << "\n";
        
        std::cerr << "local map = [ " ;
        for ( auto p : local_map )
        	std::cerr << p.first << ":" << p.second << ",";
        std::cerr << "\b] for node=" << i << "\n";
    }

    std::cerr << "-----Rank " << mpi_rank << " Finshed creating nexus objects\n";


    long ts = 0;

    // for each nexus accept upstream flow
    for( int i = stop_id; i >= start_id; --i )
    {
        std::cerr << "-----Rank " << mpi_rank << " Processing nexus object at position " << i << std::endl;

        if ( leaf(i) )
        {
            std::cerr << "mpi rank: " << mpi_rank << " adding forcing at node " << i << "\n";
            // if this is a leaf we need a synthetic flow
            nexus_map[i]->add_upstream_flow(1.0,"forcing", ts);

        }
        else
        {
            std::cerr << "mpi rank: " << mpi_rank << " processing node " << i << "\n";
            // if this is not a leaf try to get flow from its upstreams

            int l = left(i);
            int r = right(i);
            int p = parent(i);

            std::string current_id = std::string("cat-"+std::to_string(i));
            std::string left_id = std::string("cat-"+std::to_string(l));
            std::string right_id = std::string("cat-"+std::to_string(r));
            std::string p_id = std::string("cat-"+std::to_string(p));
            float dummy_value;
            float flow = 0.0;

            try
            {
            	std::cerr << "mpi rank: " << mpi_rank << " processing node " << i << " left \n";
            	float f = nexus_map.at(l)->get_downstream_flow(current_id, ts, 100.0);
            	flow += f;
            	nexus_map[i]->add_upstream_flow(f, left_id, ts );
            }
            catch (const std::out_of_range& oor) 
            {
				// l is not this partition
  			}
  			
  			try
            {
            	std::cerr << "mpi rank: " << mpi_rank << " processing node " << i << " right \n";
            	float f = nexus_map.at(r)->get_downstream_flow(current_id, ts, 100.0);
            	flow += f;
            	nexus_map[i]->add_upstream_flow(flow, right_id, ts );
            }
            catch (const std::out_of_range& oor) 
            {
		
  			}
  			
  			try
            {
            	if ( p != i )
            	{
            		std::cerr << "mpi rank: " << mpi_rank << " processing node " << i << "parent \n";
            		//flow = nexus_map[i]->get_downstream_flow(p_id,ts,100.0);
            		std::cerr << "p = " << p << "\n";
            		std::cerr << "i = " << i << "\n";
            		nexus_map.at(p)->add_upstream_flow(flow, current_id, ts );
            	}
            }
            catch (const std::out_of_range& oor) 
            {
		
  			}
  			
  			
        }

        //std::cerr << "-----Rank " << mpi_rank << " Finished processing nexus object at position " << i << std::endl;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if ( nexus_map.find(0) != nexus_map.end() )
    {
        double flow = 0.0;
        flow = nexus_map[0]->get_downstream_flow("cat-0", ts, 100.0);
        std::cerr << "Rank " << mpi_rank<< " final Flow = " << flow << std::endl;

        ASSERT_TRUE(flow == 512);
    }

    //std::cerr << "Process id " << getpid() << " Rank " << mpi_rank << " terminating\n";
}



/******************************************************************************
 * MPI DEADLOCK TESTS
 * ==================
 * As of commit 9ad6b7bf561fb2f96065511b382bc43a83167f10
 * A potential MPI deadlock scenario was discovered in the HY_PointHydroNexusRemote class.
 *
 * These tests demonstrate and verify a fix for MPI communication issues
 * observed in production when running large domains with many partitions.
 * 
 * =============================================================================
 * What can be shown:
 * =============================================================================
 * 
 * 1. Deadlock prone code:
 *    The original HY_PointHydroNexusRemote::add_upstream_flow() does:
 *      a. MPI_Isend() - non-blocking, returns immediately
 *      b. while(stored_sends.size() > 0) { MPI_Test(); } - BLOCKS until
 *         the send is confirmed complete by MPI
 *    
 *    This pattern is problematic because it blocks progress until the send
 *    completes, preventing the code from posting receives.
 * 
 * 2. WITH FORCED RENDEZVOUS, THIS PATTERN DEADLOCKS:
 *    When we force rendezvous protocol using --mca btl_tcp_eager_limit 80,
 *    the buggy pattern reliably deadlocks. Rendezvous requires a posted
 *    MPI_Irecv before the send can complete.
 * 
 * 3. PRODUCTION RUNS WERE HANGING:
 *    Large-scale runs with 384+ partitions and 831K catchments  __across multiple nodes__
 *    were observed to be hanging (one node was making progress while others were not).
 *
 * 4. THE FIX IS CORRECT:
 *    Pre-posting receives (calling MPI_Irecv before MPI_Isend) is the standard
 *    MPI best practice and eliminates any rendezvous-related deadlock risk.
 * 
 * =============================================================================
 * Assumptions about this fix that are hard to confirm:
 * =============================================================================
 * 
 * We ASSUMED that production hangs occurred because:
 *   - High connection count (~38,000 connections) exhausted the eager buffer pool
 *   - This forced MPI to use rendezvous protocol even for small messages
 *   - Rendezvous + buggy pattern = deadlock
 * 
 * HOWEVER: MPI documentation consistently states that rendezvous protocol is
 * triggered by MESSAGE SIZE exceeding eager_limit, NOT by buffer pool exhaustion.
 * We cannot find documentation supporting the "pool exhaustion triggers
 * rendezvous" theory.
 * 
 * Other possibilities for production hangs (unconfirmed)
 *   - TCP buffer exhaustion: if the sender's socket buffer fills up before
 *     the receiver can process messages, subsequent sends can block.
 *     And if two or more nodes get into this state, they can deadlock.
 *   - Network fabric issues at scale
 *   - Something else entirely that our fix happened to address
 * 
 * =============================================================================
 * THE FIX IS STILL CORRECT:
 * =============================================================================
 * 
 * Regardless of the exact production trigger, pre-posting receives is:
 *   1. MPI best practice for avoiding deadlock
 *   2. Required for correctness under rendezvous protocol
 *   3. Harmless under eager protocol (just posts receives earlier)
 * 
 * The fix eliminates a class of potential deadlocks even if we're uncertain
 * about the exact mechanism that triggered the production hang.
 * 
 * =============================================================================
 * TRIGGERING THE DEADLOCK IN TESTS:
 * =============================================================================
 * 
 * We use --mca btl_tcp_eager_limit 80 to FORCE rendezvous protocol per-message.
 * This is a TEST WORKAROUND that demonstrates the buggy pattern CAN deadlock.
 * 
 * This is NOT necessarily the exact production failure mode - it's a way to
 * reliably trigger the deadlock pattern in a controlled test environment.
 * 
 * To reproduce in tests, force tcp communcation and set the eager limit to
 * the minimum value (80 bytes -- openmpi_info may show different limits for 
 * different BTLs/environments).
 * 
 *   mpirun --mca btl tcp,self --mca btl_tcp_eager_limit 80 ...
 * 
 * Parameters:
 *   --mca btl tcp,self           : Disable shared memory, use TCP only
 *   --mca btl_tcp_eager_limit 80 : Force rendezvous per-message (minimum value)
 * 
 * NOTE: Shared memory BTL uses copy-based communication that doesn't require
 * a synchronous handshake, so --mca btl_sm_eager_limit 80 will NOT trigger
 * the deadlock.
 * 
 ******************************************************************************/


/**
 * =============================================================================
 * DISABLED_TestRawMpiDeadlockPattern
 * =============================================================================
 * 
 * PURPOSE: Document and demonstrate the MPI communication pattern that causes
 * deadlock when rendezvous protocol is triggered.
 * This is the pattern that ngen used up to and including
 * commit 9ad6b7bf561fb2f96065511b382bc43a83167f10
 * This test uses RAW MPI calls (not the HY_PointHydroNexusRemote class) to
 * clearly illustrate the problematic pattern that existed in the original code.
 * 
 * =============================================================================
 * THE PROBLEMATIC PATTERN (from original add_upstream_flow):
 * =============================================================================
 *   1. MPI_Isend (non-blocking send)
 *   2. Loop on MPI_Test waiting for send to complete  <-- BLOCKS HERE
 *   3. MPI_Irecv (never reached if step 2 blocks)
 * 
 * =============================================================================
 * WHY NGEN PARTITIONS CREATE BIDIRECTIONAL COMMUNICATION:
 * =============================================================================
 * 
 * Real hydrological networks are complex. When we partition the domain, the
 * partition boundary cuts ACROSS the drainage network, not along it.
 * This creates BIDIRECTIONAL communication between partitions.
 * 
 * Analysis of CONUS (384 partitions, 831K catchments) confirms:
 *   - 182/384 partitions (47%) have BIDIRECTIONAL communication
 * 
 * =============================================================================
 * TEST TOPOLOGY (simplified bidirectional chain):
 * =============================================================================
 * 
 *   Rank 0 <────> Rank 1 <────> Rank 2 <────> Rank 3
 *   
 *   Each rank both SENDS to AND RECEIVES from its neighbors.
 * 
 * =============================================================================
 * TO REPRODUCE DEADLOCK:
 * =============================================================================
 *   timeout 10 mpirun --mca btl tcp,self --mca btl_tcp_eager_limit 80 -n 4 \
 *       ./test/test_remote_nexus --gtest_filter="*TestRawMpiDeadlockPattern*" \
 *       --gtest_also_run_disabled_tests
 * 
 * EXPECTED: Exit code 124 (timeout killed it) - confirms true deadlock
 * 
 * =============================================================================
 */
TEST_F(Nexus_Remote_Test, DISABLED_TestRawMpiDeadlockPattern)
{
    if (mpi_num_procs < 4) {
        GTEST_SKIP() << "Requires at least 4 MPI ranks to demonstrate bidirectional deadlock";
    }
    
    // Bidirectional chain topology
    std::vector<int> downstream_ranks;
    std::vector<int> upstream_ranks;
    
    if (mpi_rank == 0) {
        downstream_ranks.push_back(1);
        upstream_ranks.push_back(1);
    } else if (mpi_rank == 1) {
        downstream_ranks.push_back(0);
        downstream_ranks.push_back(2);
        upstream_ranks.push_back(0);
        upstream_ranks.push_back(2);
    } else if (mpi_rank == 2) {
        downstream_ranks.push_back(1);
        downstream_ranks.push_back(3);
        upstream_ranks.push_back(1);
        upstream_ranks.push_back(3);
    } else if (mpi_rank == 3) {
        downstream_ranks.push_back(2);
        upstream_ranks.push_back(2);
    }
    
    bool is_sender = !downstream_ranks.empty();
    bool is_receiver = !upstream_ranks.empty();
    
    // Create MPI datatype for our message
    struct message_t {
        long time_step;
        long id;
        double flow;
    };
    
    MPI_Datatype msg_type;
    int counts[3] = {1, 1, 1};
    MPI_Aint displacements[3] = {0, sizeof(long), 2 * sizeof(long)};
    MPI_Datatype types[3] = {MPI_LONG, MPI_LONG, MPI_DOUBLE};
    MPI_Type_create_struct(3, counts, displacements, types, &msg_type);
    MPI_Type_commit(&msg_type);
    
    const int NUM_MESSAGES = 3500;
    const int TAG_BASE = 1000;
    
    std::cerr << "Rank " << mpi_rank << ": Starting bidirectional deadlock pattern\n";
    
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Storage for async operations
    std::map<int, std::vector<message_t>> send_buffers;
    std::map<int, std::vector<message_t>> recv_buffers;
    std::map<int, std::vector<MPI_Request>> send_requests;
    std::map<int, std::vector<MPI_Request>> recv_requests;
    
    for (int r : downstream_ranks) {
        send_buffers[r].resize(NUM_MESSAGES);
        send_requests[r].resize(NUM_MESSAGES);
    }
    for (int r : upstream_ranks) {
        recv_buffers[r].resize(NUM_MESSAGES);
        recv_requests[r].resize(NUM_MESSAGES);
    }
    
    // Timing asymmetry emulates "work" that causes ranks to run at different "speeds"
    if (mpi_rank == 1) {
        volatile double dummy = 0.0;
        for (int i = 0; i < 100000000; ++i) {
            dummy += std::sin(i * 0.0001) * std::cos(i * 0.0002);
        }
    }
    
    // THE PROBLEMATIC PATTERN: All ranks try to complete sends BEFORE posting receives
    if (is_sender)
    {
        for (int downstream : downstream_ranks)
        {
            for (int i = 0; i < NUM_MESSAGES; ++i)
            {
                send_buffers[downstream][i] = {i, mpi_rank, 100.0 + i};
                int tag = TAG_BASE + mpi_rank * 10000 + downstream * 100 + i;
                
                MPI_Isend(&send_buffers[downstream][i], 1, msg_type, downstream, tag,
                          MPI_COMM_WORLD, &send_requests[downstream][i]);
                
                // BLOCKING WAIT - THIS IS THE "BUG"!
                // Under eager protocols, this test returns immediately
                // Under rendezvous protocols, this test blocks until 
                // the receiver posts a matching Irecv.
                // Similar logic applies to a full TCP buffer, MPI gets blocked waiting
                // for TCP buffer to free up, which requires the receiver to 
                // read the data.
                int flag = 0;
                while (!flag) {
                    MPI_Test(&send_requests[downstream][i], &flag, MPI_STATUS_IGNORE);
                }
            }
        }
    }
    
    // Post receives - NEVER REACHED IN DEADLOCK
    if (is_receiver)
    {
        for (int upstream : upstream_ranks)
        {
            for (int i = 0; i < NUM_MESSAGES; ++i)
            {
                int tag = TAG_BASE + upstream * 10000 + mpi_rank * 100 + i;
                MPI_Irecv(&recv_buffers[upstream][i], 1, msg_type, upstream, tag,
                          MPI_COMM_WORLD, &recv_requests[upstream][i]);
            }
            MPI_Waitall(NUM_MESSAGES, recv_requests[upstream].data(), MPI_STATUSES_IGNORE);
        }
    }
    
    MPI_Type_free(&msg_type);
    MPI_Barrier(MPI_COMM_WORLD);
    std::cerr << "Rank " << mpi_rank << ": Test passed (eager buffer was sufficient)\n";
}


/**
 * =============================================================================
 * TestRemoteNexusDeadlockFree
 * =============================================================================
 * 
 * PURPOSE: Verify that HY_PointHydroNexusRemote does NOT deadlock, even with
 * extremely small MPI eager buffers that force rendezvous protocol.
 * 
 * This test uses the SAME BIDIRECTIONAL topology as DISABLED_TestRawMpiDeadlockPattern
 * but uses the HY_PointHydroNexusRemote class instead of raw MPI calls.
 * 
 * =============================================================================
 * THE FIX: AUTO-POSTED RECEIVES
 * =============================================================================
 * 
 * The HY_PointHydroNexusRemote class now auto-posts MPI_Irecv BEFORE sending.
 * This breaks the deadlock cycle because peers can always complete their sends.
 * 
 * =============================================================================
 * TO RUN (with small eager buffer to force rendezvous):
 * =============================================================================
 *   mpirun --mca btl tcp,self --mca btl_tcp_eager_limit 80 -n 4 \
 *       ./test/test_remote_nexus --gtest_filter="*TestRemoteNexusDeadlockFree*"
 * 
 * EXPECTED: Exit code 0 - test passes without deadlock
 * 
 * =============================================================================
 */
TEST_F(Nexus_Remote_Test, TestRemoteNexusDeadlockFree)
{
    if (mpi_num_procs < 4) {
        GTEST_SKIP() << "Requires at least 4 MPI ranks for bidirectional topology";
    }
    
    // Same bidirectional topology as the deadlock test
    std::vector<int> downstream_ranks;
    std::vector<int> upstream_ranks;
    
    if (mpi_rank == 0) {
        downstream_ranks.push_back(1);
        upstream_ranks.push_back(1);
    } else if (mpi_rank == 1) {
        downstream_ranks.push_back(0);
        downstream_ranks.push_back(2);
        upstream_ranks.push_back(0);
        upstream_ranks.push_back(2);
    } else if (mpi_rank == 2) {
        downstream_ranks.push_back(1);
        downstream_ranks.push_back(3);
        upstream_ranks.push_back(1);
        upstream_ranks.push_back(3);
    } else if (mpi_rank == 3) {
        downstream_ranks.push_back(2);
        upstream_ranks.push_back(2);
    }
    
    bool is_sender = !downstream_ranks.empty();
    bool is_receiver = !upstream_ranks.empty();
    
    const int NUM_CONNECTIONS = 50;
    
    std::cerr << "Rank " << mpi_rank << ": Setting up bidirectional topology\n";
    
    // Create sender nexuses for each downstream rank
    std::map<int, std::vector<std::shared_ptr<HY_PointHydroNexusRemote>>> senders;
    for (int downstream : downstream_ranks)
    {
        for (int i = 0; i < NUM_CONNECTIONS; ++i)
        {
            int nex_num = mpi_rank * 100000 + downstream * 1000 + i;
            std::string nex_id = "nex-" + std::to_string(nex_num);
            std::string my_cat = "cat-send-" + std::to_string(mpi_rank) + "-" + std::to_string(downstream) + "-" + std::to_string(i);
            std::string their_cat = "cat-recv-" + std::to_string(downstream) + "-" + std::to_string(mpi_rank) + "-" + std::to_string(i);
            
            HY_PointHydroNexusRemote::catcment_location_map_t loc_map;
            loc_map[their_cat] = downstream;
            
            std::vector<std::string> receiving = {their_cat};
            std::vector<std::string> contributing = {my_cat};
            
            senders[downstream].push_back(std::make_shared<HY_PointHydroNexusRemote>(
                nex_id, receiving, contributing, loc_map));
        }
    }
    
    // Create receiver nexuses for each upstream rank
    std::map<int, std::vector<std::shared_ptr<HY_PointHydroNexusRemote>>> receivers;
    for (int upstream : upstream_ranks)
    {
        for (int i = 0; i < NUM_CONNECTIONS; ++i)
        {
            int nex_num = upstream * 100000 + mpi_rank * 1000 + i;
            std::string nex_id = "nex-" + std::to_string(nex_num);
            std::string their_cat = "cat-send-" + std::to_string(upstream) + "-" + std::to_string(mpi_rank) + "-" + std::to_string(i);
            std::string my_cat = "cat-recv-" + std::to_string(mpi_rank) + "-" + std::to_string(upstream) + "-" + std::to_string(i);
            
            HY_PointHydroNexusRemote::catcment_location_map_t loc_map;
            loc_map[their_cat] = upstream;
            
            std::vector<std::string> receiving = {my_cat};
            std::vector<std::string> contributing = {their_cat};
            
            receivers[upstream].push_back(std::make_shared<HY_PointHydroNexusRemote>(
                nex_id, receiving, contributing, loc_map));
        }
    }
    
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Timing asymmetry emulates "work" that causes ranks to run at different "speeds"
    if (mpi_rank == 1) {
        volatile double dummy = 0.0;
        for (int i = 0; i < 100000000; ++i) {
            dummy += std::sin(i * 0.0001) * std::cos(i * 0.0002);
        }
    }

    long ts = 0;
    double flow_value = 42.0;
    
    // Send all flows - with the fix, receives are auto-posted before sending
    if (is_sender)
    {
        std::cerr << "Rank " << mpi_rank << ": Starting sends (receives auto-posted by fix)\n";
        
        for (auto& kv : senders)
        {
            int downstream = kv.first;
            auto& nexuses = kv.second;
            for (int i = 0; i < NUM_CONNECTIONS; ++i)
            {
                std::string my_cat = "cat-send-" + std::to_string(mpi_rank) + "-" + std::to_string(downstream) + "-" + std::to_string(i);
                nexuses[i]->add_upstream_flow(flow_value, my_cat, ts);
            }
        }
        std::cerr << "Rank " << mpi_rank << ": All sends completed!\n";
    }
    
    // Receive all flows
    if (is_receiver)
    {
        std::cerr << "Rank " << mpi_rank << ": Receiving from upstream\n";
        for (auto& kv : receivers)
        {
            int upstream = kv.first;
            auto& nexuses = kv.second;
            for (int i = 0; i < NUM_CONNECTIONS; ++i)
            {
                std::string my_cat = "cat-recv-" + std::to_string(mpi_rank) + "-" + std::to_string(upstream) + "-" + std::to_string(i);
                double received = nexuses[i]->get_downstream_flow(my_cat, ts, 100.0);
                ASSERT_DOUBLE_EQ(flow_value, received);
            }
        }
    }
    
    senders.clear();
    receivers.clear();
    
    MPI_Barrier(MPI_COMM_WORLD);
    std::cerr << "Rank " << mpi_rank << ": Test PASSED - no deadlock with remote nexus\n";
}


//#endif  // NGEN_MPI_TESTS_ACTIVE

//#endif  // NGEN_MPI_TESTS_ACTIVE
