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
                double recieved_flow = nexus->get_downstream_flow("cat-27",ts,100);
                ASSERT_EQ(discharge,recieved_flow);
                std::cerr << "Rank 1: Recieving flow of " << recieved_flow << " from catchment Nexus connected to catchment 26\n";
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
    double recieved_flow = -9999.0;

    for ( auto discharge : stored_discharge)
    {
        switch(mpi_rank)
        {
            
            case 0:
                recieved_flow = nexus->get_downstream_flow("cat-27",ts,100);
                ASSERT_EQ(discharge+discharge,recieved_flow);
                std::cerr << "Rank 0: Recieving flow of " << recieved_flow << " from catchment Nexus connected to catchment 27\n";
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
    double recieved_flow = -9999.0;

    for ( auto discharge : stored_discharge)
    {
        switch(mpi_rank)
        {
            
            case 0:
                nexus->add_upstream_flow(discharge,"cat-24",ts);
                recieved_flow = nexus->get_downstream_flow("cat-27",ts,100);
                ASSERT_EQ(discharge*3,recieved_flow);
                std::cerr << "Rank 0: Recieving flow of " << recieved_flow << " from catchment Nexus connected to catchment 27\n";
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
    double recieved_flow = -9999.0;

    for ( auto discharge : stored_discharge)
    {
        switch(mpi_rank)
        {
            
            case 0:
                nexus->add_upstream_flow(discharge,"cat-24",ts);
                recieved_flow = nexus->get_downstream_flow("cat-27",ts,100);
                ASSERT_EQ(discharge*3,recieved_flow);
                std::cerr << "Rank 0: Recieving flow of " << recieved_flow << " from catchment Nexus connected to catchment 27\n";
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
                recieved_flow = nexus->get_downstream_flow("cat-17",ts,100);
                ASSERT_EQ(discharge*3,recieved_flow);
                std::cerr << "Rank 3: Recieving flow of " << recieved_flow << " from catchment Nexus connected to catchment 27\n";
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
    double recieved_flow;
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
                              
        recieved_flow = nexus2->get_downstream_flow("cat-26",ts,100);       // get the recieved flow
        std::cout << "rank 0 recieved a flow of " << recieved_flow << "\n";
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
                              
        recieved_flow = nexus2->get_downstream_flow("cat-25",ts,100);       // get the recieved flow
        std::cout << "rank 1 recieved a flow of " << recieved_flow << "\n";
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


//#endif  // NGEN_MPI_TESTS_ACTIVE
