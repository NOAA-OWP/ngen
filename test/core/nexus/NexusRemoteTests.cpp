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
    HY_PointHydroNexusRemote::catcment_location_map_t loc_map;

    std::shared_ptr<HY_PointHydroNexusRemote> nexus;
    std::vector<string> upstream_catchments = {"cat-26"};
    std::vector<string> downstream_catchments = {"cat-27"};

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

TEST_F(Nexus_Remote_Test, DISABLED_TestDeadlock1)
{
    HY_PointHydroNexusRemote::catcment_location_map_t loc_map;

    std::shared_ptr<HY_PointHydroNexusRemote> nexus;

    double dummy_flow = -9999.0;
    double recieved_flow;
    long ts = 0;

    std::vector<string> catchments;

    // In this test both processes send before recieving. If communciation are synchronus this will dead lock

    // create a nexus at both ranks
    if ( mpi_rank == 0)
    {
        catchments.push_back("cat-26");
        loc_map["cat-26"] = 1;
        nexus = std::make_shared<HY_PointHydroNexusRemote>("nex-26", catchments, loc_map);

        // We use two differnt time steps becuase a nexus does not allow water to be added after a send
        nexus->add_upstream_flow(200.0,"cat-1",ts);
        nexus->get_downstream_flow("cat-26",ts,100);                      // sending to 26
        nexus->add_upstream_flow(dummy_flow,"cat-26",ts+1);                 // recieving from 26
        recieved_flow = nexus->get_downstream_flow("cat-2",ts+1,100);       // get the recieved flow
    }
    else if ( mpi_rank == 1)
    {
        catchments.push_back("cat-26");
        loc_map["cat-26"] = 0;
        nexus = std::make_shared<HY_PointHydroNexusRemote>("nex-26", catchments, loc_map);

        // We use two differnt time steps becuase a nexus does not allow water to be added after a send
        nexus->add_upstream_flow(200.0,"cat-1",ts+1);
        nexus->get_downstream_flow("cat-26",ts+1,100);                    // sending to 26
        nexus->add_upstream_flow(dummy_flow,"cat-26",ts);                 // recieving grom 26
        recieved_flow = nexus->get_downstream_flow("cat-2",ts,100);       // get the recieved flow
    }

    MPI_Barrier(MPI_COMM_WORLD);
}

TEST_F(Nexus_Remote_Test, TestTree1)
{
    int tree_height = 10;
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
            loc_map[string("cat-"+std::to_string(i))] = target_rank;
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
        std::vector<std::string> catchments;
        catchments.push_back(std::string("cat-"+std::to_string(i)));
        nexus_map[i] = std::make_shared<HY_PointHydroNexusRemote>("nex-"+std::to_string(i), catchments, loc_map);
    }

    //std::cerr << "-----Rank " << mpi_rank << " Finshed creating nexus objects\n";


    long ts = 0;

    // for each nexus accept upstream flow
    for( int i = stop_id; i >= start_id; --i )
    {
        //std::cerr << "-----Rank " << mpi_rank << " Processing nexus object at position " << i << std::endl;

        if ( leaf(i) )
        {
            // if this is a leaf we need a synthetic flow
            nexus_map[i]->add_upstream_flow(1.0,"cat-"+std::to_string(num_nodes+10), ts);

            int p = parent(i);
            if ( nexus_map.find(p) == nexus_map.end() )
            {
                nexus_map[i]->get_downstream_flow("cat-"+std::to_string(p), ts, 100.0);
                //std::cerr << "-----Rank " << mpi_rank << " remote send of leaf data from catchment- " << i << " to catchment-" << p << std::endl;
            }
        }
        else
        {
            // if this is not a leaf try to get flow from its upstreams

            int l = left(i);
            int r = right(i);

            std::string current_id = std::string("cat-"+std::to_string(i));
            std::string left_id = std::string("cat-"+std::to_string(l));
            std::string right_id = std::string("cat-"+std::to_string(r));
            float dummy_value;
            float flow;

            if ( loc_map.find(left_id) != loc_map.end() )
            {
                // l is a remote node
                //std::cerr << "-----Rank " << mpi_rank << " remote recieve of data from catchment- " << l << std::endl;
                nexus_map[i]->add_upstream_flow(dummy_value,left_id,ts);
            }
            else
            {
                //std::cerr << "-----Rank " << mpi_rank << " local recieve of data from catchment- " << l << std::endl;
                flow = nexus_map[l]->get_downstream_flow(current_id,ts,100.0);
                nexus_map[i]->add_upstream_flow(flow,left_id,ts);
            }

            if ( loc_map.find(right_id) != loc_map.end() )
            {
                // r is a remote node
                //std::cerr << "-----Rank " << mpi_rank << " remote recieve of data from catchment- " << r << std::endl;
                nexus_map[i]->add_upstream_flow(dummy_value,right_id,ts);
            }
            else
            {
                //std::cerr << "-----Rank " << mpi_rank << " local recieve of data from catchment- " << r << std::endl;
                flow = nexus_map[r]->get_downstream_flow(current_id,ts,100.0);
                nexus_map[i]->add_upstream_flow(flow,right_id,ts);
            }

            int p = parent(i);
            if ( nexus_map.find(p) == nexus_map.end() )
            {
                //std::cerr << "-----Rank " << mpi_rank << " remote send of data from catchment- " << i << std::endl;
                nexus_map[i]->get_downstream_flow("cat-"+std::to_string(p), ts, 100.0);
            }
        }

        //std::cerr << "-----Rank " << mpi_rank << " Finished processing nexus object at position " << i << std::endl;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if ( nexus_map.find(0) != nexus_map.end() )
    {
        double flow = nexus_map[0]->get_downstream_flow("cat-0", ts, 100.0);
        std::cerr << "Rank " << mpi_rank<< " final Flow = " << flow << std::endl;

        ASSERT_TRUE(flow == 512);
    }

    //std::cerr << "Process id " << getpid() << " Rank " << mpi_rank << " terminating\n";
}


//#endif  // NGEN_MPI_TESTS_ACTIVE
